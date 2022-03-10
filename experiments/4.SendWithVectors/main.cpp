#include "cxxmpi/cxxmpi.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>

namespace mpi = cxxmpi;

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};

  if (mpi::commSize() != 2) {
    std::cerr << "2 processes expected" << std::endl;
    exit(EXIT_FAILURE);
  }

  double ScalarToSend = 228;
  float ArrayToSend[] = {11, 22, 33, 44, 55, 66, 77, 88};
  std::array<unsigned char, 5> StdArrayToSend = {5, 4, 3, 2, 1};
  std::vector<int> VecToSend = {10, 20, 30, 40, 50, 60, 70};
  std::string StrToSend = "Hello, world!";

  if (mpi::commRank() == 0) {
    mpi::send(ScalarToSend, 1);
    mpi::send(ArrayToSend, 1);
    mpi::send(StdArrayToSend, 1);
    mpi::send(VecToSend, 1);
    mpi::send(StrToSend, 1);
  } else { // rank == 1
    double Scalar;
#ifdef BUG
    std::vector<double> Array;
#else
    std::vector<float> Array;
#endif
    std::vector<unsigned char> StdArray;
    std::vector<int> Vec = {1, 2}; // checking that append mechanism works
    std::string Str;

    mpi::recv(Scalar, 0);
    mpi::recv(Array, 0);
    mpi::recv(StdArray, 0);

    auto Status = mpi::recv(Vec, 0);
    assert(Status.getCount() == VecToSend.size() &&
           Status.getCount() == Vec.size() - 2);
    Vec.erase(Vec.begin(), std::next(Vec.begin(), 2));

    mpi::recv(Str, 0);

    if (!(Scalar == ScalarToSend &&
        std::equal(Array.begin(), Array.end(), ArrayToSend) &&
        std::equal(StdArray.begin(), StdArray.end(), StdArrayToSend.begin()) &&
        std::equal(Vec.begin(), Vec.end(), VecToSend.begin()) &&
        Str == StrToSend
        )) {
      std::cout << "Data corruption occured!!!" << std::endl;
      exit(EXIT_FAILURE);
    }
    std::cout << "Everything is correct" << std::endl;
  }
  return 0;
}