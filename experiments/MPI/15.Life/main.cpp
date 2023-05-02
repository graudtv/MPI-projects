#include <SFML/Graphics.hpp>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <imgui.h>
#include <imgui-SFML.h>

enum Cell { Dead = 0, Alive = 1 };

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

  bool empty() const { return Data.empty(); }

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

std::mutex GlobalAccess;
GameMap GlobalMap;
bool ViewUpdateAvail = false;

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
      Rect.setFillColor(Map(I, J) ? sf::Color::Black : sf::Color{240, 240, 240});
      Target.draw(Rect);
    }

  sf::RectangleShape Frame{sf::Vector2f(Map.getWidth(), Map.getHeight())};
  Frame.setOutlineColor(sf::Color::Green);
  Frame.setOutlineThickness(-0.1);
  Frame.setFillColor(sf::Color::Transparent);
  Target.draw(Frame);
}

void visualizer() {
  sf::RenderWindow Win{sf::VideoMode{1600, 1200}, "Life Simulator"};
  Win.setFramerateLimit(60);

  if (!ImGui::SFML::Init(Win, /* loadDefaultFont=*/false)) {
    std::cerr << "Failed to initialized Dear ImGui" << std::endl;
    exit(1);
  }

  ImGuiIO &IO = ImGui::GetIO();
  ImGui::StyleColorsDark();
  ImGui::GetStyle().ScaleAllSizes(2.0f);

  IO.Fonts->AddFontFromFileTTF("../../../../external/imgui/misc/fonts/DroidSans.ttf", 32);
  if (!ImGui::SFML::UpdateFontTexture()) {
    std::cerr << "Failed to load font" << std::endl;
    exit(1);
  }

  GameMap Map;
  sf::Clock Tmr;

  while (Win.isOpen()) {
    auto ElapsedTime = Tmr.restart();
    ImGui::SFML::Update(Win, ElapsedTime);

    sf::Event E;
    while (Win.pollEvent(E)) {
      ImGui::SFML::ProcessEvent(Win, E);
      if (E.type == sf::Event::Closed)
        Win.close();
    }
    {
      std::lock_guard<std::mutex> Lock{GlobalAccess};
      if (ViewUpdateAvail) {
        Map = GlobalMap;
        ViewUpdateAvail = false;
      }
    }
    //ImGui::Begin("Simulation Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetNextWindowSize(ImVec2(700, 0), ImGuiCond_Always);
    ImGui::Begin("Simulation Control");

    ImGui::AlignTextToFramePadding();
    ImGui::BulletText("single step");
    ImGui::SameLine();
    ImGui::Button("run");

    ImGui::AlignTextToFramePadding();
    ImGui::BulletText("multistep");
    ImGui::SameLine();
    ImGui::Button("run");
    ImGui::Indent();
    ImGui::Text("count");
    ImGui::Text("delay");
    ImGui::Unindent();

    //ImGui::BulletText("status");
    //ImGui::Indent();
    //ImGui::NewLine();
    ImGui::Separator();
    static int i = 0;
    if (i > 300)
      i = 0;
    ImGui::Text("Last command: 10 steps in %d ms, 10.0 steps/s", ++i);
    ImGui::Text("Total: 1000 steps in 2.2 s, 9.2 steps/s");
    //ImGui::Unindent();
#if 0
    if (ImGui::TreeNode("single step")) {
      if (ImGui::Button("step"))
        std::cout << "single step" << std::endl;
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("multistep")) {
      if (ImGui::Button("steps"))
        std::cout << "step" << std::endl;
      ImGui::TreePop();
    }
#endif
    ImGui::End();

    Win.clear(sf::Color::White);
    drawMap(Win, Map);

    Win.setView(Win.getDefaultView());
    ImGui::SFML::Render(Win);
    Win.display();
  }
  ImGui::SFML::Shutdown();
  exit(0);
}

void cli() {
  std::this_thread::sleep_for(std::chrono::seconds(2));

  {
    std::lock_guard<std::mutex> Lock{GlobalAccess};
    GlobalMap.readFromFile("../assets/2.txt");
    ViewUpdateAvail = true;
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  {
    std::lock_guard<std::mutex> Lock{GlobalAccess};
    GlobalMap.readFromFile("../assets/1.txt");
    ViewUpdateAvail = true;
  }
}

int main() {
  std::thread CLI{cli};
  visualizer();

  CLI.join();
  return 0;
}
