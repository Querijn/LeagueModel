#pragma once

#include "types.hpp"

#include <glm/vec2.hpp>

namespace LeagueModel
{
	namespace Input
	{
		enum MouseButton
		{
			Left = 0,
			Right = 1,
			None = -1,
		};

		enum KeyboardKey
		{
			KeyNone = 0,
			KeyQ = 'Q',
			KeyW = 'W',
			KeyE = 'E',
			KeyR = 'R',
			KeyT = 'T',
			KeyY = 'Y',
			KeyU = 'U',
			KeyI = 'I',
			KeyO = 'O',
			KeyP = 'P',
			KeyA = 'A',
			KeyS = 'S',
			KeyD = 'D',
			KeyF = 'F',
			KeyG = 'G',
			KeyH = 'H',
			KeyJ = 'J',
			KeyK = 'K',
			KeyL = 'L',
			KeyZ = 'Z',
			KeyX = 'X',
			KeyC = 'C',
			KeyV = 'V',
			KeyB = 'B',
			KeyN = 'N',
			KeyM = 'M',
			Key0 = '0',
			Key1 = '1',
			Key2 = '2',
			Key3 = '3',
			Key4 = '4',
			Key5 = '5',
			Key6 = '6',
			Key7 = '7',
			Key8 = '8',
			Key9 = '9',

			KeyF1 = 0x70,
			KeyF2 = 0x71,
			KeyF3 = 0x72,
			KeyF4 = 0x73,
			KeyF5 = 0x74,
			KeyF6 = 0x75,
			KeyF7 = 0x76,
			KeyF8 = 0x77,
			KeyF9 = 0x78,
			KeyF10 = 0x79,
			KeyF11 = 0x7A,
			KeyF12 = 0x7B,

			KeyCtrl = 0xA2, // VK for Left Control
			KeyShift = 0xA0, // VK for Left Shift
			KeyEscape = 0x1B, // VK for Escape

			KeyMax = 0xFF,
		};

		bool IsKeyDown(KeyboardKey inKey);
		bool IsMouseButtonDown(MouseButton inButton);
		bool IsKeyPressed(KeyboardKey inKey);
		bool IsMouseButtonPressed(MouseButton inButton);
		bool HasScrolled();
		f32 ScrollDelta();

		const glm::ivec2& GetMousePosition();

		void Update();
	}
}
