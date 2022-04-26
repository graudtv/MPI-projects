#include "Support/Evaluator.hpp"
#include "cxxmpi/cxxmpi.hpp"
#include "external/popl/include/popl.hpp"
#include <cmath>
#include <fstream>
#include <iomanip>
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

  /* 0. Normalized functions for convenience */
  auto NormPhi = [&](int m) {
    assert(m >= 0 && m <= M && "m out of bounds");
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

  // std::cout << F(5, 3) << std::endl;
  // std::cout << NormF(2, 1) << std::endl;

  /* 0.5. Functions for different computation schemes */
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
  auto cross = [h, tau](double Left, double Right, double Bottom, double F) {
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
    return (F - (Right - Left) / (2 * h)) * tau + 0.5 * (Right + Left);
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
  bool LeftNeighborExists = (Rank != 0);
  bool RightNeighborExists = (Rank != CommSz - 1);

  /* converts local index to global */
  auto globalX = [=](size_t I) { return Range.FirstIdx + I; };

  /* 2. Prepare vectors */
  std::vector<double> Prev(SegmentSz);
  std::vector<double> Cur(SegmentSz);
  std::vector<double> Next(SegmentSz);

  /* 3. Fill the 0-th row with phi(x) */
  for (size_t I = 0; I < SegmentSz; ++I)
    Prev[I] = NormPhi(globalX(I));

  if (Verbose) {
    std::cout << mpi::whoami << ": row[0] = " << util::join(Prev, ", ")
              << std::endl;
  }

  /* 4. Calculate the 1-st row with central 3-point scheme (where possible) */
  constexpr int Row = 0; // the current row
  for (size_t I = 1; I < SegmentSz - 1; ++I)
    Cur[I] = central3pt(Prev[I - 1], Prev[I + 1], NormF(Row, globalX(I)));
  Cur.front() = LeftNeighborExists ? central3pt(NormPhi(globalX(-1)), Prev[1],
                                                NormF(Row, globalX(0)))
                                   : Psi(Row + 1);
  Cur.back() = RightNeighborExists
                   ? central3pt(penultimate(Prev), NormPhi(globalX(SegmentSz)),
                                NormF(Row, globalX(SegmentSz - 1)))
                   : leftAngle(penultimate(Prev), Prev.back(),
                               NormF(Row, globalX(SegmentSz - 1)));

  /* 5. Prepare to run cross scheme */
  /* elems feteched from the left and right neighbors */
  double LeftNeighbor = 0;
  double RightNeighbor = 0;

  /* using MPI_PROC_NULL feature */
  auto RightRank = RightNeighborExists ? (Rank + 1) : MPI_PROC_NULL;
  auto LeftRank = LeftNeighborExists ? (Rank - 1) : MPI_PROC_NULL;

  /* sends Cur.back() to the right neighbor, Cur.front() to the left neighbor
   * and fetch LeftNeighbor, RightNeighbor from the left and right neighbor
   * A trick is done to avoid deadlock */
  auto doMsgExchange = [&]() {
    if (Rank % 2 == 0) {
      mpi::send(Cur.back(), RightRank);
      mpi::send(Cur.front(), LeftRank);
      mpi::recv(LeftNeighbor, LeftRank);
      mpi::recv(RightNeighbor, RightRank);
    } else {
      mpi::recv(LeftNeighbor, LeftRank);
      mpi::recv(RightNeighbor, RightRank);
      mpi::send(Cur.back(), RightRank);
      mpi::send(Cur.front(), LeftRank);
    }
  };

  /* 6. Exchange corner elements of the 1-st row between segments */
  doMsgExchange();

  /* 7. Now we are ready to use the cross scheme
   * "Row" variable here is the "current" row. We are fetching the "next" row,
   * i.e. Row + 1 */
  for (int Row = 1; Row < K; ++Row) {
    if (Verbose) {
      std::cout << mpi::whoami << ": row[" << Row
                << "] = " << util::join(Cur, ", ") << std::endl;
    }

    for (size_t I = 1; I < SegmentSz - 1; ++I)
      Next[I] = cross(Cur[I - 1], Cur[I + 1], Prev[I], NormF(Row, globalX(I)));
    Next.front() = LeftNeighborExists ? cross(LeftNeighbor, Cur[1], Prev[0],
                                              NormF(Row, globalX(0)))
                                      : Psi(Row + 1);
    Next.back() = RightNeighborExists
                      ? cross(penultimate(Cur), RightNeighbor, Prev.back(),
                              NormF(Row, globalX(SegmentSz - 1)))
                      : leftAngle(penultimate(Cur), Cur.back(),
                                  NormF(Row, globalX(SegmentSz - 1)));

    /* Circular shift of buffers */
    std::swap(Prev, Cur);
    std::swap(Cur, Next);

    /* Exchange neighbors (for every row except last one) */
    if (Row != K - 1)
      doMsgExchange();
    // if (Verbose) {
    //   std::cout << mpi::whoami << ": at row = " << Row << ""
    // }
  }

  // if (Verbose)
  //   std::cout << mpi::whoami << ": complete!!!" << std::endl;
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
  $ ./prog -X 1.0 -T 0.05 -M 400 -K 20 --file input.txt --data
  $ cat input.txt | ./prog -X 1.0 -T 0.05 -M 400 -K 20 --data
  $ cat input.txt | mpirun -n 4 ./prog -X 1.0 -T 0.05 -M 400 -K 20 --data
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

  auto DumpData = Op.add<Switch>("d", "data", "dump resulting array");
  auto DumpTime = Op.add<Switch>("t", "time", "dump computation time");
  auto Verbose = Op.add<Switch>("v", "verbose", "emit debug information");

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
        !(Phi.parse(PhiStr, "x") && Psi.parse(PsiStr, "t") &&
          F.parse(FStr, "t", "x"))) {
      std::cerr << "Fatal error: root was able to parse function expressions, "
                   "but this process failed to"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  mpi::Timer Tmr;
  auto Res = compute(X->value(), T->value(), M->value(), K->value(), Phi, Psi,
                     F, Verbose->value());
  if (DumpData->value() && Res)
    std::cout << util::join(Res.data()) << std::endl;
  if (DumpTime->value() && mpi::commRank() == 0)
    std::cout << std::fixed << std::setprecision(2)
              << Tmr.getElapsedTimeInSeconds() << "s" << std::endl;
  return 0;
} catch (popl::invalid_option &e) {
  emitUsageError(e.what());
}