#ifdef _WIN32

#include <renderer/opengl.hpp>

#include "window/windows_window.hpp"
#include "event_handler.hpp"

#include "event_handler/events.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#define WGL_WGLEXT_PROTOTYPES
#include <GL/wglext.h>

#include <stdio.h>
#include <WindowsX.h>

static PFNWGLCHOOSEPIXELFORMATEXTPROC glChoosePixelFormatARB;
static PFNWGLCREATECONTEXTATTRIBSARBPROC glCreateContextAttribsARB;

WindowsWindow::WindowsWindow(const WindowSettings & a_WindowSettings) :
	BaseWindow(a_WindowSettings)
{
	if (m_Initialised)
	{
		__debugbreak();
		return;
	}

	m_Quit = FALSE;

	GetContextData();

	WNDCLASSEX t_WindowClass = { 0 };
	t_WindowClass.cbSize = sizeof(t_WindowClass);
	t_WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	t_WindowClass.lpfnWndProc = &WindowsWindow::WindowProcedure;
	t_WindowClass.hInstance = GetModuleHandle(NULL);
	t_WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	t_WindowClass.lpszClassName = "OndergrondApplication";

	if (!RegisterClassEx(&t_WindowClass))
	{
		__debugbreak();
		return;
	}

	size_t t_Width = a_WindowSettings.Width;
	size_t t_Height = a_WindowSettings.Height;
	DWORD t_Style = 0;

	switch (a_WindowSettings.ViewMode)
	{
	case BaseWindow::ViewModeType::Windowed:
	{
		RECT t_Sizes = { 0 };
		t_Sizes.right = t_Width;
		t_Sizes.bottom = t_Height;

		t_Style = WS_OVERLAPPEDWINDOW;
		AdjustWindowRect(&t_Sizes, t_Style, FALSE);
		t_Width = t_Sizes.right - t_Sizes.left;
		t_Height = t_Sizes.bottom - t_Sizes.top;
		break;
	}

	case BaseWindow::ViewModeType::Fullscreen:
	{
		RECT t_WindowSize;
		GetClientRect(GetDesktopWindow(), &t_WindowSize);
		m_Fullscreen = TRUE;
		t_Style = WS_POPUP | WS_VISIBLE;

		if (!SetFullscreen(t_Width, t_Height))
		{
			m_Fullscreen = FALSE;
			t_Width = t_WindowSize.right - t_WindowSize.left;
			t_Height = t_WindowSize.bottom - t_WindowSize.top;
		}
		break;
	}

	case BaseWindow::ViewModeType::Borderless:
	{
		RECT t_WindowSize;
		GetClientRect(GetDesktopWindow(), &t_WindowSize);

		t_Style = WS_POPUP | WS_VISIBLE;
		t_Width = t_WindowSize.right - t_WindowSize.left;
		t_Height = t_WindowSize.bottom - t_WindowSize.top;
		break;
	}

	default:
		UnregisterClass("OndergrondApplication", GetModuleHandle(NULL));
		__debugbreak();
		return;
	}

	m_Window = CreateWindowEx(0, "OndergrondApplication", a_WindowSettings.Title ? a_WindowSettings.Title : "OndergrondApplication", t_Style, CW_USEDEFAULT, CW_USEDEFAULT, t_Width, t_Height, NULL, NULL, NULL, NULL);
	if (!m_Window)
	{
		UnregisterClass("OndergrondApplication", GetModuleHandle(NULL));
		__debugbreak();
		return;
	}
	
	SetWindowLongPtr(m_Window, GWLP_USERDATA, (LONG_PTR)this);
	CreateOpenGLContext();

	if (a_WindowSettings.ViewMode == BaseWindow::ViewModeType::Windowed)
	{
		ShowWindow(m_Window, SW_RESTORE);
		UpdateWindow(m_Window);
		SetForegroundWindow(m_Window);
		SetFocus(m_Window);
	}

	SetVerticalSync(true);

	m_Initialised = TRUE;
}

WindowsWindow::~WindowsWindow()
{
	m_Initialised = FALSE;

	if (m_Quit)
	{
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(m_HRC);
	}

	DestroyWindow(m_Window);
	UnregisterClass("OndergrondApplication", GetModuleHandle(NULL));

	if (m_Fullscreen)
		ChangeDisplaySettings(NULL, 0);
	m_Fullscreen = FALSE;
}

void WindowsWindow::SetupPixelFormat(HDC a_HDC)
{
	auto t_DisplayContext = a_HDC ? a_HDC : m_DisplayContext;

	int t_PixelFormats;
	static PIXELFORMATDESCRIPTOR t_Descriptor = { 0 };
	t_Descriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	t_Descriptor.nVersion = 1;
	t_Descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	t_Descriptor.iPixelType = PFD_TYPE_RGBA;
	t_Descriptor.cColorBits = 32;
	t_Descriptor.cDepthBits = 24;
	t_Descriptor.cStencilBits = 8;
	t_Descriptor.iLayerType = PFD_MAIN_PLANE;

	t_PixelFormats = ChoosePixelFormat(t_DisplayContext, &t_Descriptor);
	SetPixelFormat(t_DisplayContext, t_PixelFormats, &t_Descriptor);
}

void WindowsWindow::SetupModernPixelFormat()
{
	int t_PixelFormat;
	UINT t_PixelFormats;
	PIXELFORMATDESCRIPTOR t_Descriptor;
	float t_FloatAttributes[2];
	int t_Attributes[] =
	{
		WGL_DOUBLE_BUFFER_ARB, TRUE,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_RED_BITS_ARB, 8,
		WGL_GREEN_BITS_ARB, 8,
		WGL_BLUE_BITS_ARB, 8,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 8,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_SAMPLE_BUFFERS_ARB, 1,
		WGL_SAMPLES_ARB, 0,
		0, 0,
	};

	t_Attributes[19] = m_Settings.Context.Samples == 0 ? 1 : m_Settings.Context.Samples;
	t_FloatAttributes[0] = t_FloatAttributes[1] = 0.0f;

	glChoosePixelFormatARB(m_DisplayContext, t_Attributes, t_FloatAttributes, 1, &t_PixelFormat, &t_PixelFormats);

	DescribePixelFormat(m_DisplayContext, t_PixelFormat, sizeof(t_Descriptor), &t_Descriptor);
	SetPixelFormat(m_DisplayContext, t_PixelFormat, &t_Descriptor);
}

void WindowsWindow::GetContextData()
{
	HWND t_Dummy;
	HDC t_Context;
	HGLRC t_GLContext;
	WNDCLASSEX t_Class = { 0 };
	t_Class.cbSize = sizeof(t_Class);
	t_Class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	t_Class.lpfnWndProc = DefWindowProc;
	t_Class.hInstance = GetModuleHandle(NULL);
	t_Class.hCursor = LoadCursor(NULL, IDC_ARROW);
	t_Class.lpszClassName = "Dummy Window";

	RegisterClassEx(&t_Class);
	t_Dummy = CreateWindowEx(0, "Dummy Window", "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1, 1, NULL, NULL, NULL, NULL);

	ShowWindow(t_Dummy, SW_HIDE);
	t_Context = GetDC(t_Dummy);
	SetupPixelFormat(t_Context);
	t_GLContext = wglCreateContext(t_Context);
	wglMakeCurrent(t_Context, t_GLContext);

#ifdef _WIN32
	glChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATEXTPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	glCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
#endif

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(t_GLContext);
	DestroyWindow(t_Dummy);
	UnregisterClass("Dummy Window", GetModuleHandle(NULL));
}

void WindowsWindow::CreateOpenGLContext()
{
	m_DisplayContext = GetDC(m_Window);

	if (glChoosePixelFormatARB && glCreateContextAttribsARB)
	{
		SetupModernPixelFormat();

		int t_Attributes[] = 
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 0,
			WGL_CONTEXT_MINOR_VERSION_ARB, 0,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef DEBUG
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
			0
		};

		t_Attributes[1] = m_Settings.Context.Version.Major;
		t_Attributes[3] = m_Settings.Context.Version.Minor;

		m_HRC = glCreateContextAttribsARB(m_DisplayContext, NULL, t_Attributes);
		wglMakeCurrent(m_DisplayContext, m_HRC);
	}
	else
	{
		SetupPixelFormat();
		m_HRC = wglCreateContext(m_DisplayContext);
		wglMakeCurrent(m_DisplayContext, m_HRC);
	}

	InitOpenGL();
}
	
LRESULT WindowsWindow::WindowProcedure(HWND a_Window, UINT a_Message, WPARAM a_WParam, LPARAM a_LParam)
{
	WindowsWindow* t_Window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(a_Window, GWLP_USERDATA));
	switch (a_Message)
	{
	case WM_SYSCOMMAND:
		switch (a_WParam)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
		return 0;

	case WM_MOUSEWHEEL:
		EventHandler::EmitEvent<MouseScrollEvent>(GET_WHEEL_DELTA_WPARAM(a_WParam));
		return 0;

	case WM_MOUSEMOVE:
		EventHandler::EmitEvent<PointerMoveEvent>(0, GET_X_LPARAM(a_LParam), GET_Y_LPARAM(a_LParam));
		return 0;

	case WM_LBUTTONDOWN:
		EventHandler::EmitEvent<PointerDownEvent>(0, GET_X_LPARAM(a_LParam), GET_Y_LPARAM(a_LParam));
		EventHandler::EmitEvent<MouseDownEvent>(Mouse::Button::Left, GET_X_LPARAM(a_LParam), GET_Y_LPARAM(a_LParam));
		return 0;

	case WM_LBUTTONUP:
		EventHandler::EmitEvent<PointerUpEvent>(0, GET_X_LPARAM(a_LParam), GET_Y_LPARAM(a_LParam));
		EventHandler::EmitEvent<MouseUpEvent>(Mouse::Button::Left, GET_X_LPARAM(a_LParam), GET_Y_LPARAM(a_LParam));
		return 0;

	case WM_RBUTTONDOWN:
		EventHandler::EmitEvent<MouseDownEvent>(Mouse::Button::Right, GET_X_LPARAM(a_LParam), GET_Y_LPARAM(a_LParam));
		return 0;

	case WM_RBUTTONUP:
		EventHandler::EmitEvent<MouseUpEvent>(Mouse::Button::Right, GET_X_LPARAM(a_LParam), GET_Y_LPARAM(a_LParam));
		return 0;

	case WM_MBUTTONDOWN:
		EventHandler::EmitEvent<MouseDownEvent>(Mouse::Button::Middle, GET_X_LPARAM(a_LParam), GET_Y_LPARAM(a_LParam));
		return 0;

	case WM_MBUTTONUP:
		EventHandler::EmitEvent<MouseUpEvent>(Mouse::Button::Middle, GET_X_LPARAM(a_LParam), GET_Y_LPARAM(a_LParam));
		return 0;

	case WM_CREATE:
		return 0;

	case WM_CLOSE:
	case WM_DESTROY:
	case WM_QUIT:
		t_Window->m_Quit = true;
		return 0;

	case WM_SIZE:
		if (a_WParam != SIZE_MAXHIDE && a_WParam != SIZE_MINIMIZED)
		{
			t_Window->m_Width = LOWORD(a_LParam);
			t_Window->m_Height = HIWORD(a_LParam);
			t_Window->m_Resized = true;
		}
		return 0;
	}

	return DefWindowProc(a_Window, a_Message, a_WParam, a_LParam);
}

bool WindowsWindow::SetFullscreen(unsigned a_Width, unsigned a_Height)
{
	DEVMODE t_DevMode;
	memset(&t_DevMode, 0, sizeof(t_DevMode));
	t_DevMode.dmSize = sizeof(DEVMODE);
	t_DevMode.dmPelsWidth = a_Width;
	t_DevMode.dmPelsHeight = a_Height;
	t_DevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

	return ChangeDisplaySettings(&t_DevMode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;
}

void WindowsWindow::SetTitle(const char *a_Title)
{
	SetWindowText(m_Window, a_Title);
}

void WindowsWindow::SetVerticalSync(bool a_UseVSync)
{
	static bool(APIENTRY *wglSwapIntervalEXT)(int) = (bool(APIENTRY *)(int))wglGetProcAddress("wglSwapIntervalEXT");

	if (wglSwapIntervalEXT) wglSwapIntervalEXT(a_UseVSync);
}

void WindowsWindow::SwapBuffers(void)
{
	::SwapBuffers(m_DisplayContext);
}

int WindowsWindow::HasFocus(void)
{
	return GetFocus() == m_Window;
}

bool WindowsWindow::RunFrame(void)
{
	MSG msg;
	while (PeekMessage(&msg, m_Window, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (m_Resized)
	{
		EventHandler::EmitEvent<WindowResizeEvent>(m_Width, m_Height);
		m_Resized = false;
	}

	return !m_Quit;
}

#endif