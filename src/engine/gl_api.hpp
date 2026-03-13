#pragma once

#include <SDL.h>

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#endif

namespace cotrx::gl
{
bool Load();
void Reset();

void GenVertexArrays(GLsizei count, GLuint* arrays);
void BindVertexArray(GLuint array);
void DeleteVertexArrays(GLsizei count, const GLuint* arrays);

void GenBuffers(GLsizei count, GLuint* buffers);
void BindBuffer(GLenum target, GLuint buffer);
void BufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
void DeleteBuffers(GLsizei count, const GLuint* buffers);

void EnableVertexAttribArray(GLuint index);
void VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

GLuint CreateShader(GLenum shaderType);
void ShaderSource(GLuint shader, GLsizei count, const GLchar* const* source, const GLint* length);
void CompileShader(GLuint shader);
void GetShaderiv(GLuint shader, GLenum pname, GLint* params);
void GetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void DeleteShader(GLuint shader);

GLuint CreateProgram();
void AttachShader(GLuint program, GLuint shader);
void DetachShader(GLuint program, GLuint shader);
void LinkProgram(GLuint program);
void GetProgramiv(GLuint program, GLenum pname, GLint* params);
void GetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
void DeleteProgram(GLuint program);
void UseProgram(GLuint program);
GLint GetUniformLocation(GLuint program, const GLchar* name);
void UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
void Uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void Uniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z);
} // namespace cotrx::gl
