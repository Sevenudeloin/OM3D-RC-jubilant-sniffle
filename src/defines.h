#ifndef DEFINES_H
#define DEFINES_H

#define CREATE_NAME_LINE_HELPER(prefix, LINE) _generated_ ## prefix ## _at_ ## LINE
#define CREATE_NAME_HELPER(prefix, LINE) CREATE_NAME_LINE_HELPER(prefix, LINE)
#define CREATE_UNIQUE_NAME_WITH_PREFIX(prefix) CREATE_NAME_HELPER(prefix, __LINE__)
#define CREATE_UNIQUE_NAME CREATE_UNIQUE_NAME_WITH_PREFIX()

#include <iostream> 

#define TEST_OPENGL_ERROR()                                                             \
  do {                                      \
    GLenum err = glGetError();         \
    if (err == GL_INVALID_ENUM) std::cerr << "OpenGL Error: GL_INVALID_ENUM! in " << __FILE__ << ":" << __LINE__ << std::endl;      \
    if (err == GL_INVALID_VALUE) std::cerr << "OpenGL Error: GL_INVALID_VALUE! in " << __FILE__ << ":" << __LINE__ << std::endl;      \
    if (err == GL_INVALID_OPERATION) std::cerr << "OpenGL Error: GL_INVALID_OPERATION! in " << __FILE__ << ":" << __LINE__ << std::endl;      \
    if (err == GL_INVALID_FRAMEBUFFER_OPERATION) std::cerr << "OpenGL Error: GL_INVALID_FRAMEBUFFER_OPERATION! in " << __FILE__ << ":" << __LINE__ << std::endl;      \
    if (err == GL_OUT_OF_MEMORY) std::cerr << "OpenGL Error: GL_OUT_OF_MEMORY! in " << __FILE__ << ":" << __LINE__ << std::endl;      \
    if (err == GL_STACK_UNDERFLOW) std::cerr << "OpenGL Error: GL_STACK_UNDERFLOW! in " << __FILE__ << ":" << __LINE__ << std::endl;      \
    if (err == GL_STACK_OVERFLOW) std::cerr << "OpenGL Error: GL_STACK_OVERFLOW! in " << __FILE__ << ":" << __LINE__ << std::endl;      \
  } while(0)

/****************** OS DEFINES BELOW ******************/

#if defined(WIN32) || defined(__WIN32) || defined(__WIN32__) || defined(_WINDOWS)
#define OS_WIN
#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#if defined(__linux__) || defined(__gnu_linux__)
#define OS_LINUX
#endif

#endif // DEFINES_H
