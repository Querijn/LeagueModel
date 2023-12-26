#include "types.hpp"
#include "simple_input.hpp"

#include <cmath>

namespace LeagueModel
{
	namespace Input
	{
		static bool g_initialised = false;

		static bool g_keyDown[(size_t)KeyboardKey::KeyMax] = { false };
		static bool g_keyWasDown[(size_t)KeyboardKey::KeyMax] = { false };
		static bool g_keyPressed[(size_t)KeyboardKey::KeyMax] = { false };

		static bool g_mouseDown[16] = { false };
		static bool g_mouseWasDown[16] = { false };
		static bool g_mousePressed[16] = { false };
		static f32 g_mouseScroll = 0;
		static glm::ivec2 g_mousePosition = glm::ivec2(-1);

		void OnKeyDown(u32 virtualKey)
		{
			if (virtualKey < (size_t)KeyboardKey::KeyMax)
				g_keyDown[(int)virtualKey] = true;
		}

		void OnKeyUp(u32 virtualKey)
		{
			if (virtualKey < (size_t)KeyboardKey::KeyMax)
				g_keyDown[(int)virtualKey] = false;
		}

		void OnMouseDown(u32 button)
		{
			if (button < 16)
				g_mouseDown[(int)button] = true;
		}

		void OnMouseUp(u32 button)
		{
			if (button < 16)
				g_mouseDown[(int)button] = false;
		}

		void OnMouseScroll(f32 scrollDelta)
		{
			g_mouseScroll += scrollDelta;
		}

		bool IsKeyDown(KeyboardKey inKey)
		{
			return g_keyDown[(int)inKey];
		}

		bool IsMouseButtonDown(MouseButton inButton)
		{
			return g_mouseDown[(int)inButton];
		}

		bool IsKeyPressed(KeyboardKey inKey)
		{
			return g_keyPressed[(int)inKey];
		}

		bool IsMouseButtonPressed(MouseButton inButton)
		{
			return g_mousePressed[(int)inButton];
		}

		bool HasScrolled()
		{
			return abs(g_mouseScroll) > 0.0001f;
		}

		f32 ScrollDelta()
		{
			f32 scroll = g_mouseScroll;
			g_mouseScroll = 0.0f;
			return scroll;
		}

		const glm::ivec2& GetMousePosition()
		{
			return g_mousePosition;
		}

		void Update()
		{
			for (int i = 0; i < 16; i++)
				g_mousePressed[i] = g_mouseDown[i] == false && g_mouseWasDown[i] == true;

			for (int i = 0; i < (size_t)KeyboardKey::KeyMax; i++)
				g_keyPressed[i] = g_keyDown[i] == false && g_keyWasDown[i] == true;

			memcpy(g_mouseWasDown, g_mouseDown, sizeof(bool) * 16);
			memcpy(g_keyWasDown, g_keyDown, sizeof(bool) * (size_t)KeyboardKey::KeyMax);
		}
	}
}
