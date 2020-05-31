#include "OpenCLProgram.hpp"

#include <sstream>

cl_int OpenCLProgram::GetAvailablePlatforms(
    std::unordered_map<std::string, cl_platform_id> &platforms) {
  // Detect number of available platforms
  cl_uint numPlatforms;
  CL_ERROR_RETURN(clGetPlatformIDs(0, NULL, &numPlatforms));

  // Grab platforms
  cl_platform_id platformIds[numPlatforms];
  CL_ERROR_RETURN(clGetPlatformIDs(numPlatforms, &platformIds[0], NULL));

  size_t platformNameSize;
  for (cl_uint i = 0; i < numPlatforms; ++i) {
    CL_ERROR_RETURN(clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, 0, NULL,
                                      &platformNameSize));

    char platformName[platformNameSize];
    CL_ERROR_RETURN(clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME,
                                      platformNameSize, &platformName[0],
                                      NULL));

    platforms.emplace(std::string(platformName), platformIds[i]);
  }

  return CL_SUCCESS;
}

cl_int OpenCLProgram::GetAvailableDevices(
    cl_platform_id platformId,
    std::unordered_map<std::string, cl_device_id> &devices) {
  // Detect number of available devices
  cl_uint numDevices;
  CL_ERROR_RETURN(
      clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices));

  // Grab devices
  cl_device_id deviceIds[numDevices];
  CL_ERROR_RETURN(clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, numDevices,
                                 &deviceIds[0], NULL));

  size_t deviceNameSize;
  for (unsigned int i = 0; i < numDevices; ++i) {
    CL_ERROR_RETURN(clGetDeviceInfo(deviceIds[i], CL_DEVICE_NAME, 0, NULL,
                                    &deviceNameSize));

    char deviceName[deviceNameSize];
    CL_ERROR_RETURN(clGetDeviceInfo(deviceIds[i], CL_DEVICE_NAME,
                                    deviceNameSize, &deviceName[0], NULL));

    devices.emplace(std::string(deviceName), deviceIds[i]);
  }

  return CL_SUCCESS;
}

cl_int OpenCLProgram::IsExtensionSupported(const std::string &extension,
                                           cl_device_id device) {
  size_t extensionSize;
  CL_ERROR_RETURN(
      clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize));

  char extensions[extensionSize];
  CL_ERROR_RETURN(clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, extensionSize,
                                  &extensions[0], NULL));

  if (std::string(extensions).find(extension) != std::string::npos) {
    return CL_SUCCESS;
  }
  return CL_BUILD_ERROR;
}

cl_int OpenCLProgram::OpenGLSharingSupported(cl_device_id device) {
#if defined(__APPLE__) || defined(MACOSX)
  static const std::string CL_GL_SHARING_EXTENSION = "cl_APPLE_gl_sharing";
#else
  static const std::string CL_GL_SHARING_EXTENSION = "cl_khr_gl_sharing";
#endif

  return IsExtensionSupported(CL_GL_SHARING_EXTENSION, device);
}

cl_int OpenCLProgram::Init(
    cl_platform_id pId, cl_device_id dId, const std::string &programSource,
    const std::unordered_map<std::string, std::string> &definitions,
    const std::vector<std::string> &includePaths,
    const std::unordered_map<cl_context_properties, cl_context_properties>
        &properties,
    std::string &errorLog) {
  platform = pId;
  device = dId;

  cl_int errorCode;

  // Create context
  cl_context_properties propertiesArr[(properties.size() * 2) + 3];
  int i = 0;
  for (auto prop = properties.begin(); prop != properties.end(); ++prop) {
    propertiesArr[i] = prop->first;
    propertiesArr[i + 1] = prop->second;
    i += 2;
  }
  propertiesArr[i] = CL_CONTEXT_PLATFORM;
  propertiesArr[i + 1] = (cl_context_properties)platform;
  propertiesArr[i + 2] = 0;
  context =
      clCreateContext(&propertiesArr[0], 1, &device, NULL, NULL, &errorCode);
  CL_ERROR_RETURN(errorCode)

  // Create program
  const char *src = programSource.c_str();
  const size_t srcSize = programSource.size();
  program = clCreateProgramWithSource(context, 1, &src, &srcSize, &errorCode);
  CL_ERROR_RETURN(errorCode)

  // Compile the program
  std::stringstream buildOptions;
  for (auto define = definitions.begin(); define != definitions.end();
       ++define) {
    buildOptions << " -D " << define->first << "=" << define->second;
  }
  for (auto include = includePaths.begin(); include != includePaths.end();
       ++include) {
    buildOptions << " -I " << *include;
  }
  std::string optionsString = buildOptions.str();

  if (cl_int result = clBuildProgram(program, 1, &device, optionsString.c_str(),
                                     NULL, NULL) != CL_SUCCESS) {
    size_t logSize;
    CL_ERROR_RETURN(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                                          0, NULL, &logSize));
    char log[logSize];
    CL_ERROR_RETURN(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                                          logSize, log, NULL));
    errorLog = std::string(log);
    return result;
  }

  // Create command queue
  queue = clCreateCommandQueue(context, device, 0, &errorCode);
  CL_ERROR_RETURN(errorCode);
  return CL_SUCCESS;
}

cl_int OpenCLProgram::Unload() {
  // Ensure we are done running operations
  CL_ERROR_RETURN(FinishKernelExecution());
  // Release any buffers still in use
  for (auto buf : loadedBuffers) {
    CL_ERROR_RETURN(clReleaseMemObject(buf));
  }
  loadedBuffers.clear();
  // Release kernels
  for (auto kernel : loadedKernels) {
    CL_ERROR_RETURN(clReleaseKernel(kernel.second));
  }
  loadedKernels.clear();
  // Release command queue
  CL_ERROR_RETURN(clReleaseCommandQueue(queue));
  // Release program
  CL_ERROR_RETURN(clReleaseProgram(program));
  // Release context
  return clReleaseContext(context);
}

cl_int OpenCLProgram::LoadKernel(const std::string &kernelName) {
  cl_int errorCode;
  if (loadedKernels.find(kernelName) != loadedKernels.end()) {
    return CL_INVALID_KERNEL;
  }
  loadedKernels[kernelName] =
      clCreateKernel(program, kernelName.c_str(), &errorCode);
  CL_ERROR_RETURN(errorCode);
  return CL_SUCCESS;
}

cl_int OpenCLProgram::CreateBuffer(cl_mem_flags flags, size_t bufSize,
                                   void *data, cl_mem *newBuffer) {
  // Create the buffer
  cl_int errorCode;
  cl_mem temp = clCreateBuffer(context, flags, bufSize, data, &errorCode);
  CL_ERROR_RETURN(errorCode);
  loadedBuffers.insert(temp);
  if (newBuffer != nullptr) {
    *newBuffer = temp;
  }
  return CL_SUCCESS;
}

cl_int OpenCLProgram::ReleaseBuffer(cl_mem buf) {
  if (loadedBuffers.find(buf) == loadedBuffers.end()) {
    return CL_INVALID_MEM_OBJECT;
  }
  loadedBuffers.erase(buf);
  return clReleaseMemObject(buf);
}

cl_int OpenCLProgram::SetArgument(const std::string &kernelName, cl_uint argNum,
                                  size_t argSize, void *data) {
  if (loadedKernels.find(kernelName) == loadedKernels.end()) {
    return CL_INVALID_KERNEL;
  }
  // Set the argument
  return clSetKernelArg(loadedKernels[kernelName], argNum, argSize, data);
}

cl_int OpenCLProgram::CreateBufferArgument(const std::string &kernelName,
                                           cl_uint argNum, cl_mem_flags flags,
                                           size_t argSize, void *data,
                                           cl_mem *newBuffer) {
  cl_mem temp;
  CL_ERROR_RETURN(CreateBuffer(flags, argSize, data, &temp));
  CL_ERROR_RETURN(SetArgument(kernelName, argNum, sizeof(cl_mem), &temp));
  if (newBuffer != nullptr) {
    *newBuffer = temp;
  }
  return CL_SUCCESS;
}

cl_int OpenCLProgram::MapBuffer(cl_mem buffer, cl_bool blocking,
                                cl_map_flags flags, size_t mapSize,
                                void *mappedPointer) {
  cl_int err;
  void *temp = clEnqueueMapBuffer(queue, buffer, blocking, flags, 0, mapSize, 0,
                                  NULL, NULL, &err);
  CL_ERROR_RETURN(err);
  mappedPointer = temp;
  return CL_SUCCESS;
}

cl_int OpenCLProgram::UnmapBuffer(cl_mem buffer, void *mappedPointer) {
  return clEnqueueUnmapMemObject(queue, buffer, mappedPointer, 0, NULL, NULL);
}

cl_int OpenCLProgram::CreateGLImageObject(cl_mem_flags flags, GLenum target,
                                          GLint mipLevel, GLuint texture,
                                          cl_mem *newImage) {
  cl_int error;
  cl_mem temp =
      clCreateFromGLTexture(context, flags, target, mipLevel, texture, &error);
  CL_ERROR_RETURN(error);
  loadedBuffers.insert(temp);
  if (newImage != nullptr) {
    *newImage = temp;
  }
  return CL_SUCCESS;
}

cl_int OpenCLProgram::ExecuteKernel(
    const std::string &kernelName, const std::vector<size_t> &globalWorkSizes,
    const std::vector<size_t> *globalWorkOffsets) {
  if (loadedKernels.find(kernelName) == loadedKernels.end()) {
    return CL_INVALID_KERNEL;
  }
  auto offsets =
      globalWorkOffsets == nullptr ? NULL : (*globalWorkOffsets).data();
  return clEnqueueNDRangeKernel(queue, loadedKernels[kernelName],
                                globalWorkSizes.size(), offsets,
                                globalWorkSizes.data(), NULL, 0, NULL, NULL);
}

cl_int OpenCLProgram::FinishKernelExecution() { return clFinish(queue); }

cl_int OpenCLProgram::ReadKernelOutput(cl_mem buf, bool blocking,
                                       size_t outputSize, void *output) {
  cl_bool block;
  if (blocking) {
    block = CL_TRUE;
  } else {
    block = CL_FALSE;
  }
  return clEnqueueReadBuffer(queue, buf, block, 0, outputSize, output, 0, NULL,
                             NULL);
}

cl_int OpenCLProgram::AcquireGLObjects(cl_uint numObjects,
                                       const cl_mem *objects) {
  return clEnqueueAcquireGLObjects(queue, numObjects, objects, 0, NULL, NULL);
}

cl_int OpenCLProgram::ReleaseGLObjects(cl_uint numObjects,
                                       const cl_mem *objects) {
  return clEnqueueReleaseGLObjects(queue, numObjects, objects, 0, NULL, NULL);
}
