// Inspirations:
// - https://bheisler.github.io/post/writing-gpu-accelerated-path-tracer-part-2/
// (for random numbers)
//
// - Peter Shirley Raytracing in One Weekend (https://raytracing.github.io/)
//
// -http://raytracey.blogspot.com/2016/11/opencl-path-tracing-tutorial-1-firing.html
// (for OpenCL help)
//
// Dependencies:
// - GLFW 3.3
// - GLEW
// - glm
// - OpenCL 1.2+
// - OpenGL 3.3+
// Code written by Trevor Day, 2019

#include <chrono>
#include <fstream>
#include <iostream>

// Include this first to init GLEW
#include "OpenCLProgram.hpp"
//
#include "Camera.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

#define MS_IN_S 1000.0

// File paths
#define CL_KERNEL_PATH "./cl_src/main.cl"
#define KERNEL_INCLUDE "./cl_header/"
// Required OpenCL definition parameters
#define WIDTH "WIDTH"
#define HEIGHT "HEIGHT"
#define SAMPLES "SAMPLES"
#define DEPTH "DEPTH"
#define NUM_SPHERES "NUM_SPHERES"
#define USE_PINHOLE_CAMERA "USE_PINHOLE_CAMERA"
// Kernel names
#define RAYTRACE_KERNEL "raytrace"
#define COLOR_BUFFER_KERNEL "color_compress_buffer"
#define COLOR_IMAGE_KERNEL "color_compress_image"

// Helper to break down work into smaller chunks so that the GPU does not
// time out for large combinations of resolution and sample count
#define BASE_RESOLUTION 512
#define BASE_SAMPLES 16
void calculate_work_iterations(int sizeX, int sizeY, int ns,
                               vector<vector<size_t>>& workSizes,
                               vector<vector<size_t>>& workOffsets) {
  workSizes.clear();
  workOffsets.clear();

  // X
  vector<size_t> workX;
  vector<size_t> offsetsX;
  int numWorkX = sizeX / BASE_RESOLUTION;
  for (int x = 0; x < numWorkX; ++x) {
    workX.push_back(BASE_RESOLUTION);
    offsetsX.push_back(x * BASE_RESOLUTION);
  }
  if (sizeX % BASE_RESOLUTION != 0) {
    workX.push_back(sizeX % BASE_RESOLUTION);
    offsetsX.push_back(numWorkX * BASE_RESOLUTION);
  }

  // Y
  vector<vector<size_t>> workXY;
  vector<vector<size_t>> offsetsXY;
  int numWorkY = sizeY / BASE_RESOLUTION;
  for (int y = 0; y < numWorkY; ++y) {
    for (int x = 0; x < workX.size(); ++x) {
      workXY.push_back({workX[x], BASE_RESOLUTION});
      offsetsXY.push_back({offsetsX[x], (size_t)(y * BASE_RESOLUTION)});
    }
  }
  int moduloBaseY = sizeY % BASE_RESOLUTION;
  if (moduloBaseY != 0) {
    for (int x = 0; x < workX.size(); ++x) {
      workXY.push_back({workX[x], (size_t)moduloBaseY});
      offsetsXY.push_back({offsetsX[x], (size_t)(numWorkY * BASE_RESOLUTION)});
    }
  }

  // Samples
  int numWorkSamples = ns / BASE_SAMPLES;
  for (int s = 0; s < numWorkSamples; ++s) {
    for (int y = 0; y < workXY.size(); ++y) {
      workSizes.push_back({workXY[y][0], workXY[y][1], BASE_SAMPLES});
      workOffsets.push_back(
          {offsetsXY[y][0], offsetsXY[y][1], (size_t)(s * BASE_SAMPLES)});
    }
  }
  int moduloBaseSamples = ns % BASE_SAMPLES;
  if (moduloBaseSamples != 0) {
    for (int y = 0; y < workXY.size(); ++y) {
      workSizes.push_back(
          {workXY[y][0], workXY[y][1], (size_t)moduloBaseSamples});
      workOffsets.push_back({offsetsXY[y][0], offsetsXY[y][1],
                             (size_t)(numWorkSamples * BASE_SAMPLES)});
    }
  }
}

// Helper function to write the OpenCL ray-traced image to disk for a
// single frame
int write_image(int sizeX, int sizeY, int ns, string outPath,
                OpenCLProgram& program, cl_mem traceResultsBuffer) {
  // Execution
  auto startOfFrame = chrono::high_resolution_clock::now();

  vector<vector<size_t>> globalWorkSizes;
  vector<vector<size_t>> globalWorkOffsets;
  calculate_work_iterations(sizeX, sizeY, ns, globalWorkSizes,
                            globalWorkOffsets);
  for (int i = 0; i < globalWorkSizes.size(); ++i) {
    CL_ERROR_CHECK(program.ExecuteKernel(RAYTRACE_KERNEL, globalWorkSizes[i],
                                         &globalWorkOffsets[i]))
  }
  CL_ERROR_CHECK(program.FinishKernelExecution())

  // Color compression kernel
  CL_ERROR_CHECK(program.LoadKernel(COLOR_BUFFER_KERNEL))
  CL_ERROR_CHECK(program.SetArgument(COLOR_BUFFER_KERNEL, 0, sizeof(cl_mem),
                                     &traceResultsBuffer));
  cl_mem outputBuffer;
  CL_ERROR_CHECK(program.CreateBufferArgument(
      COLOR_BUFFER_KERNEL, 1, CL_MEM_WRITE_ONLY,
      sizeof(unsigned char) * sizeX * sizeY * 3, nullptr, &outputBuffer));
  vector<size_t> colorGlobalWorkSizes = {(size_t)sizeX, (size_t)sizeY};
  CL_ERROR_CHECK(
      program.ExecuteKernel(COLOR_BUFFER_KERNEL, colorGlobalWorkSizes, nullptr))
  CL_ERROR_CHECK(program.FinishKernelExecution())

  auto endOfFrame = chrono::high_resolution_clock::now();
  chrono::duration<double, milli> frameTime = endOfFrame - startOfFrame;
  cout << "Frame time: " << frameTime.count() << " ms" << endl;

  // Get output
  unsigned char* cpuOutput = new unsigned char[sizeX * sizeY * 3];
  CL_ERROR_CHECK(program.ReadKernelOutput(
      outputBuffer, true, sizeof(unsigned char) * sizeX * sizeY * 3, cpuOutput))
  CL_ERROR_CHECK(program.Unload());
  // Write output to disk
  int writeResult = stbi_write_png(outPath.c_str(), sizeX, sizeY, 3, cpuOutput,
                                   sizeX * sizeof(unsigned char) * 3);
  delete[] cpuOutput;

  if (writeResult == 0) {
    cout << "There was an error writing the file." << endl;
    return 1;
  }

  cout << "Successfully wrote " << outPath << endl;

  return 0;
}

int opengl_loop(int sizeX, int sizeY, int ns, OpenCLProgram& program,
                OpenGLProgram& glProgram, Camera& cam,
                cl_mem traceResultsBuffer) {
  vector<vector<size_t>> raytraceGlobalWorkSizes;
  vector<vector<size_t>> raytraceGlobalWorkOffsets;
  calculate_work_iterations(sizeX, sizeY, ns, raytraceGlobalWorkSizes,
                            raytraceGlobalWorkOffsets);

  GL_ERROR_CHECK(glProgram.AllocateImageFramebuffer())

  // Color compression kernel
  CL_ERROR_CHECK(program.LoadKernel(COLOR_IMAGE_KERNEL))
  CL_ERROR_CHECK(program.SetArgument(COLOR_IMAGE_KERNEL, 0, sizeof(cl_mem),
                                     &traceResultsBuffer));
  cl_mem image;
  CL_ERROR_CHECK(
      program.CreateGLImageObject(CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0,
                                  glProgram.GetFramebufferTexture(), &image));
  CL_ERROR_CHECK(
      program.SetArgument(COLOR_IMAGE_KERNEL, 1, sizeof(cl_mem), &image));
  vector<size_t> colorGlobalWorkSizes = {(size_t)sizeX, (size_t)sizeY};

  double deltaTime = 0.0;
  while (true) {
    auto startOfFrame = chrono::high_resolution_clock::now();

    // Check input
    GL_ERROR_CHECK(glProgram.PollEvents());
    if (glProgram.ShouldClose()) {
      break;
    }
    // Update
    cam.RotateCamera(1.0f, (float)deltaTime,
                     CLTypes::Vector3(0.0f, 0.0f, -1.0f));
    CLTypes::Camera cl_cam = cam.Calculate();
    cl_mem cameraBuffer;
    CL_ERROR_CHECK(program.CreateBufferArgument(
        RAYTRACE_KERNEL, 0, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        sizeof(CLTypes::Camera), &cl_cam, &cameraBuffer));
    // Execute ray trace kernel
    for (int i = 0; i < raytraceGlobalWorkSizes.size(); ++i) {
      CL_ERROR_CHECK(program.ExecuteKernel(RAYTRACE_KERNEL,
                                           raytraceGlobalWorkSizes[i],
                                           &raytraceGlobalWorkOffsets[i]))
    }
    CL_ERROR_CHECK(program.FinishKernelExecution())
    CL_ERROR_CHECK(program.ReleaseBuffer(cameraBuffer))
    // GL flush
    GL_ERROR_CHECK(glProgram.Flush());
    // Acquire ownership for OpenCL
    CL_ERROR_CHECK(program.AcquireGLObjects(1, &image));
    // Run color compression to OpenGL texture kernel
    CL_ERROR_CHECK(program.ExecuteKernel(COLOR_IMAGE_KERNEL,
                                         colorGlobalWorkSizes, nullptr))
    // CL flush
    CL_ERROR_CHECK(program.FinishKernelExecution());
    // Acquire OpenGL ownership
    CL_ERROR_CHECK(program.ReleaseGLObjects(1, &image));
    // Blit framebuffer
    GL_ERROR_CHECK(glProgram.BlitFramebuffer());
    // Swap buffers
    GL_ERROR_CHECK(glProgram.SwapBuffers());
    // Print FPS
    auto endOfFrame = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> frameTime = endOfFrame - startOfFrame;
    deltaTime = frameTime.count() / MS_IN_S;
    GL_ERROR_CHECK(
        glProgram.SetWindowTitle("FPS: " + to_string(1.0 / deltaTime)));
  }

  CL_ERROR_CHECK(program.Unload());
  GL_ERROR_CHECK(glProgram.Shutdown());
  return 0;
}

int main(int argc, char* argv[]) {
  // Default parameters
  bool useOpenGL = true;
  string outputPathName;
  int sizeX = BASE_RESOLUTION;
  int sizeY = BASE_RESOLUTION;
  int ns = BASE_SAMPLES;
  int rayDepth = 50;
  // Command line args
  if (argc > 1) {
    useOpenGL = stoi(argv[1]);
  }
  int argNum = 2;
  if (!useOpenGL) {
    if (argc < 3) {
      cout << "When not using OpenGL an output image filepath is required."
           << endl;
      return 0;
    } else {
      outputPathName = argv[argNum];
      ++argNum;
    }
  }
  if (argc > argNum) {
    sizeX = stoi(argv[argNum]);
    ++argNum;
  }
  if (argc > argNum) {
    sizeY = stoi(argv[argNum]);
    ++argNum;
  }
  if (argc > argNum) {
    ns = stoi(argv[argNum]);
    ++argNum;
  }
  if (argc > argNum) {
    rayDepth = stoi(argv[argNum]);
    ++argNum;
  }

  unordered_map<string, cl_platform_id> platforms;
  CL_ERROR_CHECK(OpenCLProgram::GetAvailablePlatforms(platforms))
  cl_platform_id platform = platforms.begin()->second;

  // If necessary, ask for user input on which platform to use
  if (platforms.size() > 1) {
    cout << "Available OpenCL platforms: \n\n";
    for (auto i = platforms.begin(); i != platforms.end(); ++i) {
      cout << "\t"
           << "- " << i->first << endl;
    }

    cout << endl << "Enter the name of the OpenCL platform you want to use: ";
    string input;
    cin >> input;
    // Handle incorrect user input
    while (platforms.find(input) == platforms.end()) {
      cin.clear();  // clear errors/bad flags on cin
      cin.ignore(cin.rdbuf()->in_avail(),
                 '\n');  // ignores exact number of chars in cin buffer
      cout << "No such platform." << endl
           << "Enter the name of the OpenCL platform you want to use: ";
      cin >> input;
    }
    // Print the name of chosen OpenCL platform
    cout << "Using OpenCL platform: \t" << input << endl;
    platform = platforms[input];
  }

  unordered_map<string, cl_device_id> devices;
  CL_ERROR_CHECK(OpenCLProgram::GetAvailableDevices(platform, devices))
  cl_device_id device = devices.begin()->second;

  // If necessary, ask for user input on which device to use
  if (devices.size() > 1) {
    cout << "Available OpenCL devices: \n\n";
    for (auto i = devices.begin(); i != devices.end(); ++i) {
      cout << "\t"
           << "- " << i->first << endl;
    }

    cout << endl << "Enter the name of the OpenCL device you want to use: ";
    string input;
    cin >> input;
    // Handle incorrect user input
    while (devices.find(input) == devices.end()) {
      cin.clear();  // clear errors/bad flags on cin
      cin.ignore(cin.rdbuf()->in_avail(),
                 '\n');  // ignores exact number of chars in cin buffer
      cout << "No such device." << endl
           << "Enter the name of the OpenCL device you want to use: ";
      cin >> input;
    }
    // Print the name of chosen OpenCL device
    cout << "Using OpenCL device: \t" << input << endl;
    device = devices[input];
  }

  OpenGLProgram glProgram;
  std::unordered_map<cl_context_properties, cl_context_properties>
      contextProperties;
  if (useOpenGL) {
    // Handle OpenGL interop
    if (OpenCLProgram::OpenGLSharingSupported(device) != CL_SUCCESS) {
      cout << "OpenGL interop is not supported by the selected device. Please "
              "turn off OpenGL interop or select a different device."
           << endl;
      return 1;
    }

    // OpenGL program init
    GL_ERROR_CHECK(glProgram.Init(sizeX, sizeY))
#if defined(__APPLE__) || defined(MACOSX)
    CGLContextObj cglContext = CGLGetCurrentContext();
    contextProperties = {{CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
                          (cl_context_properties)CGLGetShareGroup(cglContext)}};
#elif defined(_WIN64) || defined(_WIN32)
    contextProperties = {
        {CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext()},
        {CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC()}};
#else
    contextProperties = {
        {CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext()},
        {CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay()}};
#endif
  }

  // TODO: Allow scene data that is not hard-coded
  const int sphereCount = 5;
  CLTypes::Sphere world[sphereCount];
  world[0] =
      CLTypes::Sphere(CLTypes::Vector3(0, 0, -1), 0.5f,
                      CLTypes::Lambertian(CLTypes::Vector3(0.1f, 0.2f, 0.5f)));
  world[1] =
      CLTypes::Sphere(CLTypes::Vector3(0, -100.5, -1), 100,
                      CLTypes::Lambertian(CLTypes::Vector3(0.8f, 0.8f, 0.0f)));
  world[2] =
      CLTypes::Sphere(CLTypes::Vector3(1, 0, -1), 0.5f,
                      CLTypes::Metal(CLTypes::Vector3(0.8f, 0.6f, 0.2f), 0.0f));
  world[3] = CLTypes::Sphere(CLTypes::Vector3(-1, 0, -1), 0.5f,
                             CLTypes::Dielectric(2.52));
  world[4] = CLTypes::Sphere(CLTypes::Vector3(-1, 0, -1), -0.45f,
                             CLTypes::Dielectric(2.52));

  cl_bool usePinholeCamera = CL_TRUE;
  Camera cam(CLTypes::Vector3(0, 0, 2), CLTypes::Vector3(0, 0, -1),
             CLTypes::Vector3(0, 1, 0), 90, cl_float(sizeX) / cl_float(sizeY),
             usePinholeCamera);
  CLTypes::Camera cl_cam = cam.Calculate();

  const int numRaytraceElements = sizeX * sizeY * ns * 3;

  // Read our program source
  ifstream in;
  in.open(CL_KERNEL_PATH, ifstream::in);
  string source((istreambuf_iterator<char>(in)), (istreambuf_iterator<char>()));
  // Set up our definitions for compilation
  unordered_map<string, string> definitions = {
      {WIDTH, to_string(sizeX)},
      {HEIGHT, to_string(sizeY)},
      {SAMPLES, to_string(ns)},
      {DEPTH, to_string(rayDepth)},
      {NUM_SPHERES, to_string(sphereCount)},
      {USE_PINHOLE_CAMERA, to_string(usePinholeCamera)}};
  // Set up include paths
  vector<string> includePaths = {KERNEL_INCLUDE};
  // Initialize our OpenCL program
  OpenCLProgram program;
  {
    string errorLog;
    if (program.Init(platform, device, source, definitions, includePaths,
                     contextProperties, errorLog) != CL_SUCCESS) {
      std::cout << "Error during OpenCL program compilation! (" << errorLog
                << ")" << std::endl;
      return 1;
    }
  }

  // Set up our raytrace kernel
  CL_ERROR_CHECK(program.LoadKernel(RAYTRACE_KERNEL))

  // Arguments
  CL_ERROR_CHECK(program.CreateBufferArgument(
      RAYTRACE_KERNEL, 0, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
      sizeof(CLTypes::Camera), &cl_cam, nullptr));
  CL_ERROR_CHECK(program.CreateBufferArgument(
      RAYTRACE_KERNEL, 1, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
      sizeof(world), &world[0], nullptr));
  cl_mem traceResultsBuffer;
  CL_ERROR_CHECK(program.CreateBufferArgument(
      RAYTRACE_KERNEL, 2, CL_MEM_READ_WRITE,
      sizeof(float) * numRaytraceElements, nullptr, &traceResultsBuffer));

  if (useOpenGL) {
    return opengl_loop(sizeX, sizeY, ns, program, glProgram, cam,
                       traceResultsBuffer);
  }
  return write_image(sizeX, sizeY, ns, outputPathName, program,
                     traceResultsBuffer);
}
