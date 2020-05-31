#include "OpenGLProgram.hpp"

int OpenGLProgram::Init(unsigned int sizeX, unsigned int sizeY) {
  GL_ERROR_RETURN(InitGLFW(sizeX, sizeY));
  GL_ERROR_RETURN(InitGLEW());
  return InitOpenGL();
}

int OpenGLProgram::InitGLFW(unsigned int sizeX, unsigned int sizeY) {
  // Initialize OpenGL using GLFW
  GLFW_VOID_ERROR_RETURN(glfwInit());
  GLFW_VOID_ERROR_RETURN(glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3));
  GLFW_VOID_ERROR_RETURN(glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3));
  GLFW_VOID_ERROR_RETURN(
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE));
  GLFW_VOID_ERROR_RETURN(glfwWindowHint(GLFW_RESIZABLE, GL_TRUE));
  GLFW_VOID_ERROR_RETURN(glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE));

  // Initialize GLFW window
  GLFW_VOID_ERROR_RETURN(
      GLFWwindow* window = glfwCreateWindow(
          sizeX, sizeY, "OpenCL Ray Tracer - Trevor Day", NULL, NULL));
  if (!window) {
    return GLFW_PLATFORM_ERROR;
  }
  m_window = window;
  baseResX = sizeX;
  baseResY = sizeY;
  GLFW_VOID_ERROR_RETURN(glfwMakeContextCurrent(m_window));
  return GLFW_NO_ERROR;
}

int OpenGLProgram::InitGLEW() {
  // Initialize GLEW
  return glewInit();
}

int OpenGLProgram::InitOpenGL() {
  int windowWidth, windowHeight;
  GL_ERROR_RETURN(GetWindowPixelSize(&windowWidth, &windowHeight));
  GL_VOID_ERROR_RETURN(glViewport(0, 0, windowWidth, windowHeight));
  return GL_NO_ERROR;
}

int OpenGLProgram::Shutdown() {
  GL_ERROR_RETURN(CleanUpFramebuffer());

  // Clean up GLFW
  GLFW_VOID_ERROR_RETURN(glfwDestroyWindow(m_window));
  GLFW_VOID_ERROR_RETURN(glfwTerminate());
  return GL_NO_ERROR;
}

int OpenGLProgram::AllocateImageFramebuffer() {
  GL_ERROR_RETURN(CleanUpFramebuffer());

  GLuint fb;
  GL_VOID_ERROR_RETURN(glGenFramebuffers(1, &fb));
  GL_VOID_ERROR_RETURN(glBindFramebuffer(GL_FRAMEBUFFER, fb));

  GLuint tex;
  GL_VOID_ERROR_RETURN(glGenTextures(1, &tex));
  GL_VOID_ERROR_RETURN(glBindTexture(GL_TEXTURE_2D, tex));
  GL_VOID_ERROR_RETURN(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, baseResX,
                                    baseResY, 0, GL_RGBA, GL_FLOAT, NULL));
  // Explicitly require linear filter
  GL_VOID_ERROR_RETURN(
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_VOID_ERROR_RETURN(
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_VOID_ERROR_RETURN(glBindTexture(GL_TEXTURE_2D, 0));

  // Attach to framebuffer
  GL_VOID_ERROR_RETURN(glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0));

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    return GL_INVALID_FRAMEBUFFER_OPERATION;
  }
  m_fbo = fb;
  m_texture = tex;
  GL_VOID_ERROR_RETURN(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  return GL_NO_ERROR;
}

inline int OpenGLProgram::CleanUpFramebuffer() {
  if (m_texture != 0) {
    GL_VOID_ERROR_RETURN(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));

    GL_VOID_ERROR_RETURN(glDeleteTextures(1, &m_texture));

    GL_VOID_ERROR_RETURN(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  }
  if (m_fbo != 0) {
    GL_VOID_ERROR_RETURN(glDeleteFramebuffers(1, &m_fbo));
  }

  m_fbo = 0;
  m_texture = 0;
  return GL_NO_ERROR;
}

int OpenGLProgram::BlitFramebuffer() {
  GL_VOID_ERROR_RETURN(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo));

  int windowWidth, windowHeight;
  GL_ERROR_RETURN(GetWindowPixelSize(&windowWidth, &windowHeight));
  // Resize the viewport while we are at it
  GL_VOID_ERROR_RETURN(glViewport(0, 0, windowWidth, windowHeight));
  GL_VOID_ERROR_RETURN(glBlitFramebuffer(0, 0, baseResX, baseResY, 0, 0,
                                         windowWidth, windowHeight,
                                         GL_COLOR_BUFFER_BIT, GL_LINEAR));

  GL_VOID_ERROR_RETURN(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0));
  GL_VOID_ERROR_RETURN(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
  return GL_NO_ERROR;
}

// NOTE: Screen coordinates and pixel size of windows are not the same
int OpenGLProgram::GetWindowPixelSize(int* width, int* height) {
  GLFW_VOID_ERROR_RETURN(glfwGetFramebufferSize(m_window, width, height));
  return GLFW_NO_ERROR;
}
