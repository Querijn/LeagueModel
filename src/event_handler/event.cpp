#include "event_handler/event.hpp"

std::map<const char*, std::vector<Event::ListenerFunction>> Event::m_Listeners;
std::map<const char*, std::vector<void*>> Event::m_ListenerContexts;
std::mutex Event::m_Mutex;

Event::Event()
{
}

void Event::Run()
{
	const char* t_Name = GetName();
	auto& t_Listeners = m_Listeners[t_Name];
	for (size_t i = 0; i < t_Listeners.size(); i++)
	{
		t_Listeners[i](this, m_ListenerContexts[t_Name][i]);
	}
}