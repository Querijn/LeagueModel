#pragma once

#include <thread>

#define MAX_EVENT_THREAD_EVENT_COUNT 64

class EventHandler;
class Event;

class EventThread
{
private:
	enum StateType
	{
		NoState,
		Idle,
		Running,
		ForceQuit,
		ThreadEnded
	};

	friend class EventHandler;
protected:
	EventThread();
	~EventThread();

	void Detach();
	void Join();

	void AddToQueue(Event* a_Event);

	Event* m_Queue[MAX_EVENT_THREAD_EVENT_COUNT];
	size_t m_QueuePlaceIterator = 0;

	std::thread* m_Thread;
	StateType m_State;

private:
	void Thread();
};