#pragma once

#include <iostream>
#include <cuda.h>

#ifdef _DEBUG
# define CHECK_CUDA_CALL(cerr, str) if(cerr != CUDA_SUCCESS)\
                                    {\
                                      std::cout << str << ": " << cerr << "\n";\
                                      assert(false);\
                                    }
#else
# define CHECK_CUDA_CALL(cerr, str) if(cerr != CUDA_SUCCESS)
                                  \ {
                                  \   std::cout << str << "\n";
                                  \   return false;
                                  \ }
#endif

namespace nv_helpers_cuda
{
  // General initialization call to pick the best CUDA Device
  inline CUdevice findCudaDeviceDRV()
  {
      CUdevice cuDevice;
      int devID = 0;

      // Otherwise pick the device with highest Gflops/s
      char name[100];
      devID = 0;
      CHECK_CUDA_CALL( cuDeviceGet(&cuDevice, devID)
                     , "Couldn't get the device");
      cuDeviceGetName(name, 100, cuDevice);
      printf("> Using CUDA Device [%d]: %s\n", devID, name);

      cuDeviceGet(&cuDevice, devID);

      return cuDevice;
  }
}