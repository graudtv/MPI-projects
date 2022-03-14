#pragma once

#include <iostream>

namespace util {

namespace detail {

template <class Iterator> struct RangePrinter {
  Iterator Fst, Last;
  const char *JoinStr;
};

template <class Iterator>
std::ostream &operator<<(std::ostream &Os, const RangePrinter<Iterator> &P) {
  assert(P.JoinStr != nullptr && "invalid JoinStr pointer");
  auto Fst = P.Fst;
  auto Last = P.Last;

  if (Fst == Last)
    return Os;

  Os << *Fst;
  while (++Fst != Last)
    Os << P.JoinStr << *Fst;
  return Os;
}

/* In case we are wrapping rvalue, Printable is not a reference and
 * contains moved object
 * Overwise, Printable is lvalue ref to the object */
template <class Printable> struct WrapperPrinter {
  Printable Value;
  const char *LeftStr;
  const char *RightStr;
};

template <class Printable>
std::ostream &operator<<(std::ostream &Os, const WrapperPrinter<Printable> &P) {
  assert(P.LeftStr != nullptr && P.RightStr && "invalid wrapper strings");
  return Os << P.LeftStr << P.Value << P.RightStr;
}

} // namespace detail

/* join() usage examples:
 * std::cout << join(vec) << std::endl;
 * std::cout << join(vec, ", ") << std::endl;
 * std::cout << join(vec.begin(), vec.end()) << std::endl;
 */
template <class Iterator>
detail::RangePrinter<Iterator> join(Iterator Fst, Iterator Last,
                                    const char *JoinStr = " ") {
  return detail::RangePrinter<Iterator>{Fst, Last, JoinStr};
}

/* C++11 moment...
 * Deduced return types are a C++14 extension... */
template <class Range, class Iterator = typename Range::const_iterator>
detail::RangePrinter<Iterator> join(const Range &R, const char *JoinStr = " ") {
  return join(R.begin(), R.end(), JoinStr);
}

template <class Printable>
detail::WrapperPrinter<Printable> wrap(Printable &&P, const char *LeftStr,
                                       const char *RightStr) {
  return detail::WrapperPrinter<Printable>{std::forward<Printable>(P), LeftStr, RightStr};
}

template <class Printable>
detail::WrapperPrinter<Printable> wrap(Printable &&P, const char *Str) {
  return wrap(std::forward<Printable>(P), Str, Str);
}

template <class Printable>
detail::WrapperPrinter<Printable> wrapAngleBrackets(Printable &&P) {
  return wrap(std::forward<Printable>(P), "<", ">");
}

template <class Printable>
detail::WrapperPrinter<Printable> wrapSquareBrackets(Printable &&P) {
  return wrap(std::forward<Printable>(P), "[", "]");
}

template <class Printable>
detail::WrapperPrinter<Printable> wrapCurlyBrackets(Printable &&P) {
  return wrap(std::forward<Printable>(P), "{", "}");
}

template <class Printable>
detail::WrapperPrinter<Printable> wrapRoundBrackets(Printable &&P) {
  return wrap(std::forward<Printable>(P), "(", ")");
}

} // namespace util