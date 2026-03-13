#include "engine/gl_api.hpp"

#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
namespace
{
PFNGLGENVERTEXARRAYSPROC pGlGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC pGlBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC pGlDeleteVertexArrays = nullptr;
PFNGLGENBUFFERSPROC pGlGenBuffers = nullptr;
PFNGLBINDBUFFERPROC pGlBindBuffer = nullptr;
PFNGLBUFFERDATAPROC pGlBufferData = nullptr;
PFNGLDELETEBUFFERSPROC pGlDeleteBuffers = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC pGlEnableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC pGlVertexAttribPointer = nullptr;
PFNGLCREATESHADERPROC pGlCreateShader = nullptr;
PFNGLSHADERSOURCEPROC pGlShaderSource = nullptr;
PFNGLCOMPILESHADERPROC pGlCompileShader = nullptr;
PFNGLGETSHADERIVPROC pGlGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC pGlGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC pGlDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC pGlCreateProgram = nullptr;
PFNGLATTACHSHADERPROC pGlAttachShader = nullptr;
PFNGLDETACHSHADERPROC pGlDetachShader = nullptr;
PFNGLLINKPROGRAMPROC pGlLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC pGlGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC pGlGetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC pGlDeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC pGlUseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC pGlGetUniformLocation = nullptr;
PFNGLUNIFORMMATRIX4FVPROC pGlUniformMatrix4fv = nullptr;
PFNGLUNIFORM4FPROC pGlUniform4f = nullptr;
PFNGLUNIFORM3FPROC pGlUniform3f = nullptr;

template <typename T>
bool LoadProc(T& output, const char* name)
{
    output = reinterpret_cast<T>(SDL_GL_GetProcAddress(name));
    return output != nullptr;
}
} // namespace
#endif

namespace cotrx::gl
{
bool Load()
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    return true;
#else
    auto success = true;
    success = LoadProc(pGlGenVertexArrays, "glGenVertexArrays") && success;
    success = LoadProc(pGlBindVertexArray, "glBindVertexArray") && success;
    success = LoadProc(pGlDeleteVertexArrays, "glDeleteVertexArrays") && success;
    success = LoadProc(pGlGenBuffers, "glGenBuffers") && success;
    success = LoadProc(pGlBindBuffer, "glBindBuffer") && success;
    success = LoadProc(pGlBufferData, "glBufferData") && success;
    success = LoadProc(pGlDeleteBuffers, "glDeleteBuffers") && success;
    success = LoadProc(pGlEnableVertexAttribArray, "glEnableVertexAttribArray") && success;
    success = LoadProc(pGlVertexAttribPointer, "glVertexAttribPointer") && success;
    success = LoadProc(pGlCreateShader, "glCreateShader") && success;
    success = LoadProc(pGlShaderSource, "glShaderSource") && success;
    success = LoadProc(pGlCompileShader, "glCompileShader") && success;
    success = LoadProc(pGlGetShaderiv, "glGetShaderiv") && success;
    success = LoadProc(pGlGetShaderInfoLog, "glGetShaderInfoLog") && success;
    success = LoadProc(pGlDeleteShader, "glDeleteShader") && success;
    success = LoadProc(pGlCreateProgram, "glCreateProgram") && success;
    success = LoadProc(pGlAttachShader, "glAttachShader") && success;
    success = LoadProc(pGlDetachShader, "glDetachShader") && success;
    success = LoadProc(pGlLinkProgram, "glLinkProgram") && success;
    success = LoadProc(pGlGetProgramiv, "glGetProgramiv") && success;
    success = LoadProc(pGlGetProgramInfoLog, "glGetProgramInfoLog") && success;
    success = LoadProc(pGlDeleteProgram, "glDeleteProgram") && success;
    success = LoadProc(pGlUseProgram, "glUseProgram") && success;
    success = LoadProc(pGlGetUniformLocation, "glGetUniformLocation") && success;
    success = LoadProc(pGlUniformMatrix4fv, "glUniformMatrix4fv") && success;
    success = LoadProc(pGlUniform4f, "glUniform4f") && success;
    success = LoadProc(pGlUniform3f, "glUniform3f") && success;
    return success;
#endif
}

void Reset()
{
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    pGlGenVertexArrays = nullptr;
    pGlBindVertexArray = nullptr;
    pGlDeleteVertexArrays = nullptr;
    pGlGenBuffers = nullptr;
    pGlBindBuffer = nullptr;
    pGlBufferData = nullptr;
    pGlDeleteBuffers = nullptr;
    pGlEnableVertexAttribArray = nullptr;
    pGlVertexAttribPointer = nullptr;
    pGlCreateShader = nullptr;
    pGlShaderSource = nullptr;
    pGlCompileShader = nullptr;
    pGlGetShaderiv = nullptr;
    pGlGetShaderInfoLog = nullptr;
    pGlDeleteShader = nullptr;
    pGlCreateProgram = nullptr;
    pGlAttachShader = nullptr;
    pGlDetachShader = nullptr;
    pGlLinkProgram = nullptr;
    pGlGetProgramiv = nullptr;
    pGlGetProgramInfoLog = nullptr;
    pGlDeleteProgram = nullptr;
    pGlUseProgram = nullptr;
    pGlGetUniformLocation = nullptr;
    pGlUniformMatrix4fv = nullptr;
    pGlUniform4f = nullptr;
    pGlUniform3f = nullptr;
#endif
}

void GenVertexArrays(GLsizei count, GLuint* arrays)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glGenVertexArrays(count, arrays);
#else
    pGlGenVertexArrays(count, arrays);
#endif
}

void BindVertexArray(GLuint array)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glBindVertexArray(array);
#else
    pGlBindVertexArray(array);
#endif
}

void DeleteVertexArrays(GLsizei count, const GLuint* arrays)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glDeleteVertexArrays(count, arrays);
#else
    pGlDeleteVertexArrays(count, arrays);
#endif
}

void GenBuffers(GLsizei count, GLuint* buffers)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glGenBuffers(count, buffers);
#else
    pGlGenBuffers(count, buffers);
#endif
}

void BindBuffer(GLenum target, GLuint buffer)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glBindBuffer(target, buffer);
#else
    pGlBindBuffer(target, buffer);
#endif
}

void BufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glBufferData(target, size, data, usage);
#else
    pGlBufferData(target, size, data, usage);
#endif
}

void DeleteBuffers(GLsizei count, const GLuint* buffers)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glDeleteBuffers(count, buffers);
#else
    pGlDeleteBuffers(count, buffers);
#endif
}

void EnableVertexAttribArray(GLuint index)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glEnableVertexAttribArray(index);
#else
    pGlEnableVertexAttribArray(index);
#endif
}

void VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
#else
    pGlVertexAttribPointer(index, size, type, normalized, stride, pointer);
#endif
}

GLuint CreateShader(GLenum shaderType)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    return glCreateShader(shaderType);
#else
    return pGlCreateShader(shaderType);
#endif
}

void ShaderSource(GLuint shader, GLsizei count, const GLchar* const* source, const GLint* length)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glShaderSource(shader, count, source, length);
#else
    pGlShaderSource(shader, count, source, length);
#endif
}

void CompileShader(GLuint shader)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glCompileShader(shader);
#else
    pGlCompileShader(shader);
#endif
}

void GetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glGetShaderiv(shader, pname, params);
#else
    pGlGetShaderiv(shader, pname, params);
#endif
}

void GetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glGetShaderInfoLog(shader, maxLength, length, infoLog);
#else
    pGlGetShaderInfoLog(shader, maxLength, length, infoLog);
#endif
}

void DeleteShader(GLuint shader)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glDeleteShader(shader);
#else
    pGlDeleteShader(shader);
#endif
}

GLuint CreateProgram()
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    return glCreateProgram();
#else
    return pGlCreateProgram();
#endif
}

void AttachShader(GLuint program, GLuint shader)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glAttachShader(program, shader);
#else
    pGlAttachShader(program, shader);
#endif
}

void DetachShader(GLuint program, GLuint shader)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glDetachShader(program, shader);
#else
    pGlDetachShader(program, shader);
#endif
}

void LinkProgram(GLuint program)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glLinkProgram(program);
#else
    pGlLinkProgram(program);
#endif
}

void GetProgramiv(GLuint program, GLenum pname, GLint* params)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glGetProgramiv(program, pname, params);
#else
    pGlGetProgramiv(program, pname, params);
#endif
}

void GetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glGetProgramInfoLog(program, maxLength, length, infoLog);
#else
    pGlGetProgramInfoLog(program, maxLength, length, infoLog);
#endif
}

void DeleteProgram(GLuint program)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glDeleteProgram(program);
#else
    pGlDeleteProgram(program);
#endif
}

void UseProgram(GLuint program)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glUseProgram(program);
#else
    pGlUseProgram(program);
#endif
}

GLint GetUniformLocation(GLuint program, const GLchar* name)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    return glGetUniformLocation(program, name);
#else
    return pGlGetUniformLocation(program, name);
#endif
}

void UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glUniformMatrix4fv(location, count, transpose, value);
#else
    pGlUniformMatrix4fv(location, count, transpose, value);
#endif
}

void Uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glUniform4f(location, x, y, z, w);
#else
    pGlUniform4f(location, x, y, z, w);
#endif
}

void Uniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    glUniform3f(location, x, y, z);
#else
    pGlUniform3f(location, x, y, z);
#endif
}
} // namespace cotrx::gl
