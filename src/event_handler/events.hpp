#pragma once

#include <event_handler/event.hpp>
#include <platform/mouse_definitions.hpp>

#include <vector>
#include <string>

// Pointer events
DEFINE_EVENT_3(PointerMoveEvent, int, index, int, x, int, y);
DEFINE_EVENT_3(PointerDownEvent, int, index, int, x, int, y);
DEFINE_EVENT_3(PointerUpEvent, int, index, int, x, int, y);

// Mouse events
DEFINE_EVENT_3(MouseDownEvent, Mouse::Button, Button, int, x, int, y);
DEFINE_EVENT_3(MouseUpEvent, Mouse::Button, Button, int, x, int, y);
DEFINE_EVENT_1(MouseScrollEvent, short, Change);

DEFINE_EVENT_2(WindowResizeEvent, size_t, Width, size_t, Height);

DEFINE_EVENT_1(UpdateEvent, float, DT);