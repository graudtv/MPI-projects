#include <SFML/Graphics.hpp>
#include <cassert>
#include <cxxmpi/cxxmpi.hpp>
#include <fstream>
#include <imgui-SFML.h>
#include <imgui.h>
#include <iostream>
#include <mutex>
#include <thread>

enum Cell : char { Dead = 0, Alive = 1 };
enum class MPICommand : int { Gather, Step, Shutdown };

template <> struct cxxmpi::DatatypeSelector<Cell, void> {
  static MPI_Datatype getHandle() {
    return BuiltinTypeTraits<char>::getHandle();
  }
};

template <> struct cxxmpi::DatatypeSelector<MPICommand, void> {
  static MPI_Datatype getHandle() {
    return BuiltinTypeTraits<int>::getHandle();
  }
};

struct CommandStats {
  unsigned StepCount = 0;
  sf::Time Duration = sf::seconds(0);
};

void showCommandStats(const char *Prefix, const CommandStats &Stats) {
  if (Stats.StepCount) {
    ImGui::Text("%s: %u steps in %d ms, %0lg steps/s", Prefix, Stats.StepCount,
                Stats.Duration.asMilliseconds(),
                Stats.StepCount / Stats.Duration.asSeconds());
  } else {
    ImGui::Text("%s: not available", Prefix);
  }
}

class GameMap {
  std::vector<Cell> Data;
  size_t Width = 0;

public:
  size_t getWidth() const { return Width; }
  size_t getHeight() const { return Data.size() / getWidth(); }

  Cell &getCell(size_t I, size_t J) { return Data[I * Width + J]; }
  Cell getCell(size_t I, size_t J) const { return Data[I * Width + J]; }

  Cell &operator()(size_t I, size_t J) { return getCell(I, J); }
  Cell operator()(size_t I, size_t J) const { return getCell(I, J); }

  void clear() {
    Data.clear();
    Width = 0;
  }

  const std::vector<Cell> &buf() const { return Data; }

  bool empty() const { return Data.empty(); }

  void init(std::vector<Cell> Map, size_t MapWidth) {
    Data = std::move(Map);
    Width = MapWidth;
  }

  void append(const std::vector<Cell> &Rows) {
    if (Data.empty())
      Width = Rows.size();
    assert((Rows.size() % Width == 0) && "Row size mismatch");
    std::copy(Rows.begin(), Rows.end(), std::back_inserter(Data));
  }

  void readFromStream(std::istream &Is) {
    clear();
    std::string Input;
    while (std::getline(Is, Input)) {
      if (Input.empty())
        continue;
      if (Width == 0)
        Width = Input.size();
      if (Input.size() != Width)
        throw std::runtime_error{"Broken input file: mismatch in line sizes"};
      std::transform(Input.begin(), Input.end(), std::back_inserter(Data),
                     [](char C) { return (C == 'x') ? Alive : Dead; });
    }
  }

  /* Get data from rows in range [Fst; Last) */
  std::vector<Cell> extractRows(size_t Fst, size_t Last) const {
    std::vector<Cell> Result((Last - Fst) * getWidth());
    std::copy(&Data[Fst * getWidth()], &Data[Last * getWidth()], Result.data());
    return Result;
  }

  std::vector<Cell> extractRow(size_t Idx) const { return extractRows(Idx, Idx + 1); }

  void readFromFile(const std::string &Path) {
    std::ifstream Is{Path};
    if (!Is.is_open())
      throw std::runtime_error{"Failed to open file '" + Path + "'"};
    readFromStream(Is);
  }

  void writeToStream(std::ostream &Os) {
    for (size_t I = 0, Height = getHeight(); I < Height; ++I) {
      for (size_t J = 0; J < getWidth(); ++J)
        Os << (getCell(I, J) ? 'x' : '.');
      Os << std::endl;
    }
  }

  void writeToFile(const std::string &Path) {
    std::ofstream Os{Path};
    if (!Os.is_open())
      throw std::runtime_error{"Failed to open file '" + Path + "'"};
    writeToStream(Os);
  }
};

#if 0
void dumpMap(const GameMap &Map) {
  for (size_t I = 0; I < Map.getHeight(); ++I) {
    std::cout << cxxmpi::whoami << ": ";
    for (size_t J = 0; J < Map.getWidth(); ++J)
      std::cout << (Map(I, J) ? 'x' : '.');
    std::cout << std::endl;
  }
}
#define dbg() std::cout
#else
void dumpMap(const GameMap &) {}
#define dbg() if (0) std::cout
#endif

std::mutex GlobalAccess;
/* Variables exported by vizualizer thread */
std::condition_variable CommandAvail;
volatile unsigned StepsToRun = 0;
sf::Time StepDelayTime = sf::milliseconds(0);
volatile bool Shutdown = false;

/* Variables exported by MPI thread */
GameMap GlobalMap;
CommandStats LastCommandStats{};
CommandStats TotalCommandStats{};
bool ViewUpdateAvail = false;

/* Part of global map on which MPI executor is working */
GameMap LocalMap;

void drawMap(sf::RenderTarget &Target, const GameMap &Map) {
  if (Map.empty())
    return;

  /* Adjust sf::View to fit map into the window */
  constexpr float BorderRatio = 0.05;
  auto Scale =
      (1 - BorderRatio) * std::min(Target.getSize().x / Map.getWidth(),
                                   Target.getSize().y / Map.getHeight());
  sf::View V{sf::FloatRect(0, 0, Map.getWidth(), Map.getHeight())};
  float WidthRatio = (Map.getWidth() * Scale) / Target.getSize().x;
  float HeightRatio = (Map.getHeight() * Scale) / Target.getSize().y;
  V.setViewport(sf::FloatRect{(1 - WidthRatio) / 2, (1 - HeightRatio) / 2,
                              WidthRatio, HeightRatio});
  Target.setView(V);

  sf::RectangleShape Rect{sf::Vector2f{1, 1}};
  Rect.setFillColor(sf::Color::Black);
  for (size_t I = 0; I < Map.getHeight(); ++I)
    for (size_t J = 0; J < Map.getWidth(); ++J) {
      Rect.setPosition(J, I);
      Rect.setFillColor(Map(I, J) ? sf::Color::Black
                                  : sf::Color{240, 240, 240});
      Target.draw(Rect);
    }
  /* Horizontal grid lines */
  for (size_t I = 1; I < Map.getHeight(); ++I) {
    sf::Vertex Line[] = {
        sf::Vertex{sf::Vector2f(0, I), sf::Color::Black},
        sf::Vertex{sf::Vector2f(Map.getWidth(), I), sf::Color::Black}};
    Target.draw(Line, sizeof(Line) / sizeof(Line[0]), sf::Lines);
  }
  /* Vertical grid lines */
  for (size_t J = 1; J < Map.getWidth(); ++J) {
    sf::Vertex Line[] = {
        sf::Vertex{sf::Vector2f(J, 0), sf::Color::Black},
        sf::Vertex{sf::Vector2f(J, Map.getWidth()), sf::Color::Black}};
    Target.draw(Line, sizeof(Line) / sizeof(Line[0]), sf::Lines);
  }

  sf::RectangleShape Frame{sf::Vector2f(Map.getWidth(), Map.getHeight())};
  Frame.setOutlineColor(sf::Color::Green);
  Frame.setOutlineThickness(-0.1);
  Frame.setFillColor(sf::Color::Transparent);
  Target.draw(Frame);
}

void visualizer() {
  const auto CommSize = cxxmpi::commSize();
  sf::RenderWindow Win{sf::VideoMode{1600, 1200}, "Game Of Life"};
  sf::RenderWindow CtlWin{sf::VideoMode{1000, 400}, "Game Of Life Control"};
  CtlWin.setFramerateLimit(60);
  Win.setFramerateLimit(60);

  if (!ImGui::SFML::Init(CtlWin, /* loadDefaultFont=*/false)) {
    std::cerr << "Failed to initialized Dear ImGui" << std::endl;
    exit(1);
  }

  ImGuiIO &IO = ImGui::GetIO();
  ImGui::StyleColorsDark();
  ImGui::GetStyle().ScaleAllSizes(2.0f);

  IO.Fonts->AddFontFromFileTTF(
      "../../../../external/imgui/misc/fonts/DroidSans.ttf", 32);
  if (!ImGui::SFML::UpdateFontTexture()) {
    std::cerr << "Failed to load font" << std::endl;
    exit(1);
  }

  GameMap Map;
  sf::Clock Tmr;
  int StepCount = 10;
  int StepDelay = 0;
  CommandStats LastStats, TotalStats;

  while (Win.isOpen() && CtlWin.isOpen()) {
    auto ElapsedTime = Tmr.restart();
    ImGui::SFML::Update(CtlWin, ElapsedTime);

    sf::Event E;
    while (Win.pollEvent(E))
      if (E.type == sf::Event::Closed)
        Win.close();
    while (CtlWin.pollEvent(E)) {
      ImGui::SFML::ProcessEvent(CtlWin, E);
      if (E.type == sf::Event::Closed)
        CtlWin.close();
    }
    {
      std::lock_guard<std::mutex> Lock{GlobalAccess};
      if (ViewUpdateAvail) {
        Map = GlobalMap;
        LastStats = LastCommandStats;
        TotalStats = TotalCommandStats;
        ViewUpdateAvail = false;
      }
    }
    ImGui::SetNextWindowSize(ImVec2(CtlWin.getSize().x, CtlWin.getSize().y),
                             ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    constexpr auto WindowFlags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove;
    ImGui::Begin("Simulation Control", nullptr, WindowFlags);
    {
      ImGui::Text("Status:");
      ImGui::SameLine();
      if (StepsToRun)
        ImGui::TextColored(ImVec4{0, 1, 0, 1}, "running");
      else
        ImGui::TextColored(ImVec4{1, 1, 0, 1}, "pending");
      ImGui::Separator();

      ImGui::AlignTextToFramePadding();
      ImGui::BulletText("single step");
      ImGui::SameLine();
      if (ImGui::Button("run##0")) {
        std::lock_guard<std::mutex> Lock{GlobalAccess};
        if (!StepsToRun) {
          StepsToRun = 1;
          StepDelayTime = sf::milliseconds(0);
          CommandAvail.notify_one();
        }
      }

      ImGui::AlignTextToFramePadding();
      ImGui::BulletText("multistep");
      ImGui::SameLine();
      if (ImGui::Button("run##1")) {
        std::lock_guard<std::mutex> Lock{GlobalAccess};
        if (!StepsToRun) {
          StepsToRun = StepCount;
          StepDelayTime = sf::milliseconds(StepDelay);
          CommandAvail.notify_one();
        }
      }
      ImGui::Indent();
      if (ImGui::InputInt("count", &StepCount) && StepCount < 1)
        StepCount = 1;
      if (ImGui::InputInt("delay (ms)", &StepDelay) && StepDelay < 0)
        StepDelay = 0;
      ImGui::Unindent();

      ImGui::Separator();
      ImGui::Text("Running on %d MPI executor%s", CommSize,
                  CommSize > 1 ? "s" : "");
      showCommandStats("Last command", LastStats);
      showCommandStats("Total", TotalStats);
    }
    ImGui::End();

    Win.clear(sf::Color::White);
    drawMap(Win, Map);

    Win.setView(Win.getDefaultView());
    Win.display();

    CtlWin.clear(sf::Color::White);
    ImGui::SFML::Render(CtlWin);
    CtlWin.display();
  }
  ImGui::SFML::Shutdown();

  std::lock_guard<std::mutex> Lock{GlobalAccess};
  Shutdown = true;
  CommandAvail.notify_one();
}

/* Scatter GlobalMap on root MPI executor to others */
void mpiScatterGameMap() {
  GameMap MapToSend;
  if (cxxmpi::commRank() == 0) {
    std::lock_guard<std::mutex> Lock{GlobalAccess};
    MapToSend = GlobalMap;
  }
  size_t MapWidth = MapToSend.getWidth();
  cxxmpi::bcast(MapWidth, 0);

  if (cxxmpi::commRank() == 0) {
    auto WorkerCount = cxxmpi::commSize();
    auto Splitter =
        util::WorkSplitterLinear(MapToSend.getHeight(), WorkerCount);

    for (unsigned I = 1; I < WorkerCount; ++I) {
      auto Rows = Splitter.getRange(I);
      auto Data = MapToSend.extractRows(Rows.FirstIdx, Rows.LastIdx);
      cxxmpi::send(Data, I);
    }
    auto Rows = Splitter.getRange(0);
    LocalMap.init(MapToSend.extractRows(Rows.FirstIdx, Rows.LastIdx), MapWidth);
  } else {
    std::vector<Cell> Buf;
    cxxmpi::recv(Buf, 0);
    LocalMap.init(std::move(Buf), MapWidth);
  }
  std::cout << cxxmpi::whoami << ": init local map " << LocalMap.getWidth()
            << " x " << LocalMap.getHeight() << std::endl;
  dumpMap(LocalMap);
}

/* Receive local maps from MPI executors and put them into GlobalMap on root */
void mpiGatherGameMap() {
  // std::cout << cxxmpi::whoami << ": gather" << std::endl;
  if (auto Res = cxxmpi::gatherv(LocalMap.buf())) {
    std::lock_guard<std::mutex> Lock{GlobalAccess};
    GlobalMap.init(Res.takeData(), LocalMap.getWidth());
    ViewUpdateAvail = true;
  }
}

void mpiStep() {
  dbg() << cxxmpi::whoami << ": step" << std::endl;
  const auto CommRank = cxxmpi::commRank();
  const auto CommSize = cxxmpi::commSize();
  /* Lower = lower index, Higher = higher index */
  const auto LowerNeighbor = (CommRank + CommSize - 1) % CommSize;
  const auto UpperNeighbor = (CommRank + 1) % CommSize;
  const auto MapWidth = LocalMap.getWidth();

  const auto extractLowerRow = [](const GameMap &Map) {
    return Map.extractRow(0);
  };
  const auto extractUpperRow = [](const GameMap &Map) {
    return Map.extractRow(Map.getHeight() - 1);
  };

  std::vector<Cell> LowerRow;
  std::vector<Cell> UpperRow;

  if (cxxmpi::commSize() > 1) {
    if (cxxmpi::commRank() % 2 == 0) {
      cxxmpi::send(extractLowerRow(LocalMap), LowerNeighbor);
      cxxmpi::send(extractUpperRow(LocalMap), UpperNeighbor);
      cxxmpi::recv(UpperRow, UpperNeighbor);
      cxxmpi::recv(LowerRow, LowerNeighbor);
    } else {
      cxxmpi::recv(UpperRow, UpperNeighbor);
      cxxmpi::recv(LowerRow, LowerNeighbor);
      cxxmpi::send(extractLowerRow(LocalMap), LowerNeighbor);
      cxxmpi::send(extractUpperRow(LocalMap), UpperNeighbor);
    }
  } else {
    LowerRow = extractUpperRow(LocalMap);
    UpperRow = extractLowerRow(LocalMap);
  }

  GameMap Map;
  Map.append(LowerRow);
  Map.append(LocalMap.buf());
  Map.append(UpperRow);

  dumpMap(Map);

  auto lcell = [=](unsigned J) { return (J + MapWidth - 1) % MapWidth; };
  auto rcell = [=](unsigned J) { return (J + 1) % MapWidth; };
  auto calcIsAlive = [](bool WasAlive, unsigned AliveNeighbors) {
    if (WasAlive)
      return (AliveNeighbors == 2 || AliveNeighbors == 3) ? Alive : Dead;
    return (AliveNeighbors == 3) ? Alive : Dead;
  };

  for (size_t I = 1; I + 1 < Map.getHeight(); ++I)
    for (size_t J = 0; J < MapWidth; ++J) {
      unsigned AliveCount = Map(I - 1, lcell(J)) + Map(I - 1, J) +
                            Map(I - 1, rcell(J)) + Map(I, lcell(J)) +
                            Map(I, rcell(J)) + Map(I + 1, lcell(J)) +
                            Map(I + 1, J) + Map(I + 1, rcell(J));
      LocalMap(I - 1, J) = calcIsAlive(Map(I, J), AliveCount);
    }
}

void mpiRoot() {
  unsigned StepCount = 0;
  sf::Time StepDelay{};
  MPICommand Cmd;

  mpiScatterGameMap();
  while (1) {
    {
      std::unique_lock<std::mutex> Lock{GlobalAccess};
      CommandAvail.wait(Lock, []() { return StepsToRun || Shutdown; });
      StepCount = StepsToRun;
      StepDelay = StepDelayTime;
    }
    if (Shutdown) {
      cxxmpi::bcast((Cmd = MPICommand::Shutdown), 0);
      return;
    }

    sf::Time ExecutionTime{};

    for (unsigned I = 0; I < StepCount; ++I) {
      if (Shutdown) {
        cxxmpi::bcast((Cmd = MPICommand::Shutdown), 0);
        return;
      }

      sf::Clock Tmr;
      cxxmpi::bcast((Cmd = MPICommand::Step), 0);
      mpiStep();
      ExecutionTime += Tmr.getElapsedTime();

      /* Handle interactive mode (i.e. multistep mode with nonzero delay) */
      if (StepDelay > sf::seconds(0)) {
        cxxmpi::bcast((Cmd = MPICommand::Gather), 0);
        mpiGatherGameMap();
        auto TimeToSleep = StepDelay - Tmr.getElapsedTime();
        if (TimeToSleep > sf::seconds(0))
          sf::sleep(TimeToSleep);
      }
    }

    cxxmpi::bcast((Cmd = MPICommand::Gather), 0);
    mpiGatherGameMap();
    {
      std::lock_guard<std::mutex> Lock{GlobalAccess};
      LastCommandStats.StepCount = StepCount;
      LastCommandStats.Duration = ExecutionTime;
      TotalCommandStats.StepCount += StepCount;
      TotalCommandStats.Duration += ExecutionTime;
      ViewUpdateAvail = true;
      StepsToRun = 0;
    }
  }
}

void mpiSecondary() {
  mpiScatterGameMap();
  while (1) {
    MPICommand Cmd;
    dbg() << cxxmpi::whoami << ": waiting for command" << std::endl;
    cxxmpi::bcast(Cmd, 0);
    dbg() << cxxmpi::whoami << ": command received" << std::endl;
    if (Cmd == MPICommand::Shutdown)
      return;
    if (Cmd == MPICommand::Gather) {
      mpiGatherGameMap();
      continue;
    }
    if (Cmd == MPICommand::Step) {
      mpiStep();
      continue;
    }
    assert(0 && "unknown command");
  }
}

int main(int argc, char *argv[]) {
  cxxmpi::MPIContext Ctx{&argc, &argv};
  if (cxxmpi::commRank() == 0) {
    if (argc != 2) {
      std::cerr << "Usage: " << argv[0] << " <path_to_map>" << std::endl;
      exit(1);
    }
    GlobalMap.readFromFile(argv[1]);
    ViewUpdateAvail = true;

    std::thread MPI{mpiRoot};
    visualizer();
    MPI.join();
  } else
    mpiSecondary();
  return 0;
}
