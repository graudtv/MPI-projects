#include <CL/opencl.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <cassert>
#include <algorithm>

#define N 1024

/***** Begin OpenCL kernels *****/
std::string ProgramSource = R"(
__kernel void square(__global int *buf) {
  int i = get_local_id(0);
  buf[i] = buf[i] * buf[i];
  printf("running local_id %d group_id %d global_id %d\n", get_local_id(0), get_group_id(0), get_global_id(0));
}
)";
/***** End OpenCL kernels *****/

std::vector<int> makeRandomArray(size_t Size) {
  std::vector<int> Arr(Size);
  std::default_random_engine Gen;
  std::uniform_int_distribution<int> Distrib(INT_MIN, INT_MAX);

  std::generate(Arr.begin(), Arr.end(),
                [&]() -> int { return Distrib(Gen); });
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

template <class T> void print(const std::vector<T> &Arr) {
  std::cout << "[";
  if (!Arr.empty())
    std::cout << Arr.front();
  for (size_t I = 1; I < Arr.size(); ++I)
    std::cout << " " << Arr[I] << " ";
  std::cout << "]";
}

template <class T> struct join_arr {
  const T *Data;
  size_t Size;

  join_arr(const std::vector<T> &V) : Data(V.data()), Size(V.size()) {}

  friend std::ostream &operator <<(std::ostream &Os, const join_arr &A) {
    if (A.Size)
      Os << A.Data[0];
    for (size_t I = 1; I < A.Size; ++I)
      Os << ' ' << A.Data[I];
    return Os;
  }

};

int main() {
  cl::Device Dev = cl::Device::getDefault();
  std::cout << "Selected device: " << Dev.getInfo<CL_DEVICE_NAME>() << std::endl;
  cl::Context Ctx{Dev};
  cl::CommandQueue Queue{Ctx};
  cl_int Err;

  cl::Program Prog{Ctx, ProgramSource};
  if (Prog.build() != CL_SUCCESS) {
    std::cerr << "Error: failed to compile kernels" << std::endl;
    std::cerr << Prog.getBuildInfo<CL_PROGRAM_BUILD_LOG>(Dev) << std::endl;
    exit(1);
  }

  cl::Kernel Main{Prog, "square", &Err};
  assert(!Err && "failed to create kernel");

  std::vector<int> Arr = {3, 7, 4, 8, 6, 2, 1, 5};
  std::cout << "before: " << join_arr(Arr) << std::endl;
  cl::Buffer Buf{Ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, N * sizeof(int), Arr.data(), &Err};
  assert(!Err && "failed to create buf");

  Main.setArg(0, Buf);
  Queue.enqueueNDRangeKernel(Main, cl::NDRange(), cl::NDRange(8), cl::NDRange(4));

  Err = Queue.enqueueReadBuffer(Buf, true, 0, N * sizeof(int), Arr.data());
  assert(!Err && "failed to read from buf");
  std::cout << "after:  " << join_arr(Arr) << std::endl;
  return 0;
}
