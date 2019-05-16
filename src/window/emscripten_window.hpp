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

	void Resize(size_t a_Width, size_t a_Height);

	bool HasFocus();

	bool RunFrame();
	void SwapBuffers();

	size_t GetWidth() const { return m_Width; }
	size_t GetHeight() const { return m_Height; }

private:
	bool m_MouseIsInside = false;
	size_t m_Width, m_Height;
};

#endif