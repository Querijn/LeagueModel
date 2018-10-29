#pragma once

#ifdef _WIN32
#include <Windows.h> 

#include <GL/GL.h>
#include <GL/glext.h> 
#include <GL/wglext.h>

#include "stb_image.h"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GL/glew.h>
#endif

#include <cstdint>
#include <iostream>

#if defined(_DEBUG)
#pragma region DebugDefines
#define GL(a) do \
{\
	glGetError();\
	do { a; } while(0);\
	GLenum t_Error = glGetError(); \
	const char* t_ErrorMessage;\
	switch (t_Error)\
	{\
		case GL_NO_ERROR:\
			break;\
\
		case GL_INVALID_ENUM:\
			t_ErrorMessage = "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.";\
			break;\
\
		case GL_INVALID_VALUE:\
			t_ErrorMessage = "GL_INVALID_VALUE: A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.";\
			break;\
\
		case GL_INVALID_OPERATION:\
			t_ErrorMessage = "GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.";\
			break;\
\
		case GL_INVALID_FRAMEBUFFER_OPERATION:\
			t_ErrorMessage = "GL_INVALID_FRAMEBUFFER_OPERATION: The command is trying to render to or read from the framebuffer while the currently bound framebuffer is not framebuffer complete (i.e.the return value from glCheckFramebufferStatus is not GL_FRAMEBUFFER_COMPLETE). The offending command is ignored and has no other side effect than to set the error flag.";\
			break;\
\
		case GL_OUT_OF_MEMORY:\
			t_ErrorMessage = "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";\
			break;\
\
		default:\
			t_ErrorMessage = "Unknown error message.";\
			break;\
	}\
\
	if (t_Error == GL_NO_ERROR) break;\
	std::cerr << "GL Error at " << __FUNCTION__ << ":" << __LINE__ << ": " << t_ErrorMessage << " (" << t_Error << ")" << std::endl;  \
} while (0);

#define GL_RET(a, b) [&]() -> b \
{\
	glGetError();\
	b t_Result = a;\
	GLenum t_Error = glGetError();\
	const char* t_ErrorMessage;\
	switch (t_Error)\
	{\
		case GL_NO_ERROR:\
			return t_Result;\
\
		case GL_INVALID_ENUM:\
			t_ErrorMessage = "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.";\
			break;\
\
		case GL_INVALID_VALUE:\
			t_ErrorMessage = "GL_INVALID_VALUE: A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.";\
			break;\
\
		case GL_INVALID_OPERATION:\
			t_ErrorMessage = "GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.";\
			break;\
\
		case GL_INVALID_FRAMEBUFFER_OPERATION:\
			t_ErrorMessage = "GL_INVALID_FRAMEBUFFER_OPERATION: The command is trying to render to or read from the framebuffer while the currently bound framebuffer is not framebuffer complete (i.e.the return value from glCheckFramebufferStatus is not GL_FRAMEBUFFER_COMPLETE). The offending command is ignored and has no other side effect than to set the error flag.";\
			break;\
\
		case GL_OUT_OF_MEMORY:\
			t_ErrorMessage = "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";\
			break;\
\
		default:\
			t_ErrorMessage = "Unknown error message.";\
			break;\
	}\
\
	if (t_Error == GL_NO_ERROR) return t_Result;\
	std::cout << "GL Error at " << __FUNCTION__ << ":" << __LINE__ << ": " << t_ErrorMessage << " (" << t_Error << ")" << std::endl;  \
	return t_Result;\
}();
#pragma endregion 
#else
#pragma region ProdDefines

#define GL(a) a
#define GL_RET(a, b) a

#pragma endregion
#endif

#if defined(_WIN32)
#define HEADER_DEFINE_GL_FUNCTION(a, b) extern a b;
#define CPP_DEFINE_GL_FUNCTION(a, b) a b;

#define DEFINE_GL_FUNCTIONS(def) \
	def(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);\
	def(PFNGLCOMPRESSEDTEXIMAGE1DPROC, glCompressedTexImage1D);\
	def(PFNGLCOMPRESSEDTEXIMAGE2DPROC, glCompressedTexImage2D);\
	def(PFNGLCOMPRESSEDTEXIMAGE3DPROC, glCompressedTexImage3D);\
	def(PFNGLTEXIMAGE3DPROC, glTexImage3D);\
	def(PFNGLUNIFORM3FPROC, glUniform3f);\
	def(PFNGLGENBUFFERSPROC, glGenBuffers);\
	def(PFNGLBINDBUFFERPROC, glBindBuffer);\
	def(PFNGLBUFFERDATAPROC, glBufferData);\
	def(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);\
	def(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);\
	def(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);\
	def(PFNGLCREATEPROGRAMPROC, glCreateProgram);\
	def(PFNGLATTACHSHADERPROC, glAttachShader);\
	def(PFNGLLINKPROGRAMPROC, glLinkProgram);\
	def(PFNGLGETPROGRAMIVPROC, glGetProgramiv);\
	def(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);\
	def(PFNGLDELETESHADERPROC, glDeleteShader);\
	def(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);\
	def(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);\
	def(PFNGLDELETEPROGRAMPROC, glDeleteProgram);\
	def(PFNGLCREATESHADERPROC, glCreateShader);\
	def(PFNGLSHADERSOURCEPROC, glShaderSource);\
	def(PFNGLCOMPILESHADERPROC, glCompileShader);\
	def(PFNGLGETSHADERIVPROC, glGetShaderiv);\
	def(PFNGLDETACHSHADERPROC, glDetachShader);\
	def(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);\
	def(PFNGLACTIVETEXTUREPROC, glActiveTexture);\
	def(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);\
	def(PFNGLUSEPROGRAMPROC, glUseProgram);\
	def(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation);\
	def(PFNGLUNIFORM1IPROC, glUniform1i);\
	def(PFNGLUNIFORM1IVPROC, glUniform1iv);\
	def(PFNGLUNIFORM1FPROC, glUniform1f);\
	def(PFNGLUNIFORM1FVPROC, glUniform1fv);\
	def(PFNGLUNIFORM2FVPROC, glUniform2fv);\
	def(PFNGLUNIFORM3FVPROC, glUniform3fv);\
	def(PFNGLUNIFORM4FVPROC, glUniform4fv);\
	def(PFNGLUNIFORMMATRIX3FVPROC, glUniformMatrix3fv);\
	def(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);

DEFINE_GL_FUNCTIONS(HEADER_DEFINE_GL_FUNCTION);
#endif

void InitOpenGL();