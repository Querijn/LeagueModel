#pragma once

#include <map>
#include <mutex>
#include <vector>

class Event;
typedef void(*EventListenerFunction)(const Event*, void*);

#define ADD_MEMBER_FUNCTION_EVENT_LISTENER(EventClassName, CastName, FunctionName) EventClassName::AddListener([](const EventClassName* a_Event, void* a_Context)\
{\
	auto t_Self = static_cast<CastName*>(a_Context);\
	t_Self->FunctionName(a_Event);\
}, this);

#define DEFINE_EVENT(Name) class Name : public Event\
{\
public:\
	typedef void(*ListenerFunction)(const Name*, void*);\
	\
	static void AddListener(Name::ListenerFunction a_Listener)\
	{\
		static const char* t_Name = #Name; \
		m_Listeners[t_Name].push_back(reinterpret_cast<Event::ListenerFunction>(a_Listener)); \
		m_ListenerContexts[t_Name].push_back(nullptr); \
	}\
	\
	template<typename T>\
	static void AddListener(Name::ListenerFunction a_Listener, T* a_Context)\
	{\
		static const char* t_Name = #Name;\
		m_Listeners[t_Name].push_back(reinterpret_cast<Event::ListenerFunction>(a_Listener));\
		m_ListenerContexts[t_Name].push_back(reinterpret_cast<void*>(a_Context));\
	}\
	\
	virtual const char* GetName() const override\
	{\
		static const char* t_Name = #Name; \
		return t_Name;\
	}\
};

#define DEFINE_EVENT_1(Name, Type1, Variable1) class Name : public Event\
{\
public:\
	typedef void(*ListenerFunction)(const Name*, void*);\
	Name(Type1 a_Variable1) : \
		Variable1(a_Variable1)\
	{\
	}\
	\
	Type1 Variable1;\
	\
	static void AddListener(Name::ListenerFunction a_Listener)\
	{\
		static const char* t_Name = #Name; \
		m_Listeners[t_Name].push_back(reinterpret_cast<Event::ListenerFunction>(a_Listener)); \
		m_ListenerContexts[t_Name].push_back(nullptr); \
	}\
	\
	template<typename T>\
	static void AddListener(Name::ListenerFunction a_Listener, T* a_Context)\
	{\
		static const char* t_Name = #Name; \
		m_Listeners[t_Name].push_back(reinterpret_cast<Event::ListenerFunction>(a_Listener));\
		m_ListenerContexts[t_Name].push_back(reinterpret_cast<void*>(a_Context));\
	}\
	\
	virtual const char* GetName() const override\
	{\
		static const char* t_Name = #Name; \
		return t_Name;\
	}\
};

#define DEFINE_EVENT_2(Name, Type1, Variable1, Type2, Variable2) class Name : public Event\
{\
public:\
	typedef void(*ListenerFunction)(const Name*, void*);\
	Name(Type1 a_Variable1, Type2 a_Variable2) : \
		Variable1(a_Variable1),\
		Variable2(a_Variable2)\
	{\
	}\
	\
	Type1 Variable1;\
	Type2 Variable2;\
	\
	static void AddListener(Name::ListenerFunction a_Listener)\
	{\
		static const char* t_Name = #Name; \
		m_Listeners[t_Name].push_back(reinterpret_cast<Event::ListenerFunction>(a_Listener)); \
		m_ListenerContexts[t_Name].push_back(nullptr); \
	}\
	\
	template<typename T>\
	static void AddListener(Name::ListenerFunction a_Listener, T* a_Context)\
	{\
		static const char* t_Name = #Name; \
		m_Listeners[t_Name].push_back(reinterpret_cast<Event::ListenerFunction>(a_Listener));\
		m_ListenerContexts[t_Name].push_back(reinterpret_cast<void*>(a_Context));\
	}\
	\
	virtual const char* GetName() const override\
	{\
		static const char* t_Name = #Name; \
		return t_Name;\
	}\
};

#define DEFINE_EVENT_3(Name, Type1, Variable1, Type2, Variable2, Type3, Variable3) class Name : public Event\
{\
public:\
	typedef void(*ListenerFunction)(const Name*, void*);\
	Name(Type1 a_Variable1, Type2 a_Variable2, Type3 a_Variable3) : \
		Variable1(a_Variable1),\
		Variable2(a_Variable2),\
		Variable3(a_Variable3)\
	{\
	}\
	\
	Type1 Variable1;\
	Type2 Variable2;\
	Type3 Variable3;\
	\
	static void AddListener(Name::ListenerFunction a_Listener)\
	{\
		static const char* t_Name = #Name; \
		m_Listeners[t_Name].push_back(reinterpret_cast<Event::ListenerFunction>(a_Listener)); \
		m_ListenerContexts[t_Name].push_back(nullptr); \
	}\
	\
	template<typename T>\
	static void AddListener(Name::ListenerFunction a_Listener, T* a_Context)\
	{\
		static const char* t_Name = #Name; \
		m_Listeners[t_Name].push_back(reinterpret_cast<Event::ListenerFunction>(a_Listener));\
		m_ListenerContexts[t_Name].push_back(reinterpret_cast<void*>(a_Context));\
	}\
	\
	virtual const char* GetName() const override\
	{\
		static const char* t_Name = #Name; \
		return t_Name;\
	}\
};

#define DEFINE_EVENT_4(Name, Type1, Variable1, Type2, Variable2, Type3, Variable3, Type4, Variable4) class Name : public Event\
{\
public:\
	typedef void(*ListenerFunction)(const Name*, void*);\
	Name(Type1 a_Variable1, Type2 a_Variable2, Type3 a_Variable3, Type4 a_Variable4) : \
		Variable1(a_Variable1),\
		Variable2(a_Variable2),\
		Variable3(a_Variable3),\
		Variable4(a_Variable4)\
	{\
	}\
	\
	Type1 Variable1;\
	Type2 Variable2;\
	Type3 Variable3;\
	Type4 Variable4;\
	\
	static void AddListener(Name::ListenerFunction a_Listener)\
	{\
		static const char* t_Name = #Name; \
		m_Listeners[t_Name].push_back(reinterpret_cast<Event::ListenerFunction>(a_Listener)); \
		m_ListenerContexts[t_Name].push_back(nullptr); \
	}\
	\
	template<typename T>\
	static void AddListener(Name::ListenerFunction a_Listener, T* a_Context)\
	{\
		static const char* t_Name = #Name; \
		m_Listeners[t_Name].push_back(reinterpret_cast<Event::ListenerFunction>(a_Listener));\
		m_ListenerContexts[t_Name].push_back(reinterpret_cast<void*>(a_Context));\
	}\
	\
	virtual const char* GetName() const override\
	{\
		static const char* t_Name = #Name; \
		return t_Name;\
	}\
};

class Event
{
public:
	using ListenerFunction = EventListenerFunction;

	Event();

	virtual const char* GetName() const = 0;
	void Run();

protected:
	static std::map<const char*, std::vector<ListenerFunction>> m_Listeners;
	static std::map<const char*, std::vector<void*>> m_ListenerContexts;
	static std::mutex m_Mutex;
};