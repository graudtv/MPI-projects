#include <catch2/catch_all.hpp>
#include "cxxmpi/cxxmpi.hpp"

TEST_CASE("WorkSplitterLinear::getRange()", "[Util]") {
  util::WorkSplitterLinear S{11, 4};
  CHECK(S.getRange(0).FirstIdx == 0);
  CHECK(S.getRange(0).LastIdx == 3);
  CHECK(S.getRange(1).FirstIdx == 3);
  CHECK(S.getRange(1).LastIdx == 6);
  CHECK(S.getRange(2).FirstIdx == 6);
  CHECK(S.getRange(2).LastIdx == 9);
  CHECK(S.getRange(3).FirstIdx == 9);
  CHECK(S.getRange(3).LastIdx == 11);
}

TEST_CASE("WorkSplitterLinear::getSizes()", "[Util]") {
  util::WorkSplitterLinear S{11, 4};
  auto Sizes = S.getSizes();
  CHECK(Sizes.size() == 4);
  int ExpectedSizes[] = {3, 3, 3, 2};
  CHECK(std::equal(Sizes.begin(), Sizes.end(), ExpectedSizes));
}

TEST_CASE("WorkSplitterLinear::getDisplacements()", "[Util]") {
  util::WorkSplitterLinear S{11, 4};
  auto Displs = S.getDisplacements();
  CHECK(Displs.size() == 4);
  int ExpectedDispls[] = {0, 3, 6, 9};
  CHECK(std::equal(Displs.begin(), Displs.end(), ExpectedDispls));
}