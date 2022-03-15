/* Example of using custom (derived) types in MPI */

#include "cxxmpi/cxxmpi.hpp"
#include <iostream>
#include <cassert>

namespace mpi = cxxmpi;

/* Just a random struct we want to transmit via one message */
struct MyData {
  signed char C;
  float F;
  int I1;
  double I2;
};

bool operator==(const MyData &Fst, const MyData &Snd) {
  return Fst.C == Snd.C && Fst.F == Snd.F && Fst.I1 == Snd.I1 &&
         Fst.I2 == Snd.I2;
}

bool operator!=(const MyData &Fst, const MyData &Snd) { return !(Fst == Snd); }

/* number of fields in MyData */
#define NUMFIELDS 4

MPI_Datatype createDatatypeForMyData() {
  int BlockLengths[NUMFIELDS] = {1, 1, 1, 1};
  MPI_Aint Displacements[NUMFIELDS] = {offsetof(MyData, C), offsetof(MyData, F),
                                       offsetof(MyData, I1),
                                       offsetof(MyData, I2)};
  MPI_Datatype Types[NUMFIELDS] = {MPI_CHAR, MPI_FLOAT, MPI_INT, MPI_DOUBLE};
  MPI_Datatype Type = MPI_DATATYPE_NULL;
  (void)MPI_Type_create_struct(NUMFIELDS, BlockLengths, Displacements, Types, &Type);

#ifndef NDEBUG
  MPI_Aint Lb, Extent;
  int Size;
  (void)MPI_Type_get_extent(Type, &Lb, &Extent);
  (void)MPI_Type_size(Type, &Size);
  assert(Lb == 0);
  assert(Extent == sizeof(MyData));
#endif

  return Type;
}

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};

  if (mpi::commSize() != 2) {
    std::cerr << "2 processes expected" << std::endl;
    exit(EXIT_FAILURE);
  }

  MyData DataToSend{'!', 12.0, 3, 7};
  MPI_Datatype Type = createDatatypeForMyData();
  MPI_Type_commit(&Type);

  if (mpi::commRank() == 0) {
    MPI_Send(&DataToSend, 1, Type, 1, 0, MPI_COMM_WORLD);
  } else {
    MyData ReceivedData;
    MPI_Recv(&ReceivedData, 1, Type, 0, 0, MPI_COMM_WORLD, nullptr);
    if (ReceivedData != DataToSend) {
      std::cout << "Data corruption occured!!!" << std::endl;
      exit(EXIT_FAILURE);
    }
    std::cout << "Everything is correct" << std::endl;
  }
  /* valid, but unnecessary */
  //// MPI_Type_free(&Type);
  return 0;
}