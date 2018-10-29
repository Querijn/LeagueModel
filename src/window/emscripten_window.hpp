#pragma once
#if defined(__EMSCRIPTEN__)

#include <stddef.h>
#include <window/base_window.hpp>

class EmscriptenWindow : public BaseWindow
{
public:
	using WindowSettings = BaseWindow::WindowSettings;
	using ContextSettings = BaseWindow::ContextSettings;
	using ViewModeType = BaseWindow::ViewModeType;

	EmscriptenWindow(const WindowSettings& a_WindowSettings);
	~EmscriptenWindow();

	bool HasFocus();

	bool RunFrame();
	void SwapBuffers();

	// TODO
	size_t GetWidth() const { return 800; }
	size_t GetHeight() const { return 600; }
};

#endif