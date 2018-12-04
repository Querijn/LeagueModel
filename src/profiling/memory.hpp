#pragma once

#if defined(NDEBUG) && !defined(DEBUG_MEMORY)

#define New(t) new t
#define Delete(t) delete t

class Memory
{
public:
	static void Expose();
};

#else

class Memory
{
public:
	template<typename T>
	static T* TrackNew(T* a_Pointer, const char* a_File, size_t a_Line)
	{
		NewPointerInfo(a_File, a_Line, typeid(T).name(), a_Pointer);
		return a_Pointer;
	}

	template<typename T>
	static void TrackDelete(T* a_Pointer, const char* a_File, size_t a_Line)
	{
		DeletePointerInfo(a_File, a_Line, typeid(T).name(), a_Pointer);
		delete a_Pointer;
	}

	static void Expose();

private:
	static void NewPointerInfo(const char* a_File, size_t a_Line, const char* a_Name, void* a_Pointer);
	static void DeletePointerInfo(const char* a_File, size_t a_Line, const char* a_Name, void* a_Pointer);
};

#define New(t) Memory::TrackNew(new t, __FILE__, __LINE__)
#define Delete(t) Memory::TrackDelete(t, __FILE__, __LINE__)

#endif