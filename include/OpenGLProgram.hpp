#ifndef OPENGL_PROGRAM
#define OPENGL_PROGRAM

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <OpenGL/OpenGL.h>

#include <string>

#if GLFW_NO_ERROR != GL_NO_ERROR
#error "GLFW no error code does not equal to GL no error code"
#endif
#if GL_NO_ERROR != GLEW_OK
#error "GLEW no error code does not equal to GL no error code"
#endif

// Helper macros
#define GL_ERROR_CHECK(x)                                                 \
  {                                                                       \
    auto error = x;                                                       \
    if (error != GL_NO_ERROR) {                                           \
      cout << "Encountered an OpenGL/GLFW/GLEW error(" << __FILE__ << ":" \
           << __LINE__ << "): " << error << endl;                         \
      return 1;                                                           \
    }                                                                     \
  }

#define GL_ERROR_RETURN(x)      \
  {                             \
    auto error = x;             \
    if (error != GL_NO_ERROR) { \
      return error;             \
    }                           \
  }

#define GL_VOID_ERROR_RETURN(x) \
  x;                            \
  {                             \
    auto error = glGetError();  \
    if (error != GL_NO_ERROR) { \
      return error;             \
    }                           \
  }

#define GLFW_VOID_ERROR_RETURN(x)    \
  x;                                 \
  {                                  \
    auto error = glfwGetError(NULL); \
    if (error != GLFW_NO_ERROR) {    \
      return error;                  \
    }                                \
  }

class OpenGLProgram {
 public:
  OpenGLProgram()
      : m_window(nullptr), baseResX(0), baseResY(0), m_fbo(0), m_texture(0) {}

  int Init(unsigned int sizeX, unsigned int sizeY);
  int Shutdown();

  int AllocateImageFramebuffer();
  inline GLuint GetFramebufferTexture() { return m_texture; }

  int BlitFramebuffer();
  inline int SwapBuffers() {
    GLFW_VOID_ERROR_RETURN(glfwSwapBuffers(m_window));
    return GLFW_NO_ERROR;
  }

  inline bool ShouldClose() { return glfwWindowShouldClose(m_window); }
  inline int SetWindowTitle(const std::string& title) {
    GLFW_VOID_ERROR_RETURN(glfwSetWindowTitle(m_window, title.c_str()));
    return GLFW_NO_ERROR;
  }

  inline int PollEvents() {
    GLFW_VOID_ERROR_RETURN(glfwPollEvents());
    return GLFW_NO_ERROR;
  }

  inline int Flush() {
    GL_VOID_ERROR_RETURN(glFlush());
    return GL_NO_ERROR;
  }

  int GetWindowPixelSize(int* width, int* height);

 private:
  inline int InitGLFW(unsigned int sizeX, unsigned int sizeY);
  inline int InitGLEW();
  inline int InitOpenGL();

  inline int CleanUpFramebuffer();

  GLFWwindow* m_window;
  unsigned int baseResX;
  unsigned int baseResY;
  GLuint m_fbo;
  GLuint m_texture;
};

#endif
