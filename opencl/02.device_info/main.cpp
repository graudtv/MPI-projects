#include <CL/opencl.hpp>
#include <iostream>
#include <vector>

const char *yesOrNo(bool Cond) { return Cond ? "yes" : "no"; }

std::string bytesToString(size_t Bytes) {
  if (Bytes & 1023)
    return std::to_string(Bytes) + "B";
  if ((Bytes >>= 12) & 1023)
    return std::to_string(Bytes) + "KB";
  if ((Bytes >>= 12) & 1023)
    return std::to_string(Bytes) + "MB";
  return std::to_string(Bytes >>= 12) + "GB";
}

int main() {
  std::vector<cl::Platform> Platforms;
  if (cl::Platform::get(&Platforms) != CL_SUCCESS) {
    std::cerr << "Error: failed to get list of platforms" << std::endl;
    exit(1);
  }
  std::cout << "Found " << Platforms.size() << " platform(s):" << std::endl;
  for (auto &&Plf : Platforms) {
    std::cout << "* Platform: " << Plf.getInfo<CL_PLATFORM_NAME>() << std::endl;
    std::cout << "  Vendor: " << Plf.getInfo<CL_PLATFORM_VENDOR>() << std::endl;
    std::cout << "  Version: " << Plf.getInfo<CL_PLATFORM_VERSION>() << std::endl;
    std::cout << "  Profile: " << Plf.getInfo<CL_PLATFORM_PROFILE>() << std::endl;
    std::cout << "  Extensions: " << Plf.getInfo<CL_PLATFORM_EXTENSIONS>() << std::endl;
    std::cout << "  Devices:" << std::endl;
    std::vector<cl::Device> Devices;
    if (Plf.getDevices(CL_DEVICE_TYPE_ALL, &Devices) != CL_SUCCESS) {
      std::cout << "    Error: failed to get list of devices" << std::endl;
      continue;
    }
    for (auto &&Dev : Devices) {
      std::cout << "  - Device: " << Dev.getInfo<CL_DEVICE_NAME>() << std::endl;
      std::cout << "    Available: " << yesOrNo(Dev.getInfo<CL_DEVICE_AVAILABLE>()) << std::endl;
      std::cout << "    Frequency: " << Dev.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << "MHz" << std::endl;
      std::cout << "    Compute units: " << Dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
      std::cout << "    Max 2D image size: " << Dev.getInfo<CL_DEVICE_IMAGE2D_MAX_WIDTH>()
                << " x " << Dev.getInfo<CL_DEVICE_IMAGE2D_MAX_HEIGHT>() << std::endl;
      std::cout << "    Max 3D image size: " << Dev.getInfo<CL_DEVICE_IMAGE3D_MAX_WIDTH>()
                << " x " << Dev.getInfo<CL_DEVICE_IMAGE3D_MAX_HEIGHT>() << std::endl;
      std::cout << "    Max memory allocation size: " << bytesToString(Dev.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>()) << std::endl;
      std::cout << "    Max work group size: " << Dev.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
      std::cout << "    Max work items in group: " << Dev.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()[0] << std::endl;
      std::cout << "    Native char vector width: " << Dev.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR>() << std::endl;
      std::cout << "    Native int vector width: " << Dev.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_INT>() << std::endl;
      std::cout << "    Compiler available: " << yesOrNo(Dev.getInfo<CL_DEVICE_COMPILER_AVAILABLE>()) << std::endl;
      std::cout << "    64-bit float-s: " << yesOrNo(Dev.getInfo<CL_DEVICE_DOUBLE_FP_CONFIG>() != 0) << std::endl;
      std::cout << "    Global memory size: " << bytesToString(Dev.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>()) << std::endl;
      std::cout << "    Local memory size: " << bytesToString(Dev.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>()) << std::endl;
      std::cout << "    Extensions: " << Dev.getInfo<CL_DEVICE_EXTENSIONS>() << std::endl;

    }
  }
  std::cout << std::endl;
  std::cout << "Default platform: " << cl::Platform::getDefault().getInfo<CL_PLATFORM_NAME>() << std::endl;
  std::cout << "Default device: " << cl::Device::getDefault().getInfo<CL_DEVICE_NAME>() << std::endl;
  return 0;
}
