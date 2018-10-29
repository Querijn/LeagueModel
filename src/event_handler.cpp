#include "event_handler.hpp"
#include "event_handler/event_thread.hpp"
#include "event_handler/events.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>

std::mutex EventHandler::m_QueueMutex;
std::vector<Event*> EventHandler::m_Queue;

EventThread** EventHandler::m_Threads;
size_t EventHandler::m_ThreadCount;

void EventHandler::Init()
{
	// TODO:
	// If above core count, use core count
	m_ThreadCount = 1; // Ondergrond::CurrentPlatform::GetCoreCount();

	m_Threads = new EventThread*[m_ThreadCount];
	for (size_t i = 0; i < m_ThreadCount; i++)
		m_Threads[i] = new EventThread();
}

void EventHandler::Run()
{
	// While jobs were added to the queue
	m_QueueMutex.lock();
	while (IsQueueEmpty() == false)
	{
		// Divide them over the threads
		size_t t_CurrentEventThread = 0;
		for (size_t i = 0; i < m_Queue.size(); i++)
		{
			if (m_Queue[i] == nullptr)
				break;

			auto t_CurrentThread = m_Threads[t_CurrentEventThread];
			t_CurrentThread->m_Queue[i] = m_Queue[i];
			t_CurrentEventThread = (t_CurrentEventThread + 1) % m_ThreadCount;
		}
		m_Queue.clear();
		m_QueueMutex.unlock();

		for (size_t i = m_ThreadCount - 1; i < m_ThreadCount; i--)
		{
			m_Threads[i]->m_State = EventThread::StateType::Running;

			if (i == 0)
			{
				m_Threads[i]->Join();
				break; // Should not be necessary, but here for the sanity of signedness
			}
			else m_Threads[i]->Detach();
		}

		m_QueueMutex.lock();
	}
	m_QueueMutex.unlock();
}

bool EventHandler::IsQueueEmpty()
{
	return m_Queue.size() == 0;
}

size_t EventHandler::GetThreadCount()
{
	return m_ThreadCount;
}