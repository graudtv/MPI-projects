#include "cxxmpi/cxxmpi.hpp"
#include "Support/Parsing.hpp"
#include "Support/OstreamHelpers.hpp"
#include <iostream>
#include <iomanip>
#include <boost/multiprecision/gmp.hpp>

namespace mpi = cxxmpi;
namespace mp = boost::multiprecision;

int getSeriesSize(int Precision) {
   return 2 * Precision;
}

#ifndef LINEAR

/* SeriesSize - число членов ряда */
void calculateGroup(int SeriesSize, unsigned int Precision) {
   int Rank = mpi::commRank();
   int CommSz = mpi::commSize();

   /* the 0-th element (i.e. 1) is handled separately */
   auto WorkRange = util::WorkSplitterLinear(SeriesSize - 1, CommSz).getRange(Rank).shift(1);
   assert(WorkRange.size() > 0 && "no work was given to this process");
   auto From = WorkRange.FirstIdx;
   auto To = WorkRange.LastIdx - 1;

   mp::mpz_int Nominator = 1;
   mp::mpz_int PrevItem = 1;
   for (; To > From; --To) {
      PrevItem *= To;
      Nominator += PrevItem;
   }
   mp::mpz_int Denominator = PrevItem * To;

   mp::mpf_float PartialRes{mp::mpq_rational{Nominator, Denominator}, Precision};
   mp::mpf_float DenomRes{mp::mpq_rational{1, Denominator}, Precision};

   mpi::send(PartialRes.str(), 0, /* tag = */ 0);
   mpi::send(DenomRes.str(), 0, /* tag = */ 1);
}

int calculateExp(int Precision) {
   mp::mpf_float::default_precision(Precision);
   calculateGroup(getSeriesSize(Precision), Precision);

   if (mpi::commRank() != 0) {
      // std::cerr << mpi::whoami << ": finished" << std::endl;
      return 0;
   }

   auto CommSz = mpi::commSize();
   std::vector<mp::mpf_float> Partials(CommSz);
   std::vector<mp::mpf_float> Denoms(CommSz);

   std::string Buf;
   for (int I = 0; I < CommSz * 2; ++I) {
      auto Status = mpi::recv(Buf);
      if (Status.tag() == 0) {
         Partials[Status.source()].assign(Buf);
      } else {
         assert(Status.tag() == 1);
         Denoms[Status.source()].assign(Buf);
      }
      Buf.clear();
   }

   /* reduce */
   mp::mpf_float Sum = 1;
   mp::mpf_float CurDenom = 1;
   for (int I = 0; I < CommSz; ++I) {
      Sum += Partials[I] * CurDenom;
      CurDenom *= Denoms[I];
   }
   std::cout << Sum.str(Precision) << std::endl;
   return 0;
}

#else // LINEAR

/* calculates exp without using any parallel routines */
int calculateExp(int Precision) {
   int SeriesSize = getSeriesSize(Precision);

   mp::mpf_float::default_precision(Precision);
   mp::mpf_float Sum = 1;
   mp::mpf_float Denom = 1;

   for (int I = 1; I <= SeriesSize; ++I) {
      Denom *= I;
      Sum += 1 / Denom;
   }
   std::cout << Sum.str(Precision) << std::endl;
   return 0;
}

#endif // LINEAR

void emitUsageError() {
   std::cerr << "Usage: ./prog PRECISION" << std::endl;
   exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
#ifndef LINEAR
   mpi::MPIContext Ctx{&argc, &argv};
#endif

   int Precision = 0;
   if (argc != 2 || !util::parseInt(argv[1], &Precision))
      emitUsageError();
   return calculateExp(Precision);
}