#pragma once
#ifdef __EMSCRIPTEN__

#include <stddef.h>
#include <platform/base_platform.hpp>

class EmscriptenPlatform
{
public:
	EmscriptenPlatform();

	static bool IsNative();
	static bool IsWeb();
		
	static void SetMainLoop(LoopFunction a_Function);

	static double GetTimeSinceStart();
};

#endif