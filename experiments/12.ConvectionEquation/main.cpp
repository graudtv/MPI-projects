#include "Support/Evaluator.hpp"
#include "cxxmpi/cxxmpi.hpp"
#include "external/popl/include/popl.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <unistd.h>

namespace mpi = cxxmpi;

template <class T> const T &penultimate(const std::vector<T> &V) {
  assert(V.size() >= 2 &&
         "vector is too small and doesn't have pre-back element");
  return V[V.size() - 2];
}

/* The main part of algorithm
 *
 * X, T - x and t limits
 * M - number of points on x axis
 * K - number of points on t axis
 * PhiTy, PsiTy - functions double -> double
 * FTy - function (double, double) -> double
 */
template <class PhiTy, class PsiTy, class FTy>
mpi::GatherResult<double> compute(double X, double T, int M, int K, PhiTy &&Phi,
                                  PsiTy &&Psi, FTy &&F, bool Verbose) {
  double h = X / M;   // coordinate step
  double tau = T / K; // time step

  /* 0. Get normalized functions for convenience */
  auto NormPhi = [&](int m) {
    /* current algorithm uses (-1)-th and (M+1)-th elems
     * it could be fixed, if necessary */
    assert(m >= -1 && m <= M + 1 && "m out of bounds");
    return Phi(m * h);
  };
  auto NormPsi = [&](int k) {
    assert(k >= 0 && k <= K && "k out of bounds");
    return Psi(k * tau);
  };
  auto NormF = [&](int k, int m) {
    assert(k >= 0 && k <= K && "k out of bounds");
    assert(m >= 0 && m <= M && "m out of bounds");
    return F(k * tau, m * h);
  };

  /* 0.5. Other helper functions */
  /*
   *               Result
   *                  ^
   *                  |
   *                  |
   *  Left <-----> Central, F
   */
  auto leftAngle = [h, tau](double Left, double Central, double F) {
    return (F - (Central - Left) / h) * tau + Central;
  };

  /*
   *               Result
   *                  ^
   *                  |
   *                  |
   *  Left <--------------------> Rigth
   *                  ^ F
   *                  |
   *                  v
   *               Bottom
   */
  auto chest = [h, tau](double Left, double Right, double Bottom, double F) {
    return (2 * F - (Right - Left) / h) * tau + Bottom;
  };

  /*               Result
   *                  ^
   *                  |
   *                  |
   *  Left <--------------------> Rigth
   *                  F
   */
  auto central3pt = [h, tau](double Left, double Right, double F) {
    return (F - (Right - Left) / (2 * h)) * tau + 0.5 * (Right - Left);
  };

  /* 1. Split work
   * Each process handles some segment of X axis */
  auto Rank = mpi::commRank();
  auto CommSz = mpi::commSize();
  auto Range = util::WorkSplitterLinear(M + 1, CommSz).getRange(Rank);
  auto SegmentSz = Range.size(); // sizeof segment for current process
  assert(SegmentSz >= 2 && "too small work allocated for this worker");
  assert(std::fabs(NormPhi(0) - NormPsi(0)) < 0.01 &&
         "NormalizedPhi and NormalizedPsi significantly diverge at 0 (but must "
         "be the same)");

  /* 2. Init vectors */
  std::vector<double> Prev(SegmentSz);
  std::vector<double> Cur(SegmentSz);
  std::vector<double> Next(SegmentSz);
  /* elems that are fetched from the left and right neighbors */
  double LeftNeighbor = 0;
  double RightNeighbor = 0;

  /* fetch left and right elems taking neighbors into account */
  auto leftFrom = [&](const std::vector<double> &Vec, size_t I) {
    return (I == 0) ? LeftNeighbor : Vec[I - 1];
  };
  auto rightFrom = [&](const std::vector<double> &Vec, size_t I) {
    return (I == SegmentSz) ? RightNeighbor : Vec[I + 1];
  };

  /* converts local index to global */
  auto globalX = [=](size_t I) { return Range.FirstIdx + I; };

  /* 3. Fill the 0-th row with phi(x) */
  for (size_t I = 0; I < SegmentSz; ++I)
    Prev[I] = NormPhi(globalX(I));
  /* here we have out of bound access for processes with Rank == 0 and
   * with Rank == CommSz - 1, but I don't think it is a big problem now */
  LeftNeighbor = NormPhi(Range.FirstIdx - 1);
  RightNeighbor = NormPhi(Range.LastIdx);

  /* 4. Calculate the 1-st row with central 3-point scheme */
  for (size_t I = 0; I < SegmentSz; ++I) {
    Cur[I] =
        central3pt(leftFrom(Prev, I), rightFrom(Prev, I), NormF(0, globalX(I)));
  }

  /* 4.5. MORE LAMBDAS !!! */
  bool LeftNeighborExists = (Rank != 0);
  bool RightNeighborExists = (Rank != CommSz - 1);

  /* we can send to rightRank() even if there is no right
   * neighbor due to MPI_PROC_NULL feature */
  auto rightRank = [=] {
    return LeftNeighborExists ? (Rank - 1) : MPI_PROC_NULL;
  };
  auto leftRank = [=] {
    return RightNeighborExists ? (Rank + 1) : MPI_PROC_NULL;
  };

  /* sends Cur.back() to the right neighbor, Cur.front() to the left neighbor
   * and fetch LeftNeighbor, RightNeighbor from the left and right neighbor
   * If there is no left neighbor, fetches data from Psi instead
   * If there is no right neighbor, fetches data with leftAngle scheme instead
   * A trick is done to avoid deadlock */
  auto doMsgExchange = [&](size_t CurRow) {
    if (Rank % 2 == 0) {
      mpi::send(Cur.back(), rightRank());
      mpi::send(Cur.front(), leftRank());
      mpi::recv(LeftNeighbor, leftRank());
      mpi::recv(RightNeighbor, rightRank());
    } else {
      mpi::recv(LeftNeighbor, leftRank());
      mpi::recv(RightNeighbor, rightRank());
      mpi::send(Cur.back(), rightRank());
      mpi::send(Cur.front(), leftRank());
    }
    if (!LeftNeighborExists)
      LeftNeighbor = NormPsi(CurRow);
    if (!RightNeighborExists)
      RightNeighbor =
          leftAngle(penultimate(Prev), Prev.back(), NormF(CurRow - 1, M));
  };

  /* 5. Exchange corner elements of the 1-st row between segments */
  doMsgExchange(1);

  /* 6. Now we are ready to use the cross scheme
   * Row here is the "current" row. We are fetching the "next" row,
   * i.e. Row + 1 */
  for (int Row = 1; Row < K; ++Row) {
    for (size_t I = 0; I < SegmentSz; ++I) {
      Next[I] = chest(leftFrom(Cur, I), rightFrom(Cur, I), Prev[I],
                      NormF(Row, globalX(I)));
    }
    std::swap(Prev, Cur);
    std::swap(Cur, Next);
    /* We don't need to exchange corners for the very last row */
    if (Row != K - 1)
      doMsgExchange(Row);
  }

  if (Verbose) {
    std::cout << mpi::whoami << ": complete!!!" << std::endl;
    // std::cout << mpi::whoami << "data: " << util::join(Cur) << std::endl;
  }
  return mpi::gatherv(Cur, 0);
}

/* Phi, PhiStr, Psi, PsiStr - out args */
bool parseBoundaryConditions(std::istream &Is, util::Evaluator &Phi,
                             std::string &PhiStr, util::Evaluator &Psi,
                             std::string &PsiStr, util::Evaluator2D &F,
                             std::string &FStr, bool EnablePrompts = true) {
  if (EnablePrompts) {
    std::cout << "Enter boundary conditions" << std::endl;
    std::cout << "phi(x) = u(0, x) = ";
  }

  std::getline(Is, PhiStr);
  if (!Phi.parse(PhiStr, "x")) {
    std::cerr << "Error: " << Phi.getErrorStr() << std::endl;
    return false;
  }

  if (EnablePrompts)
    std::cout << "psi(t) = u(t, 0) = ";
  std::getline(Is, PsiStr);
  if (!Psi.parse(PsiStr, "t")) {
    std::cerr << "Error: " << Psi.getErrorStr() << std::endl;
    return false;
  }

  if (EnablePrompts)
    std::cout << "f(t, x) = ";
  std::getline(Is, FStr);
  if (!F.parse(FStr, "t", "x")) {
    std::cerr << "Error: " << F.getErrorStr() << std::endl;
    return false;
  }
  return true;
}

popl::OptionParser Op("Options");

void emitUsageError(const char *msg) {
  std::cerr << "Error: " << msg << std::endl
            << std::endl
            << "Usage: ./prog [OPTIONS]" << std::endl
            << Op <<
      R"(
Short example:
  ./prog -X 1.0 -T 0.05 -M 400 -K 20

About:
  This program solves convection equation:
        du(t, x)/dt + du(t, x)/dx = f(t, x),      0 <= t <= T, 0 <= x <= X
        u(0, x) = phi(x),                         0 <= x <= X
        u(t, 0) = psi(t),                         0 <= t <= T

  The program may be run both as a single process and in parallel

  X, T are passed as command line options.
  Functions phi(x), psi(t) and f(t, x) are specified in text format at runtime.
  See Input Data Format section to understand how to specify them

  Options M, K specify number of x and t axis divisions
  Other options are optional

Input Data Format:
  Input data is read from stdin or a file specified with -f option by
  a process with rank 0.
  Input should contain 3 lines. First two lines are expressions
  specifying boundary condition functions phi(x) = u(0, x) and
  psi(t) = u(t, 0). The last line specifies function f(t, x)
  Input is parsed with libmatheval, see its documentation for valid
  expression formats.

  Example:
  $ cat > input.txt << EOF
  > x
  > sin(t) + t ^ 2
  > x + t + x * t
  > EOF
  $ ./prog -X 1.0 -T 0.05 -M 400 -K 20 --file input.txt
  $ cat input.txt | ./prog -X 1.0 -T 0.05 -M 400 -K 20
  $ cat input.txt | mpirun -n 4 ./prog -X 1.0 -T 0.05 -M 400 -K 20
)" << std::endl;
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) try {
  mpi::MPIContext Ctx{&argc, &argv};
  mpi::Timer InitTmr;

  using popl::Attribute;
  using popl::Switch;
  using popl::Value;

  auto X = Op.add<Value<double>, Attribute::required>("X", "",
                                                      "x range, 0 <= x <= X");
  auto T = Op.add<Value<double>, Attribute::required>("T", "",
                                                      "t range, 0 <= t <= T");
  auto M = Op.add<Value<int>, Attribute::required>(
      "M", "", "number of x axis divisions, x_step = X / M");
  auto K = Op.add<Value<int>, Attribute::required>(
      "K", "", "number of t axis divisions, t_step = T / K");
  auto Filename = Op.add<Value<std::string>>(
      "f", "file", "use specified file instead of stdin");

  auto Verbose = Op.add<Switch>("v", "verbose", "emit debug information");
  auto PrintTime = Op.add<Switch>("t", "time", "print computation time");

  Op.parse(argc, argv);

  if (Op.non_option_args().size() != 0) {
    emitUsageError(
        ("unexpected argument " + Op.non_option_args().front()).c_str());
  }
  if (Op.unknown_options().size() != 0)
    emitUsageError(("unknown option " + Op.unknown_options().front()).c_str());
  if (X->value() <= 0 || T->value() <= 0 || M->value() <= 1 || K->value() <= 0)
    emitUsageError("one of parameters is unadequate");

  std::ifstream Ifs;            // not used if using stdin
  std::istream *Is = &std::cin; // points to actually used stream

  if (Filename->is_set()) {
    Ifs.open(Filename->value());
    if (!Ifs) {
      std::cerr << "Error: failed to open file " << Filename->value()
                << std::endl;
      exit(EXIT_FAILURE);
    }
    Is = &Ifs;
  }

  std::string PhiStr, PsiStr, FStr;
  util::Evaluator Phi, Psi;
  util::Evaluator2D F;

  if (mpi::commRank() == 0) {
    /* enabling user prompts if reading from terminal */
    bool EnablePrompts = (Is == &std::cin) && isatty(fileno(stdin));
    /* try parse Phi and Psi functions */
    if (!parseBoundaryConditions(*Is, Phi, PhiStr, Psi, PsiStr, F, FStr,
                                 EnablePrompts))
      return EXIT_FAILURE;
    constexpr double CmpPrec = 0.01;
    if (abs(Phi(0) - Psi(0)) >= CmpPrec) {
      std::cerr << "Error: phi(0) must be equal to psi(0), but they are "
                   "significantly different: phi(0) = "
                << Phi(0) << ", psi(0) = " << Psi(0) << std::endl;
      return EXIT_FAILURE;
    }
  }
  /* broadcast functions to everyone else
   * if there is only one process, no need to broadcast, the process is alone
   * in the MPI world :(( */
  if (mpi::commSize() > 1) {
    mpi::bcast(PhiStr, 0);
    mpi::bcast(PsiStr, 0);
    mpi::bcast(FStr, 0);
    if (mpi::commRank() != 0 &&
        !(Phi.parse(PhiStr, "x") && Psi.parse(PsiStr, "t")) &&
        F.parse(FStr, "t", "x")) {
      std::cerr << "Fatal error: root was able to parse function expressions, "
                   "but this process failed to"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  /* time spent initialization */
  double InitTime = InitTmr.getElapsedTimeInSeconds();

  mpi::Timer ComputationTmr;
  auto Res = compute(
      X->value(), T->value(), M->value(), K->value(), Phi, Psi,
      [](double, double) { return 0; }, Verbose->value());
  if (PrintTime->value() && mpi::commRank() == 0) {
    printf("Initialization time: %lg s\n"
           "Computation time: %lg s\n",
           InitTime, ComputationTmr.getElapsedTimeInSeconds());
  }
  if (Res) {
    std::cout << util::join(Res.data()) << std::endl;
  }
  return 0;
} catch (popl::invalid_option &e) {
  emitUsageError(e.what());
}