#ifdef __EMSCRIPTEN__
#include "platform/emscripten_platform.hpp"

#include <emscripten.h>
#include <algorithm>

LoopFunction m_LoopFunction;

EmscriptenPlatform::EmscriptenPlatform()
{
}

bool EmscriptenPlatform::IsNative()
{
	return false;
}

bool EmscriptenPlatform::IsWeb()
{
	return true;
}

size_t EmscriptenPlatform::GetCoreCount()
{
	return 1;
}

void EmscriptenPlatform::Sleep(size_t a_Milliseconds)
{

}

void Loop()
{
	if (!m_LoopFunction()) 
		emscripten_cancel_main_loop();
}

void EmscriptenPlatform::SetMainLoop(LoopFunction a_Function)
{
	m_LoopFunction = a_Function;
	emscripten_set_main_loop(Loop, 0, 0);
}

void EmscriptenPlatform::AtExit(AtExitFunction a_Function)
{
	std::atexit(a_Function);
}
#endif