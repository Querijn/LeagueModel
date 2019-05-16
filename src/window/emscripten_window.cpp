#if defined(__EMSCRIPTEN__)

#include <window/emscripten_window.hpp>
#include "event_handler.hpp"
#include "event_handler/events.hpp"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <iostream>

EM_BOOL OnMouseMove(int a_EventType, const EmscriptenMouseEvent *a_Event, void *a_UserData)
{
	Mouse::Button t_Button;
	switch (a_Event->button)
	{
	default:
	case 0:
		t_Button = Mouse::Button::Left;
		break;
	case 1:
		t_Button = Mouse::Button::Middle;
		break;
	case 2:
		t_Button = Mouse::Button::Right;
		break;
	}

	switch (a_EventType)
	{
	case EMSCRIPTEN_EVENT_MOUSEDOWN:
		EventHandler::EmitEvent<MouseDownEvent>(t_Button, a_Event->canvasX, a_Event->canvasY);
		break;

	case EMSCRIPTEN_EVENT_MOUSEUP:
		EventHandler::EmitEvent<MouseUpEvent>(t_Button, a_Event->canvasX, a_Event->canvasY);
		break;

	case EMSCRIPTEN_EVENT_MOUSEMOVE:
		EventHandler::EmitEvent<PointerMoveEvent>(0, a_Event->canvasX, a_Event->canvasY);
		break;
	}

	return 0;
}

EM_BOOL OnTouchCallback(int a_EventType, const EmscriptenTouchEvent *a_Event, void *a_UserData)
{
	for (int i = 0; i < a_Event->numTouches; ++i)
	{
		const EmscriptenTouchPoint *t = &a_Event->touches[i];
		if (!t->isChanged) continue;

		switch (a_EventType)
		{
		case EMSCRIPTEN_EVENT_TOUCHSTART:
			EventHandler::EmitEvent<PointerDownEvent>(t->identifier, t->canvasX, t->canvasY);
			break;

		case EMSCRIPTEN_EVENT_TOUCHCANCEL:
		case EMSCRIPTEN_EVENT_TOUCHEND:
			EventHandler::EmitEvent<PointerUpEvent>(t->identifier, t->canvasX, t->canvasY);
			break;

		case EMSCRIPTEN_EVENT_TOUCHMOVE:
			EventHandler::EmitEvent<PointerMoveEvent>(t->identifier, t->canvasX, t->canvasY);
			break;
		}
	}

	return 0;
}

void EmscriptenWindow::Resize(size_t a_Width, size_t a_Height)
{
	printf("Resized window to width/height: %zu, %zu\n", a_Width, a_Height);
	m_Width = a_Width * 0.8;
	m_Height = a_Height;
	EventHandler::EmitEvent<WindowResizeEvent>(m_Width, m_Height);
	emscripten_set_canvas_element_size("canvas", m_Width, m_Height);
}

EmscriptenWindow::EmscriptenWindow(const WindowSettings & a_WindowSettings) :
	BaseWindow(a_WindowSettings)
{
	EmscriptenWebGLContextAttributes t_Attributes;
	emscripten_webgl_init_context_attributes(&t_Attributes);
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE t_Context = emscripten_webgl_create_context(0, &t_Attributes);
	emscripten_webgl_make_context_current(t_Context);

	int t_Width;
	int t_Height;
	emscripten_get_canvas_element_size("canvas", &t_Width, &t_Height);
	Resize(t_Width, t_Height);

	emscripten_set_resize_callback(nullptr, this, 1, [](int a_EventType, const EmscriptenUiEvent *a_Event, void *a_UserData)
	{
		EmscriptenWindow* t_Window = (EmscriptenWindow*)a_UserData;
		t_Window->Resize(a_Event->windowInnerWidth, a_Event->windowInnerHeight);
		return 0;
	});
	
	emscripten_set_mousedown_callback("#canvas", this, 1, OnMouseMove);
	emscripten_set_mouseup_callback("#canvas", this, 1, OnMouseMove);
	emscripten_set_mousemove_callback("#canvas", this, 1, OnMouseMove);

	emscripten_set_touchstart_callback("#canvas", this, 1, OnTouchCallback);
	emscripten_set_touchend_callback("#canvas", this, 1, OnTouchCallback);
	emscripten_set_touchmove_callback("#canvas", this, 1, OnTouchCallback);
	emscripten_set_touchcancel_callback("#canvas", this, 1, OnTouchCallback);

	emscripten_set_wheel_callback("#canvas", this, 1, [](int a_EventType, const EmscriptenWheelEvent *a_Event, void *a_UserData)
	{
		EventHandler::EmitEvent<MouseScrollEvent>(-a_Event->deltaY);
		return 0;
	});
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