#include <renderer/opengl.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

bool m_IsOpenGLInitialised = false;

#if defined(_WIN32)
DEFINE_GL_FUNCTIONS(CPP_DEFINE_GL_FUNCTION);
#endif

void InitOpenGL()
{
	if (m_IsOpenGLInitialised)
		return;

#if defined(_WIN32)
#define INIT_DEFINE_GL_FUNCTION(a, b) b = (a)wglGetProcAddress(#b);

	DEFINE_GL_FUNCTIONS(INIT_DEFINE_GL_FUNCTION);
#elif defined(__EMSCRIPTEN__)
		if (glewInit() != GLEW_OK) 
			std::cout << "Glew failed to initialize." << std::endl;
#endif

	m_IsOpenGLInitialised = true;
}	