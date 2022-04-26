/* C++ wrapper for libmatheval */

#include "OstreamHelpers.hpp"
#include <array>
#include <iostream>
#include <matheval.h>
#include <sstream>

namespace util {

class EvaluatorBase {
public:
  EvaluatorBase() = default;
  EvaluatorBase(const EvaluatorBase &Other) = delete;
  EvaluatorBase &operator=(const EvaluatorBase &Other) = delete;
  virtual ~EvaluatorBase() { releaseEvaluator(); }

  /* Check if Evaluator has successfully parsed function and
   * is able to evaluate it */
  bool isValid() const { return F; }

  const char *getTmpString() const {
    assert(isValid() && "Evaluator is not initialized");
    return evaluator_get_string(F);
  }

  std::string toString() const { return std::string{getTmpString()}; }
  std::string getErrorStr() const { return Err.str(); }

protected:
  void *F = nullptr;
  std::ostringstream Err;

  /* release underlying evaluator object */
  void releaseEvaluator() {
    if (F) {
      evaluator_destroy(F);
      F = nullptr;
    }
  }

  void clearErrors() { Err.str(""); }

  bool createEvaluator(const std::string &Expr) {
    releaseEvaluator(); // release previous state if already initialized
    clearErrors();
    if (!(F = evaluator_create(const_cast<char *>(Expr.c_str())))) {
      Err << "failed to parse expression";
      return false;
    }
    return true;
  }
};

template <size_t N> class EvaluatorND : public EvaluatorBase {
public:
  /* Tries to parse an expression as a function of ExpectedParamNames
   * If error occures, false is returned and explanation can be obtained
   * with getErrorStr() method. On success, true is returned
   *
   * Warning! If ExpectedParamNames contains two params with the
   * same name, behavior is undefined!
   */
  template <class... StringTs>
  bool parse(const std::string &Expr, StringTs &&...ExpectedParamNames) {
    static_assert(
        sizeof...(ExpectedParamNames) == N,
        "EvaluatorND::parse() was called with wrong number of arguments");
    if (!createEvaluator(Expr))
      return false;
    if (!parseImpl(std::array<std::string, N>{ExpectedParamNames...})) {
      releaseEvaluator();
      return false;
    }
    return true;
  }

  /* Evaluates expression */
  template <class... ConvertibleToDoubleTs>
  double operator()(ConvertibleToDoubleTs &&...Args) const {
    static_assert(
        sizeof...(Args) == N,
        "EvaluatorND::operator()() was called with wrong number of arguments");
    assert(isValid() && "Evaluator was not initialized");

    std::array<double, N> InputValues{static_cast<double>(Args)...};
    return evaluator_evaluate(F, N, const_cast<char **>(InputParamNames.data()),
                              InputValues.data());
  }

private:
  std::array<char *, N> InputParamNames;

  bool parseImpl(std::array<std::string, N> ExpectedParamNames) {
    char **ActualParamNames;
    int ActualParamCount;
    evaluator_get_variables(F, &ActualParamNames, &ActualParamCount);

    /* Parsed function may not contain some of params we expect.
     * (for example f(x, y) = x is a valid 2D function)
     * But if parsed contains unexpected params, it's definitely
     * an error */
    if (ActualParamCount > N) {
      explainWhatWasExpected(ExpectedParamNames);
      Err << ", but parsed function seems to be a function of ";
      if (ActualParamCount == 1) {
        Err << "one variable " << ActualParamNames[0];
      } else {
        Err << ActualParamCount << " variables: "
            << util::join(ActualParamNames, ActualParamNames + ActualParamCount,
                          ", ");
      }
      return false;
    }

    std::fill(InputParamNames.begin(), InputParamNames.end(), nullptr);

    /* Now we need to check that parsed function doesn't contain
     * any unexpected params and fill InputParamNames, which
     * will be later passed to evaluator_evaluate() */
    for (size_t I = 0; I < ActualParamCount; ++I) {
      char *ActualParamName = ActualParamNames[I];
      auto It = std::find(ExpectedParamNames.begin(), ExpectedParamNames.end(),
                          ActualParamName);
      if (It == ExpectedParamNames.end()) {
        explainWhatWasExpected(ExpectedParamNames);
        Err << ", but parsed function contains unknown variable "
            << ActualParamName;
        return false;
      }
      size_t Offset = std::distance(ExpectedParamNames.begin(), It);
      assert(InputParamNames[Offset] == nullptr &&
             "this is bug in lib matheval");
      InputParamNames[Offset] = ActualParamName;
    }
    /* Now fill unused variables in InputParamNames with empty strings */
    std::transform(
        InputParamNames.begin(), InputParamNames.end(), InputParamNames.begin(),
        [](char *Str) { return Str ? Str : const_cast<char *>(""); });
    return true;
  }

  void
  explainWhatWasExpected(const std::array<std::string, N> &ExpectedParamNames) {
    Err << "expected ";
    if (N == 0) {
      Err << "constant expression";
    } else if (N == 1) {
      Err << "function of one variable " << ExpectedParamNames[0];
    } else {
      Err << "function of " << N << " variables: "
          << util::join(ExpectedParamNames.begin(), ExpectedParamNames.end(),
                        ", ");
    }
  }
};

inline std::ostream &operator<<(std::ostream &Os, const EvaluatorBase &E) {
  return Os << E.getTmpString();
}

using Evaluator = EvaluatorND<1>;
using Evaluator2D = EvaluatorND<2>;

} // namespace util