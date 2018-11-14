#if defined(__EMSCRIPTEN__)

#include <window/emscripten_window.hpp>
#include "event_handler.hpp"
#include "event_handler/events.hpp"

#include <emscripten.h>
#include <emscripten/html5.h>

#include <iostream>

EM_BOOL OnMouseMove(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
	Mouse::Button t_Button;
	switch (e->button)
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

	switch (eventType)
	{
	case EMSCRIPTEN_EVENT_MOUSEDOWN:
		EventHandler::EmitEvent<MouseDownEvent>(t_Button, e->canvasX, e->canvasY);
		break;

	case EMSCRIPTEN_EVENT_MOUSEUP:
		EventHandler::EmitEvent<MouseUpEvent>(t_Button, e->canvasX, e->canvasY);
		break;

	case EMSCRIPTEN_EVENT_MOUSEMOVE:
		EventHandler::EmitEvent<PointerMoveEvent>(0, e->canvasX, e->canvasY);
		break;
	}

	return 0;
}

EM_BOOL OnTouchCallback(int eventType, const EmscriptenTouchEvent *e, void *userData)
{
	for (int i = 0; i < e->numTouches; ++i)
	{
		const EmscriptenTouchPoint *t = &e->touches[i];
		if (!t->isChanged) continue;

		switch (eventType)
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

EmscriptenWindow::EmscriptenWindow(const WindowSettings & a_WindowSettings) :
	BaseWindow(a_WindowSettings)
{
	EmscriptenWebGLContextAttributes t_Attributes;
	emscripten_webgl_init_context_attributes(&t_Attributes);
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE t_Context = emscripten_webgl_create_context(0, &t_Attributes);
	emscripten_webgl_make_context_current(t_Context);
	emscripten_set_canvas_element_size("canvas", a_WindowSettings.Width, a_WindowSettings.Height);
	
	emscripten_set_mousedown_callback("#canvas", this, 1, OnMouseMove);
	emscripten_set_mouseup_callback("#canvas", this, 1, OnMouseMove);
	emscripten_set_mousemove_callback("#canvas", this, 1, OnMouseMove);

	emscripten_set_touchstart_callback("#canvas", this, 1, OnTouchCallback);
	emscripten_set_touchend_callback("#canvas", this, 1, OnTouchCallback);
	emscripten_set_touchmove_callback("#canvas", this, 1, OnTouchCallback);
	emscripten_set_touchcancel_callback("#canvas", this, 1, OnTouchCallback);

	emscripten_set_wheel_callback("#canvas", this, 1, [](int eventType, const EmscriptenWheelEvent *e, void *userData)
	{
		EventHandler::EmitEvent<MouseScrollEvent>(-e->deltaY);
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