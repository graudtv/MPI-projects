#include <CL/opencl.hpp>
#include <popl.hpp>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>
#include <cassert>

/***** Begin OpenCL kernels *****/
std::string ProgramSource = R"(

typedef int data_t;
#define PRIVATE_BUF_SIZE 64u

#define assert(cond)                                                           \
  do {                                                                         \
    if (!(cond))                                                               \
      printf("Assertion failed: %s\n", #cond);                                 \
  } while (0)

/* Naming conventions
 *  buf, buf_sz     - the whole array to be sorted
 *  data, data_sz   - part of array processed by a single work group
 *  chunk, chunk_sz - part of data, which can be written to by a work item
 *
 * Work-items may only read from _data_ associated with work-item's work
 * group.
 * Work-items may only write to the _chunk_ associated with the work group.
 */

void bmerge_stage3(__global data_t *data, uint data_sz);
void bmerge_stage2(__global data_t *data, uint data_sz);
void bmerge_stage1(__global data_t *data, uint data_sz);

__kernel void sort_chunk(__global data_t *buf, uint buf_size, __local data_t *temp) {
  uint chunk_size = buf_size / get_global_size(0);
  __global data_t *chunk = buf + chunk_size * get_global_id(0);

  event_t ev = async_work_group_copy(temp, chunk, chunk_size, 0);
  wait_group_events(1, &ev);

  ev = async_work_group_copy(chunk, temp, chunk_size, 0);
  wait_group_events(1, &ev);
}

void bmerge_stage3(__global data_t *data, uint data_sz) {
  uint chunk_sz = data_sz / get_local_size(0);
  __global data_t *chunk = data + chunk_sz * get_local_id(0);

  for (uint i = chunk_sz / 2; i > 0; i /= 2)
    for (uint j = 0; j < chunk_sz; ++j) {
      uint l = i ^ j;
      if (l > j && chunk[j] > chunk[l]) {
        /* swap(chunk[j], chunk[l]) */
        data_t tmp = chunk[j];
        chunk[j] = chunk[l];
        chunk[l] = tmp;
      }
    }
}

void bmerge_stage2(__global data_t *data, uint data_sz) {
  uint chunk_sz = data_sz / get_local_size(0);
  __global data_t *chunk = data + chunk_sz * get_local_id(0);
  data_t local_data[PRIVATE_BUF_SIZE];

  uint repeat_count =
      (chunk_sz <= PRIVATE_BUF_SIZE) ? 1 : chunk_sz / PRIVATE_BUF_SIZE;

  for (uint i = data_sz / 2; i >= chunk_sz; i /= 2) {
    uint block_id = chunk_sz * get_local_id(0) / i;
    bool is_odd_block = block_id % 2;

    for (uint r = 0; r < repeat_count; ++r) {
      for (uint j = 0; j < min(chunk_sz, PRIVATE_BUF_SIZE); ++j) {
        uint self_idx = chunk_sz * get_local_id(0) + j;
        uint other_idx = i ^ self_idx;
        local_data[j] = is_odd_block  ? max(data[self_idx], data[other_idx])
                                      : min(data[self_idx], data[other_idx]);
      }
      /* Upload local_data[] to chunk[] */
      barrier(CLK_GLOBAL_MEM_FENCE);
      for (uint j = 0; j < chunk_sz; ++j)
        chunk[j] = local_data[j];
      barrier(CLK_GLOBAL_MEM_FENCE);
    }
  }
}

void bmerge_stage1(__global data_t *data, uint data_sz) {
  assert(get_local_size(0) >= 2 && "stage1 requires at least 2 work items in work group");

  uint chunk_sz = data_sz / get_local_size(0);
  __global data_t *chunk = data + chunk_sz * get_local_id(0);
  data_t local_data[PRIVATE_BUF_SIZE];

  bool dir = chunk_sz * get_local_id(0) < (data_sz / 2);
  uint repeat_count =
      (chunk_sz <= PRIVATE_BUF_SIZE) ? 1 : chunk_sz / PRIVATE_BUF_SIZE;

  for (uint r = 0; r < repeat_count; ++r) {
    for (uint j = 0; j < min(chunk_sz, PRIVATE_BUF_SIZE); ++j) {
      uint self_idx = chunk_sz * get_local_id(0) + r * PRIVATE_BUF_SIZE + j;
      uint other_idx = (data_sz - 1) ^ self_idx;
      local_data[j] = dir ? min(data[self_idx], data[other_idx])
                          : max(data[self_idx], data[other_idx]);
    }
    /* Upload local_data[] to chunk[] */
    barrier(CLK_GLOBAL_MEM_FENCE);
    for (uint j = 0; j < chunk_sz; ++j)
      chunk[r * PRIVATE_BUF_SIZE + j] = local_data[j];
    barrier(CLK_GLOBAL_MEM_FENCE);
  }
}

__kernel void bmerge(__global data_t *buf, uint buf_sz) {
  // printf("> thread local_id %u local_size %u group_id %u global_id %u\n",
  //       get_local_id(0), get_local_size(0), get_group_id(0), get_global_id(0));

  /* Split buf among work groups */
  uint data_sz = buf_sz / get_num_groups(0);
  __global data_t *data = buf + data_sz * get_group_id(0);

  bmerge_stage1(data, data_sz);
  bmerge_stage2(data, data_sz);
  bmerge_stage3(data, data_sz);
}

)";
/***** End OpenCL kernels *****/

std::vector<int> makeRandomArray(size_t Size) {
  std::vector<int> Arr(Size);
  std::default_random_engine Gen;
  std::uniform_int_distribution<int> Distrib(-100, 100);

  std::generate(Arr.begin(), Arr.end(), [&]() -> int { return Distrib(Gen); });
  return Arr;
}

template <class T> bool isAscending(const std::vector<T> &Arr) {
  for (size_t I = 0; I + 1 < Arr.size(); ++I)
    if (Arr[I + 1] < Arr[I])
      return false;
  return true;
}

template <class T> bool isDescending(const std::vector<T> &Arr) {
  for (size_t I = 0; I + 1 < Arr.size(); ++I)
    if (Arr[I + 1] > Arr[I])
      return false;
  return true;
}

template <class T> struct join_arr {
  const T *Data;
  size_t Size;

  join_arr(const std::vector<T> &V) : Data(V.data()), Size(V.size()) {}

  friend std::ostream &operator<<(std::ostream &Os, const join_arr &A) {
    if (A.Size)
      Os << A.Data[0];
    for (size_t I = 1; I < A.Size; ++I)
      Os << ' ' << A.Data[I];
    return Os;
  }
};

template <class ClockT = std::chrono::high_resolution_clock> class Timer {
  using TimePoint = typename ClockT::time_point;
  TimePoint Start;

public:
  Timer() : Start(ClockT::now()) {}

  template <class T> auto getElapsedTime() const {
    return std::chrono::duration_cast<T>(ClockT::now() - Start);
  }

  double getElapsedSeconds() const {
    return getElapsedTime<std::chrono::microseconds>().count() / 1000000.0;
  }
  double getElapsedMilliseconds() const {
    return getElapsedTime<std::chrono::microseconds>().count() / 1000.0;
  }
};

/* https://stackoverflow.com/a/108340/16620735 */
bool isPowerOfTwo(size_t X) { return X > 0 && !(X & (X - 1)); }

popl::OptionParser Op("Options");

void emitUsageError(const char *msg) {
  std::cerr << "Error: " << msg << std::endl
            << std::endl
            << "Usage: ./prog [OPTIONS]" << std::endl
            << Op;
  exit(1);
}

class BitonicSorter {
  cl::Device Dev;
  cl::Context Ctx;
  cl::CommandQueue Queue;
  cl::Program Prog;
  cl::Kernel BitonicMerge;
  cl::Kernel SortChunk;
  bool Verbose = false;

public:
  explicit BitonicSorter(cl::Device D = cl::Device::getDefault()) : Dev(D) {
    cl_int Err = CL_SUCCESS;
    Ctx = cl::Context{Dev};
    Queue = cl::CommandQueue{Ctx};
    Prog = cl::Program{Ctx, ProgramSource};
    if (Prog.build() != CL_SUCCESS) {
      std::cerr << "Error: failed to compile kernels" << std::endl;
      std::cerr << Prog.getBuildInfo<CL_PROGRAM_BUILD_LOG>(Dev) << std::endl;
      exit(1);
    }
    BitonicMerge = cl::Kernel{Prog, "bmerge", &Err};
    assert(!Err && "failed to create kernel");
    SortChunk = cl::Kernel{Prog, "sort_chunk", &Err};
  }
  void setVerbose(bool V = true) { Verbose = V; }

  void sort(std::vector<int> &Arr) {
    assert(isPowerOfTwo(Arr.size()) && "Only POT sizes are supported");
    /* Sort small arrays on host */
    if (Arr.size() <= 16) {
      if (Verbose)
        std::cout << "bsort: small array, will sort on host" << std::endl;
      std::sort(Arr.begin(), Arr.end());
      return;
    }
    cl_int Err = CL_SUCCESS;
    size_t MaxWorkGroupSize =
        BitonicMerge.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(Dev);
    size_t ChunkSize = std::max(16ul, Arr.size() / MaxWorkGroupSize);
    size_t GlobalWorkSize = Arr.size() / ChunkSize;
    assert(Arr.size() > ChunkSize && "At least two work items required");

    for (size_t I = 0; I < Arr.size() / ChunkSize; ++I)
      std::sort(Arr.data() + I * ChunkSize,
                Arr.data() + I * ChunkSize + ChunkSize);

    cl::Buffer In{Ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                  Arr.size() * sizeof(int), Arr.data()};
    SortChunk.setArg(0, In);
    SortChunk.setArg(1, static_cast<cl_uint>(Arr.size()));
    SortChunk.setArg(2, static_cast<cl_uint>(Arr.size()));

    BitonicMerge.setArg(0, In);
    BitonicMerge.setArg(1, static_cast<cl_uint>(Arr.size()));

    Queue.enqueueNDRangeKernel(
        SortChunk, cl::NDRange(), cl::NDRange(GlobalWorkSize),
        cl::NDRange(std::min(GlobalWorkSize, MaxWorkGroupSize)));

    for (size_t LocalWorkSize = 2; LocalWorkSize <= GlobalWorkSize;
         LocalWorkSize *= 2) {
      Timer Tmr;
      Err = Queue.enqueueNDRangeKernel(BitonicMerge, cl::NDRange(),
                                       cl::NDRange(GlobalWorkSize),
                                       cl::NDRange(LocalWorkSize));
      if (Verbose) {
        Queue.finish();
        std::cout << "bsort: merge " << GlobalWorkSize / LocalWorkSize
                  << " work groups x " << LocalWorkSize << " work items x "
                  << ChunkSize << " data elements -- "
                  << Tmr.getElapsedMilliseconds() << "ms" << std::endl;
      }
      assert(!Err && "failed to enqueue kernel");
    }
    Queue.finish();
    Err = Queue.enqueueReadBuffer(In, true, 0, Arr.size() * sizeof(int),
                                  Arr.data());
    assert(!Err && "failed to read from buf");
  }
};

void sortOnHost(std::vector<int> &Arr) { std::sort(Arr.begin(), Arr.end()); }

size_t parseSize(std::string Str) {
  assert(!Str.empty());

  size_t Factor = 1;
  if (Str.back() == 'M') {
    Factor = 1 << 20;
    Str.pop_back();
  } else if (Str.back() == 'K') {
    Factor = 1 << 10;
    Str.pop_back();
  }
  auto Sz = std::stoll(Str);
  if (Sz < 0)
    throw std::runtime_error("invalid size");
  return Sz * Factor;
}

#ifndef BENCH
int main(int argc, char *argv[]) try {
  using popl::Attribute;
  using popl::Switch;
  using popl::Value;

  auto OptArraySize = Op.add<Value<std::string>, Attribute::required>(
      "n", "", "array size. E.g 512, 4K, 1M");
  auto OptRunOnHost = Op.add<Switch>("", "host", "run std::sort() on host");
  auto OptDumpArrays = Op.add<Switch>("", "data", "dump array data");
  auto OptVerbose = Op.add<Switch>("v", "verbose", "print more information");
  Op.parse(argc, argv);

  if (Op.non_option_args().size() != 0) {
    emitUsageError(
        ("unexpected argument " + Op.non_option_args().front()).c_str());
  }
  if (Op.unknown_options().size() != 0)
    emitUsageError(("unknown option " + Op.unknown_options().front()).c_str());

  size_t ArraySize = parseSize(OptArraySize->value());
  if (!isPowerOfTwo(ArraySize))
    emitUsageError("array size must be power of two");
  auto Arr = makeRandomArray(ArraySize);

  if (OptVerbose->value())
    std::cout << "Array size: " << Arr.size() << std::endl;
  if (OptDumpArrays->value())
    std::cout << "Array data: " << join_arr(Arr) << std::endl;

  Timer Tmr;

  if (OptRunOnHost->value()) {
    if (OptVerbose->value())
      std::cout << "Running std::sort() on host" << std::endl;
    sortOnHost(Arr);
  } else {
    cl::Device Dev = cl::Device::getDefault();
    if (OptVerbose->value())
      std::cout << "Selected device: " << Dev.getInfo<CL_DEVICE_NAME>()
                << std::endl;
    BitonicSorter Sorter{Dev};
    Sorter.setVerbose(OptVerbose->value());
    Sorter.sort(Arr);
  }
  if (OptDumpArrays->value())
    std::cout << "Array data: " << join_arr(Arr) << std::endl;

  std::cout << std::setprecision(2) << std::fixed;
  std::cout << "Elapsed time: " << Tmr.getElapsedSeconds() << "s" << std::endl;

  if (!isAscending(Arr)) {
    std::cout << "Test: FAIL" << std::endl;
    exit(1);
  }
  std::cout << "Test: OK" << std::endl;
  return 0;
} catch (popl::invalid_option &e) {
  emitUsageError(e.what());
}
#else // BENCH

#include <benchmark/benchmark.h>

static void BM_HostQSort(benchmark::State &State) {
  auto Arr = makeRandomArray(State.range(0));
  std::default_random_engine Gen;
  for (auto _ : State) {
    State.PauseTiming();
    std::shuffle(Arr.begin(), Arr.end(), Gen);
    State.ResumeTiming();
    sortOnHost(Arr);
  }
  State.SetComplexityN(State.range(0));
  State.SetItemsProcessed(State.range(0) * State.iterations());
}
BENCHMARK(BM_HostQSort)
    ->Name("host_qsort")
    ->RangeMultiplier(2)
    ->Range(1 << 8, 1 << 22)
    ->Complexity();

static void BM_DeviceBitonic(benchmark::State &State) {
  auto Arr = makeRandomArray(State.range(0));
  std::default_random_engine Gen;
  BitonicSorter Sorter(cl::Device::getDefault());
  for (auto _ : State) {
    State.PauseTiming();
    std::shuffle(Arr.begin(), Arr.end(), Gen);
    State.ResumeTiming();

    auto Start = std::chrono::high_resolution_clock::now();
    Sorter.sort(Arr);
    auto End = std::chrono::high_resolution_clock::now();
    auto ElapsedSeconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(End - Start);
    State.SetIterationTime(ElapsedSeconds.count());
  }
  State.SetComplexityN(State.range(0));
  State.SetItemsProcessed(State.range(0) * State.iterations());
}
BENCHMARK(BM_DeviceBitonic)
    ->Name("device_bitonic")
    ->RangeMultiplier(2)
    ->Range(1 << 8, 1 << 20)
    // ->UseManualTime()
    ->Unit(benchmark::kMillisecond)
    ->Complexity();

BENCHMARK_MAIN();

#endif // BENCH
