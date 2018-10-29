#include "event_handler/event_thread.hpp"
#include "event_handler/event.hpp"
#include "event_handler.hpp"

#include <iostream>

EventThread::EventThread() :
	m_State(StateType::NoState)
{
	if (EventHandler::GetThreadCount() != 1)
		m_Thread = new std::thread([&]() { Thread(); });
	else m_Thread = nullptr;

	for (size_t i = 0; i < MAX_EVENT_THREAD_EVENT_COUNT; i++)
		m_Queue[i] = nullptr;
}

EventThread::~EventThread()
{
	delete m_Thread;
}

void EventThread::Detach()
{
	m_State = StateType::Running;
	if (m_Thread) m_Thread->detach();
}

void EventThread::Join()
{
	m_State = StateType::Running;
	if (m_Thread) m_Thread->join();
	else Thread();
}

void EventThread::AddToQueue(Event * a_Event)
{
	m_Queue[m_QueuePlaceIterator] = a_Event;
	m_QueuePlaceIterator++;
}

void EventThread::Thread()
{
	while (m_State != StateType::ForceQuit)
	{
		// TODO: Make threading lock check
		while (m_State != StateType::Running) {}

		// TODO: Lock adding Events

		// For each Event
		for (size_t i = 0; i < MAX_EVENT_THREAD_EVENT_COUNT; i++)
		{
			auto t_Event = m_Queue[i];
			if (t_Event == nullptr) continue;

			// std::cout << "Sending event: " << t_Event->GetName() << std::endl;

			// Run them
			t_Event->Run();
			m_Queue[i] = nullptr;
		}

		m_State = StateType::Idle;
		if (EventHandler::GetThreadCount() == 1) break;
	}

	m_State = StateType::ThreadEnded;
}