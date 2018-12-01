#pragma once
#if defined(__EMSCRIPTEN__)

#include <file/base_file.hpp>

#include <vector>

class EmscriptenFile : public BaseFile
{
public:
	using LoadState = FileLoadState;
	using OnLoadFunction = void(*)(EmscriptenFile* a_File, FileLoadState a_LoadState, void* a_Argument);

	void Load(OnLoadFunction a_OnLoadCallback, void* a_Argument = nullptr);
	size_t Read(uint8_t* a_Destination, size_t a_ByteCount, size_t& a_Offset) const;

	template<typename T>
	bool Get(T& a_Element, size_t& a_Offset)
	{
		bool t_Read = Read((uint8_t*)&a_Element, sizeof(T), a_Offset) == sizeof(T);
		return t_Read;
	}

	template<typename T>
	bool Get(std::vector<T>& a_Element, size_t& a_Offset)
	{
		return false;
	}

	template<typename T>
	bool Get(std::vector<T>& a_Element, size_t a_Count, size_t& a_Offset)
	{
		a_Element.clear();

		bool t_Worked = true;
		for (int i = 0; i < a_Count; i++)
		{
			T t_Element;
			if (Get(t_Element, a_Offset) != sizeof(T))
				t_Worked = false;

			auto t_Name = std::to_string(t_Element);
			a_Element.push_back(t_Element);
		}

		return t_Worked;
	}

	const std::vector<uint8_t>& GetData() const;
	std::string GetName() const;

	friend class FileSystem;
protected:
	EmscriptenFile(std::string a_FileName) : BaseFile(a_FileName) { }

private:
	static void OnLoad(void* a_Argument, void* a_Data, int a_Size);
	static void OnLoadFailed(void* a_Argument);

	std::vector<uint8_t> m_Data;
	std::shared_ptr<std::ifstream> m_Stream;
	
	OnLoadFunction m_OnLoadArg = nullptr;
	void* m_ArgData = nullptr;
};

#endif