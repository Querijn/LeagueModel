#ifdef __EMSCRIPTEN__
#include "league_model/platform/emscripten_platform.hpp"

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

double EmscriptenPlatform::GetTimeSinceStart()
{
	return 0.001 * emscripten_get_now();
}
#endif