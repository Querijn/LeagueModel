#pragma once
#ifdef _WIN32

#include <Windows.h>

#include <window/base_window.hpp>

class WindowsWindow : public BaseWindow
{
public:
	using WindowSettings = BaseWindow::WindowSettings;
	using ContextSettings = BaseWindow::ContextSettings;
	using ViewModeType = BaseWindow::ViewModeType;

	WindowsWindow(const WindowSettings& a_WindowSettings);
	~WindowsWindow();

	bool RunFrame();
	void SwapBuffers();

	size_t GetWidth() const { return m_Width; }
	size_t GetHeight() const { return m_Height; }

private:
	static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

	bool SetFullscreen(unsigned width, unsigned height);
	void SetTitle(const char * title);
	void SetVerticalSync(bool a_UseVSync);
	int HasFocus();
	void CreateOpenGLContext();
	void GetContextData();
	void SetupPixelFormat(HDC a_HDC = nullptr);
	void SetupModernPixelFormat();

	HWND m_Window;
	HGLRC m_HRC;
	HDC m_DisplayContext;

	bool m_Quit = false;
	bool m_Initialised = false;

	bool m_Resized;
	size_t m_Width;
	size_t m_Height;

	bool m_Fullscreen;
};

#endif