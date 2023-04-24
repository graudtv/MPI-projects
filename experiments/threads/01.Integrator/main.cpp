#include "Support/Evaluator.hpp"
#include "external/popl/include/popl.hpp"

#include <atomic>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <semaphore>
#include <stack>
#include <thread>

namespace detail {

template <class Function>
double adaptiveRecursiveImpl(const Function &F, double A, double B, double Eps,
                             double Fa, double Fb) {
  auto C = (A + B) / 2;
  auto Fc = F(C);
  auto Sab = (Fa + Fb) * (B - A) / 2;
  auto Sacb = (Fa + Fc) * (C - A) / 2 + (Fc + Fb) * (B - C) / 2;
  if (abs(Sacb - Sab) <= abs(Sacb) * Eps)
    return Sacb;
  return adaptiveRecursiveImpl(F, A, C, Eps, Fa, Fc) +
         adaptiveRecursiveImpl(F, C, B, Eps, Fc, Fb);
}

} // namespace detail

template <class Function>
double adaptiveRecursive(const Function &F, double From, double To,
                         double Eps) {
  return detail::adaptiveRecursiveImpl(F, From, To, Eps, F(From), F(To));
}

template <class T> using Stack = std::stack<T, std::vector<T>>;

template <class Function>
double adaptiveNonRecursive(const Function &F, double From, double To,
                            double Eps) {
  Stack<std::tuple<double, double, double, double>> Stk;
  double A = From, B = To, Fa = F(From), Fb = F(To);
  double Res = 0;

  std::cout << std::fixed << std::setprecision(20);
  int i = 0;
  while (1) {
    auto C = (A + B) / 2;
    auto Fc = F(C);
    auto Sab = (Fa + Fb) * (B - A) / 2;
    auto Sacb = (Fa + Fc) * (C - A) / 2 + (Fc + Fb) * (B - C) / 2;

    if (abs(Sacb - Sab) <= abs(Sacb) * Eps) {
      Res += Sacb;
      if (Stk.empty())
        return Res;
      std::tie(A, B, Fa, Fb) = Stk.top();
      Stk.pop();
      continue;
    }
    Stk.emplace(A, C, Fa, Fc);
    A = C;
    Fa = Fc;
  }
  return Res;
}

struct StackData {
  double A, B, Fa, Fb;
  StackData() = default;
  StackData(double A, double B, double Fa, double Fb)
      : A(A), B(B), Fa(Fa), Fb(Fb) {}
};

#if 0
#define DBG(output)                                                            \
  {                                                                            \
    std::lock_guard Lock{StdioAccess};                                         \
    std::cout << std::this_thread::get_id() << ": " << output << std::endl;    \
  }
#else
#define DBG(output)
#endif

thread_local unsigned IterationCount = 0;
constexpr unsigned IterationSpan = 0x400;

template <class Function> class ParallelIntegrator {
  const Function *F;
  int NumThreads;
  double Eps;

  std::mutex GlobalAccess;
  std::stack<StackData> GlobalStack;
  std::condition_variable GlobalUpdate;
  int ActiveCount = 0;
  int FinishedCount = 0;

  std::mutex StdioAccess;

  std::mutex ResultAccess;
  double Result = 0;

  /* Returns true if task was acquired and completed, false if there is
   * no more tasks */
  bool waitTask(double &Res) {
    std::unique_lock<std::mutex> Lock{GlobalAccess};
    --ActiveCount;
    if (!ActiveCount && GlobalStack.empty()) { // computation completed
      DBG("computation completion detected");
      ++FinishedCount;
      Lock.unlock();
      /* Notify other threads about completion (spinlock) */
      while (*static_cast<volatile int *>(&FinishedCount) != NumThreads)
        GlobalUpdate.notify_all();
      return false;
    }
    while (GlobalStack.empty()) {
      GlobalUpdate.wait(Lock);
      if (FinishedCount) {
        DBG("computation completion signal received");
        ++FinishedCount;
        return false;
      }
    }
    auto Task = GlobalStack.top();
    GlobalStack.pop();
    ++ActiveCount;
    Lock.unlock();

    DBG("starting [" << Task.A << "; " << Task.B << "]");
    Res += doTask(Task.A, Task.B, Task.Fa, Task.Fb);
    DBG("finished [" << Task.A << "; " << Task.B << "]");
    return true;
  }

  double doTask(double A, double B, double Fa, double Fb) {
    auto C = (A + B) / 2;
    auto Fc = (*F)(C);
    auto Sab = (Fa + Fb) * (B - A) / 2;
    auto Sacb = (Fa + Fc) * (C - A) / 2 + (Fc + Fb) * (B - C) / 2;
    if (abs(Sacb - Sab) <= abs(Sacb) * Eps)
      return Sacb;
    if ((++IterationCount % IterationSpan == 0) && GlobalStack.empty() &&
        GlobalAccess.try_lock()) {
      DBG("offloading [" << A << "; " << C << "]");
      /* share lhs task */
      GlobalStack.emplace(A, C, Fa, Fc);
      GlobalAccess.unlock();
      GlobalUpdate.notify_one();
      /* continue working on rhs task */
      return doTask(C, B, Fc, Fb);
    }
    return doTask(A, C, Fa, Fc) + doTask(C, B, Fc, Fb);
  }

  void threadEntryPoint(double From, double To) {
    DBG("initial task [" << From << "; " << To << "]");
    auto Res = doTask(From, To, (*F)(From), (*F)(To));
    DBG("initial task ready [" << From << "; " << To << "] result " << Res);
    while (waitTask(Res))
      ;
    DBG("thread completed, result " << Res);
    std::lock_guard Lock{ResultAccess};
    Result += Res;
  }

  static void ThreadEntryPoint(ParallelIntegrator *PI, double From, double To) {
    PI->threadEntryPoint(From, To);
  }

public:
  double integrate(const Function &Func, double From, double To, double EpsVal,
                   int ThreadCount) {
    F = &Func;
    NumThreads = ThreadCount;
    ActiveCount = ThreadCount;
    Eps = EpsVal;

    std::vector<std::thread> Threads;
    double Step = (To - From) / ThreadCount;
    for (int I = 0; I < NumThreads; ++I)
      Threads.emplace_back(ThreadEntryPoint, this, From + Step * I,
                           From + Step * I + Step);
    std::for_each(Threads.begin(), Threads.end(), [](auto &&T) { T.join(); });
    return Result;
  }
};

popl::OptionParser Op("Options");

void emitUsageError(const char *Msg) {
  std::cerr << "Error: " << Msg << std::endl
            << std::endl
            << "Usage: ./integrate [OPTIONS]" << std::endl
            << Op <<
#ifndef EXPR
      R"(
Examples:
  ./integrate --expr 'x ^ 2' --from 0 --to 3
  ./integrate --expr 'sin(x)' --from 0 --to 1 --eps 0.001 --parallel 8
)";
#else
      R"(
Examples:
  ./integrate --from 0 --to 3
  ./integrate --from 0 --to 1 --eps 0.001 --parallel 8
)";
#endif
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) try {
  using popl::Attribute;
  using popl::Switch;
  using popl::Value;

  auto From = Op.add<Value<double>, Attribute::required>(
      "f", "from", "left integration boundary");
  auto To = Op.add<Value<double>, Attribute::required>(
      "t", "to", "right integration boundary");
#ifndef EXPR
  auto Expr = Op.add<Value<std::string>, Attribute::required>(
      "e", "expr", "function to be integrated");
#endif
  auto Eps =
      Op.add<Value<double>>("", "eps", "computation precision", 0.000001);
  auto LinearRecursive =
      Op.add<Switch>("", "linear-recursive",
                     "run linear recursive algorithm instead of parallel one");
  auto LinearNonRecursive = Op.add<Switch>(
      "", "linear-nonrecursive",
      "run linear non recursive algorithm instead of parallel one");
  auto Parallel = Op.add<Value<int>>(
      "", "parallel", "specify number of threads for parallel algorithm", 4);

  Op.parse(argc, argv);
  if (Op.non_option_args().size() >= 1)
    emitUsageError(("unexpected argument '" + Op.non_option_args().front() + "'").c_str());
  if (Op.unknown_options().size() != 0)
    emitUsageError(("unknown option '" + Op.unknown_options().front() + "'").c_str());
  if (From->value() >= To->value()) {
    std::cerr << "Error: 'from' value must be less than 'to'" << std::endl;
    return EXIT_FAILURE;
  }

#ifndef EXPR
  util::Evaluator F;
  if (!F.parse(Expr->value(), "x")) {
    std::cerr << "Error: " << F.getErrorStr() << std::endl;
    return EXIT_FAILURE;
  }
#else
  auto F = [](double x) -> double { return EXPR; };
#endif

  if (LinearRecursive->value()) {
    std::cout << adaptiveRecursive(F, From->value(), To->value(), Eps->value())
              << std::endl;
    return EXIT_SUCCESS;
  }
  if (LinearNonRecursive->value()) {
    std::cout << adaptiveNonRecursive(F, From->value(), To->value(),
                                      Eps->value())
              << std::endl;
    return EXIT_SUCCESS;
  }

  auto NumThreads = Parallel->value();
  if (NumThreads <= 0) {
    std::cerr << "Error: invalid number of threads specified" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << ParallelIntegrator<decltype(F)>{}.integrate(
                   F, From->value(), To->value(), Eps->value(), NumThreads)
            << std::endl;
#if 0
  auto Func = [](double x) { return std::sin(1 / x); };
  std::cout << ParallelIntegrator<decltype(Func)>{}.integrate(
                   Func, From->value(), To->value(), Eps->value(), NumThreads)
            << std::endl;
#elif 0
  extern double f(double x);
  std::cout << ParallelIntegrator<decltype(f)>{}.integrate(
                   f, From->value(), To->value(), Eps->value(), NumThreads)
            << std::endl;
#endif
  return EXIT_SUCCESS;
} catch (popl::invalid_option &Err) {
  emitUsageError(Err.what());
}
