#pragma once

#include <cassert>

namespace util {

/* Represents integer range [FirstIdx; LastIdx)
 * Note that LastIdx is not included into the range
 */
struct WorkRangeLinear {
  int FirstIdx;
  int LastIdx;

  WorkRangeLinear(int First, int Last) : FirstIdx(First), LastIdx(Last) {
    assert(FirstIdx >= 0);
    assert(LastIdx >= 0);
  }

  int size() const { return LastIdx - FirstIdx; }
  WorkRangeLinear shift(int Offset) const { return WorkRangeLinear{FirstIdx + Offset, LastIdx + Offset}; }
};

/* Example
 * Suppose we are splitting 11 work items to 4 workers
 * Then workers will have work ranges [0, 3), [3, 6), [6, 9), [9, 11),
 * i.e. work sizes are 3, 3, 3, 2
 */
class WorkSplitterLinear {
public:
  WorkSplitterLinear(int WorkSz, int NumWorkers) : WorkSz(WorkSz), NumWorkers(NumWorkers) {}

  WorkRangeLinear getRange(int WorkerId) {
    int DefaultGroupSz = WorkSz / NumWorkers;

    if (WorkerId < WorkSz % NumWorkers) {
      int FirstIdx = WorkerId * (DefaultGroupSz + 1);
      int LastIdx = FirstIdx + (DefaultGroupSz + 1);
      return WorkRangeLinear{FirstIdx, LastIdx};
    } 

    int NumOfEnlargedGroups = WorkSz % NumWorkers;
    int FirstIdx = NumOfEnlargedGroups * (DefaultGroupSz + 1) +
      (WorkerId - NumOfEnlargedGroups) * DefaultGroupSz;
    int LastIdx = FirstIdx + DefaultGroupSz;     
    return WorkRangeLinear{FirstIdx, LastIdx};
  }

private:
  int WorkSz;
  int NumWorkers;
};

} // namespace util