#ifndef OPENCL_PROGRAM_HPP
#define OPENCL_PROGRAM_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Include this first to init GLEW first
#include "OpenGLProgram.hpp"
//
#include <OpenCL/opencl.h>

// Helper macros
#define CL_ERROR_CHECK(x)                                                 \
  if (cl_int result = (x)) {                                              \
    cout << "Encountered an OpenCL error(" << __FILE__ << ":" << __LINE__ \
         << "): " << result << endl;                                      \
    return 1;                                                             \
  }

#define CL_ERROR_RETURN(x)   \
  if (cl_int result = (x)) { \
    return result;           \
  }

class OpenCLProgram {
 public:
  static cl_int GetAvailablePlatforms(
      std::unordered_map<std::string, cl_platform_id> &platforms);

  static cl_int GetAvailableDevices(
      cl_platform_id platformId,
      std::unordered_map<std::string, cl_device_id> &devices);

  static cl_int IsExtensionSupported(const std::string &extension,
                                     cl_device_id device);

  static cl_int OpenGLSharingSupported(cl_device_id device);

  cl_int Init(cl_platform_id platform, cl_device_id device,
              const std::string &programSource,
              const std::unordered_map<std::string, std::string> &definitions,
              const std::vector<std::string> &includePaths,
              const std::unordered_map<cl_context_properties,
                                       cl_context_properties> &properties,
              std::string &errorLog);
  cl_int Unload();

  cl_int LoadKernel(const std::string &kernelName);

  cl_int CreateBuffer(cl_mem_flags flags, size_t bufSize, void *data,
                      cl_mem *newBuffer);

  cl_int ReleaseBuffer(cl_mem buf);

  cl_int SetArgument(const std::string &kernelName, cl_uint argNum,
                     size_t argSize, void *data);

  cl_int CreateBufferArgument(const std::string &kernelName, cl_uint argNum,
                              cl_mem_flags flags, size_t argSize, void *data,
                              cl_mem *newBuffer);

  cl_int MapBuffer(cl_mem buffer, cl_bool blocking, cl_map_flags flags,
                   size_t mapSize, void *mappedPointer);

  cl_int UnmapBuffer(cl_mem buffer, void *mappedPointer);

  cl_int ExecuteKernel(const std::string &kernelName,
                       const std::vector<size_t> &globalWorkSizes,
                       const std::vector<size_t> *globalWorkOffsets);

  cl_int FinishKernelExecution();

  cl_int ReadKernelOutput(cl_mem buf, bool blocking, size_t outputSize,
                          void *output);

  // OpenGL interop
  cl_int CreateGLImageObject(cl_mem_flags flags, GLenum target, GLint mipLevel,
                             GLuint texture, cl_mem *newImage);
  cl_int AcquireGLObjects(cl_uint numObjects, const cl_mem *objects);
  cl_int ReleaseGLObjects(cl_uint numObjects, const cl_mem *objects);

 private:
  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_program program;
  cl_command_queue queue;
  std::unordered_map<std::string, cl_kernel> loadedKernels;
  std::unordered_set<cl_mem> loadedBuffers;
};

#endif
