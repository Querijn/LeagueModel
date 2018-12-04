#pragma once

#include <event_handler/event.hpp>
#include <helper.hpp>

#include <mutex>
#include <iostream>

class EventThread;

class EventHandler
{
public:
	static void Init();
	static void CleanUp();

	template<typename T, typename... TArgs>
	static void EmitEvent(TArgs... a_Args)
	{
		AssertDerivedFrom(Event, T, "EmitEvents only takes events!");

		Event* t_Event = reinterpret_cast<Event*>(new T(a_Args...));

		std::lock_guard<std::mutex> t_Lock(m_QueueMutex);
		m_Queue.push_back(t_Event);
	}

	template<typename T, typename... TArgs>
	static void EmitEventImmediately(TArgs... a_Args)
	{
		AssertDerivedFrom(Event, T, "EmitEvents only takes events!");

		T t_Event(a_Args...);
		t_Event.Run();
	}

	static void Run();

	static size_t GetThreadCount();

private:
	EventHandler() = delete;

	static bool IsQueueEmpty();

	static std::mutex m_QueueMutex;
	static std::vector<Event*> m_Queue;

	static EventThread** m_Threads;
	static size_t m_ThreadCount;
};