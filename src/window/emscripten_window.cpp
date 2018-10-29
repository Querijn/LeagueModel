#if defined(__EMSCRIPTEN__)

#include <window/emscripten_window.hpp>

#include <emscripten.h>
#include <emscripten/html5.h>

#include <iostream>

EmscriptenWindow::EmscriptenWindow(const WindowSettings & a_WindowSettings) :
	BaseWindow(a_WindowSettings)
{
	EmscriptenWebGLContextAttributes t_Attributes;
	emscripten_webgl_init_context_attributes(&t_Attributes);
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE t_Context = emscripten_webgl_create_context(0, &t_Attributes);
	emscripten_webgl_make_context_current(t_Context);
	emscripten_set_canvas_element_size("canvas", a_WindowSettings.Width, a_WindowSettings.Height);
}

EmscriptenWindow::~EmscriptenWindow()
{
}

void EmscriptenWindow::SwapBuffers()
{
}

bool EmscriptenWindow::HasFocus()
{
	return true;
}

bool EmscriptenWindow::RunFrame()
{
	return true;
}

#endif