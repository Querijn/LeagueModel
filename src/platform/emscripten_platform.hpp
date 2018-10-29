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
		
	static size_t GetCoreCount();
	static void Sleep(size_t a_Milliseconds);
	static void SetMainLoop(LoopFunction a_Function);
	static void AtExit(AtExitFunction a_Function);
		
};

#endif