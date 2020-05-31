# Real-time Ray Tracer using OpenCL
This is a final project I did for a college Graphics course. The feature set itself is very simple. This is not a ray tracer supporting a lot of advanced features. However, the purpose of this project was to build a cross-platform ray tracer that could run in real-time on the GPU using OpenCL, without needing to use nVidia's RTX platform. At the time I wrote this, real-time ray tracing cards were just starting to release, so that is what spurred my interest in the topic. While technically a complete project, I would not call this a very robust program. I never actually tried to compile on anything other than macOS, so I can't guarantee it will run right away on Windows/Linux. But I think it is a cool example of using OpenCL for real-time graphical applications.

![Example Output](/img/example.png)

## Instructions
The included `Makefile` will work on macOS. It *probably* will not work on anything else though. Overall, however, the program is dead simple to compile. Use your preferred C++ compiler, the source files are in `src/` and the headers are in `include/`. The library dependencies are listed below. Once built, the basic command is `raytracer <USE_OPENGL> [<OUTPUT_FILEPATH_IF_NOT_USING_OPENGL>] <RESOLUTION_X> <RESOLUTION_Y> <SAMPLES_PER_PIXEL> <RAY_BOUNCE_DEPTH>`.

### Dependencies
* [GLFW 3.3](https://www.glfw.org/)
* [GLEW 2.1.0](http://glew.sourceforge.net/)
* [glm](https://glm.g-truc.net/0.9.9/index.html)
* [OpenCL 1.2+](https://www.khronos.org/opencl/)
* [OpenGL 3.3+](https://www.khronos.org/opengl/)
* *Included*: [stb image write](https://github.com/nothings/stb/blob/master/stb_image_write.h)

## Resources
I leaned on the following resources for guidance when constructing this project:
* [Ray Tracing Book Series](https://raytracing.github.io/)
* [Random Numbers in OpenCL](https://bheisler.github.io/post/writing-gpu-accelerated-path-tracer-part-2/)
* [Help with OpenCL](http://raytracey.blogspot.com/2016/11/opencl-path-tracing-tutorial-1-firing.html)