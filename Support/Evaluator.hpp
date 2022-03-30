#include "OstreamHelpers.hpp"
#include <matheval.h>
#include <sstream>
#include <iostream>

namespace util {

/* Evaluates function of one variable parsed from string */
class Evaluator {
public:
  /* Creates uninitialize Evaluator
   * Evaluator can be initialized via parse() method */
  Evaluator() = default;

  Evaluator(const Evaluator &Other) = delete;
  Evaluator &operator=(const Evaluator &Other) = delete;

  ~Evaluator() { destroy(); }

  /* Check if Evaluator has successfully parsed function and
   * is able to evaluate it */
  bool isValid() const { return F; }

  const char *getTmpString() const {
    assert(isValid() && "Evaluator is not initialized");
    return evaluator_get_string(F);
  }

  std::string getErrorStr() const { return Err.str(); }

  /* Returns true on success.
   * On error, getErrorStr() will return error description */
  bool parse(const std::string &Expr, const std::string &ExpectedParamName) {
    destroy(); // release previous state if was initialized
    bool Success = tryParse(Expr, ExpectedParamName);
    if (!Success)
      destroy();
    return Success;
  }

  /* evaluate function */
  double operator()(double Arg) {
    assert(isValid() && "Evaluator is not initialized");
    return evaluator_evaluate(F, /* ParamCount = */ 1, ParamNames, &Arg);
  }

private:
  bool IsValid = false;
  void *F = nullptr;
  char **ParamNames = nullptr;
  std::ostringstream Err;

  void destroy() {
    if (F) {
      evaluator_destroy(F);
      F = nullptr;
    }
  }

  bool tryParse(const std::string &Expr, const std::string &ExpectedParamName) {
    if (!(F = evaluator_create(const_cast<char *>(Expr.c_str())))) {
      Err << "failed to parse expression";
      return false;
    }
    int ParamCount = 0;
    evaluator_get_variables(F, &ParamNames, &ParamCount);
    if (ParamCount > 1) {
      Err << "expected function of one variable '" << ExpectedParamName
          << "', but parsed function seems to be a function of " << ParamCount
          << " variables: "
          << util::join(ParamNames, ParamNames + ParamCount, ", ");
      return false;
    }
    /* parsed function may not contain any params at all, it is considered
     * valid. So param name is only checked if function do contains param */
    if (ParamCount != 0 && ParamNames[0] != ExpectedParamName) {
      Err << "expected function of one variable '" << ExpectedParamName
          << "', but parsed function of one variable '" << ParamNames[0] << "'";
      return false;
    }
    return true;
  }
};

inline
std::ostream &operator <<(std::ostream &Os, const Evaluator &E) {
  return Os << E.getTmpString();
}


} // namespace util