#include "bin.hpp"

#include <file_system.hpp>

#include <algorithm>
#include <initializer_list>
#include <map>

struct ValueStorageCompare
{
	bool operator() (const League::Bin::ValueStorage& a_LeftHandSide, const League::Bin::ValueStorage& a_RightHandSide) const
	{
		if (a_LeftHandSide.GetType() != a_RightHandSide.GetType())
			return a_LeftHandSide.GetType() < a_RightHandSide.GetType();

		if (a_LeftHandSide.GetPointer() != a_RightHandSide.GetPointer())
			return a_LeftHandSide.GetPointer() < a_RightHandSide.GetPointer();

		auto* t_Data1 = a_LeftHandSide.GetData();
		auto* t_Data2 = a_RightHandSide.GetData();
		for (int i = 0; i < 256; i++)
		{
			if (t_Data1[i] == t_Data2[i]) continue;
			return t_Data1[i] < t_Data2[i];
		}

		return false;
	}
};

inline size_t League::Bin::GetSizeByType(ValueStorage::Type a_Type)
{
	switch (a_Type)
	{
	case ValueStorage::Type::Bool:
		return sizeof(bool);

	case ValueStorage::Type::S8:
	case ValueStorage::Type::U8:
		return sizeof(uint8_t);

	case ValueStorage::Type::S16:
	case ValueStorage::Type::U16:
		return sizeof(uint16_t);

	case ValueStorage::Type::S32:
	case ValueStorage::Type::U32:
		return sizeof(uint32_t);

	case ValueStorage::Type::S64:
	case ValueStorage::Type::U64:
		return sizeof(uint64_t);

	case ValueStorage::Type::Float:
		return sizeof(float);

	case ValueStorage::Type::Hash:
		return sizeof(uint32_t);


	case ValueStorage::Type::Padding:
		return sizeof(uint8_t);

	case ValueStorage::Type::U16Vec3:
		return sizeof(uint16_t) * 3;

	case ValueStorage::Type::FVec2:
		return sizeof(float) * 2;

	case ValueStorage::Type::FVec3:
		return sizeof(float) * 3;

	case ValueStorage::Type::FVec4:
		return sizeof(float) * 4;

	case ValueStorage::Type::RGBA:
		return sizeof(uint8_t) * 4;

	default:
		__debugbreak();
	}
}

void League::Bin::GetStorageData(File* a_File, League::Bin::ValueStorage& a_Storage, size_t& a_Offset)
{
	a_File->Get(a_Storage.m_Hash, a_Offset);
	a_File->Get(a_Storage.m_Type, a_Offset);
	a_Storage.FetchDataFromFile(a_File, a_Offset);
}

void League::Bin::ValueStorage::DebugPrint()
{
	switch (m_Type)
	{
	case ValueStorage::Type::Bool:
		printf("Bool = %s\n", Read<bool>() ? "true" : "false");
		return;

	case ValueStorage::Type::S8:
		printf("int8 = %d\n", Read<int8_t>());
		return;

	case ValueStorage::Type::U8:
		printf("int8 = %d\n", Read<uint8_t>());
		return;

	case ValueStorage::Type::S16:
		printf("int16 = %d\n", Read<int16_t>());
		return;

	case ValueStorage::Type::U16:
		printf("uint16 = %d\n", Read<uint16_t>());
		return;

	case ValueStorage::Type::S32:
		printf("int32 = %d\n", Read<int32_t>());
		return;

	case ValueStorage::Type::U32:
		printf("int32 = %d\n", Read<int32_t>());
		return;

	case ValueStorage::Type::Hash:
		printf("int32 = %d\n", Read<int32_t>());
		return;

	case ValueStorage::Type::S64:
		printf("int64 = %d\n", Read<int64_t>());
		return;

	case ValueStorage::Type::U64:
		printf("uint64 = %d\n", Read<uint64_t>());
		return;

	case ValueStorage::Type::Float:
		printf("uint64 = %f\n", Read<float>());
		return;

	case ValueStorage::Type::String:
		printf("string = %s\n", (char*)m_Data);
		break;

	case ValueStorage::Type::U16Vec3:
	{
		uint16_t* t_Vector = (uint16_t*)m_Data;
		for (int i = 0; i < 3; i++)
			printf("U16Vec3 = { %d, %d, %d }\n", t_Vector[0], t_Vector[1], t_Vector[2]);
		break;
	}

	default:
		printf("I don't know how to print this type (%d)!\n", m_Type);
		return;
	}
}

template<typename T>
std::string GetJSONVector(const void* a_Data, size_t a_Size, const std::initializer_list<char>& a_Names = { 'x', 'y', 'z', 'w' })
{
	std::string t_Return = "{ ";
	T* t_Vector = (T*)a_Data;

	for (int i = 0; i < a_Size; i++)
	{
		char t_Character = *(a_Names.begin() + i);
		t_Return += '\"';
		t_Return += t_Character;
		t_Return += "\": ";
		t_Return += std::to_string(t_Vector[i]);

		if (i != a_Size - 1) t_Return += ", ";
	}
	t_Return += '}';

	return t_Return;
}

std::string League::Bin::ValueStorage::GetAsJSON() const
{
	switch (m_Type)
	{
	case ValueStorage::Type::Bool:
		return Read<bool>() ? "true" : "false";

	case ValueStorage::Type::S8:
		return std::to_string(Read<int8_t>());

	case ValueStorage::Type::Padding:
	case ValueStorage::Type::U8:
		return std::to_string(Read<uint8_t>());

	case ValueStorage::Type::S16:
		return std::to_string(Read<int16_t>());

	case ValueStorage::Type::U16:
		return std::to_string(Read<uint16_t>());

	case ValueStorage::Type::S32:
		return std::to_string(Read<int32_t>());

	case ValueStorage::Type::U32:
	case ValueStorage::Type::Link: // TODO
	case ValueStorage::Type::Hash: // TODO
		return std::to_string(Read<uint32_t>());

	case ValueStorage::Type::S64:
		return std::to_string(Read<int64_t>());

	case ValueStorage::Type::U64:
		return std::to_string(Read<uint64_t>());

	case ValueStorage::Type::Float:
		return std::to_string(Read<float>());

	case ValueStorage::Type::U16Vec3:
		return GetJSONVector<uint16_t>(m_Data, 3);

	case ValueStorage::Type::FVec2:
		return GetJSONVector<float>(m_Data, 2);

	case ValueStorage::Type::FVec3:
		return GetJSONVector<float>(m_Data, 3);

	case ValueStorage::Type::FVec4:
		return GetJSONVector<float>(m_Data, 4);

	case ValueStorage::Type::RGBA:
		return GetJSONVector<uint8_t>(m_Data, 4, { 'r', 'g', 'b', 'a' });

	case ValueStorage::Type::String:
		return  "\"" + std::string((const char*)m_Data) + "\"";

	case ValueStorage::Type::Map:
	{
		std::string t_Return = "{";

		auto& t_Values = *(std::map<ValueStorage, ValueStorage, ValueStorageCompare>*)m_Pointer;

		bool t_First = true;
		for (auto& t_MapPair : t_Values)
		{
			if (t_First == false) t_Return += ",";

			t_Return += "\"" + t_MapPair.first.GetAsJSON() + "\": ";
			t_Return += t_MapPair.second.GetAsJSON();

			t_First = false;
		}

		return t_Return + "}";
	}

	case ValueStorage::Type::Array:
	case ValueStorage::Type::Container:
	{
		std::string t_Return = "[";

		auto& t_Values = *(std::vector<ValueStorage>*)m_Pointer;

		bool t_First = true;
		for (auto& t_ValueStorage : t_Values)
		{
			if (t_First == false) t_Return += ",";

			t_Return += t_ValueStorage.GetAsJSON();

			t_First = false;
		}

		return t_Return + "]";
	}

	case ValueStorage::Type::Struct:
	case ValueStorage::Type::Embedded:
	{
		std::string t_Return = "{";

		auto& t_Values = *(std::vector<ValueStorage>*)m_Pointer;

		bool t_First = true;
		for (auto& t_ValueStorage : t_Values)
		{
			if (t_First == false) t_Return += ",";

			t_Return += "\"" + std::to_string(t_ValueStorage.GetHash()) + "\": ";
			t_Return += t_ValueStorage.GetAsJSON();

			t_First = false;
		}

		return t_Return + "}";
	}

	default:
		__debugbreak();
	}
}

void League::Bin::ValueStorage::FetchDataFromFile(File* a_File, size_t& a_Offset)
{
	switch (m_Type)
	{
	case ValueStorage::Type::Bool:
	case ValueStorage::Type::S8:
	case ValueStorage::Type::U8:
	case ValueStorage::Type::S16:
	case ValueStorage::Type::U16:
	case ValueStorage::Type::S32:
	case ValueStorage::Type::U32:
	case ValueStorage::Type::S64:
	case ValueStorage::Type::U64:
	case ValueStorage::Type::Float:
	case ValueStorage::Type::Padding:
	case ValueStorage::Type::U16Vec3:
	case ValueStorage::Type::FVec2:
	case ValueStorage::Type::FVec3:
	case ValueStorage::Type::FVec4:
	case ValueStorage::Type::RGBA:
	case ValueStorage::Type::Hash:
		a_File->Read(m_Data, GetSizeByType(m_Type), a_Offset);
		DebugPrint();
		break;

	case ValueStorage::Type::String:
	{
		uint16_t t_StringLength;
		a_File->Get(t_StringLength, a_Offset);

		assert(t_StringLength < sizeof(m_Data));
		a_File->Read(m_Data, t_StringLength, a_Offset);
		break;
	}

	case ValueStorage::Type::Struct:
	case ValueStorage::Type::Embedded:
	{
		uint32_t t_Hash;
		a_File->Get(t_Hash, a_Offset);

		uint32_t t_Size;
		a_File->Get(t_Size, a_Offset);

		uint16_t t_Count;
		a_File->Get(t_Count, a_Offset);

		auto t_Array = new std::vector<ValueStorage>(t_Count);
		m_Pointer = t_Array;
		for (int i = 0; i < t_Count; i++) 
			League::Bin::GetStorageData(a_File, t_Array->at(i), a_Offset);
		break;
	}

	case ValueStorage::Type::Container:
	{
		ValueStorage::Type t_Type;
		a_File->Get(t_Type, a_Offset);

		uint32_t t_Size;
		a_File->Get(t_Size, a_Offset);

		uint32_t t_Count;
		a_File->Get(t_Count, a_Offset);

		auto t_Array = new std::vector<ValueStorage>(t_Count);
		m_Pointer = t_Array;
		for (int i = 0; i < t_Count; i++)
		{
			auto& t_Element = t_Array->at(i);
			t_Element.m_Type = t_Type;
			t_Element.FetchDataFromFile(a_File, a_Offset);
		}
		break;
	}

	case ValueStorage::Type::Array:
	{
		ValueStorage::Type t_Type;
		a_File->Get(t_Type, a_Offset);

		uint8_t t_Count;
		a_File->Get(t_Count, a_Offset);

		auto t_Array = new std::vector<ValueStorage>(t_Count);
		m_Pointer = t_Array;
		for (int i = 0; i < t_Count; i++)
		{
			auto& t_Element = t_Array->at(i);
			t_Element.m_Type = t_Type;
			t_Element.FetchDataFromFile(a_File, a_Offset);
		}
		break;
	}

	case ValueStorage::Type::Map:
	{
		ValueStorage t_Key;
		a_File->Get(t_Key.m_Type, a_Offset);

		ValueStorage t_Value;
		a_File->Get(t_Value.m_Type, a_Offset);

		uint32_t t_Size;
		a_File->Get(t_Size, a_Offset);

		uint32_t t_Count;
		a_File->Get(t_Count, a_Offset);

		auto t_Map = new std::map<ValueStorage, ValueStorage, ValueStorageCompare>();
		m_Pointer = t_Map;
		for (int i = 0; i < t_Count; i++)
		{
			t_Key.FetchDataFromFile(a_File, a_Offset);
			t_Value.FetchDataFromFile(a_File, a_Offset);
			(*t_Map)[t_Key] = t_Value;
		}
		break;
	}

	case ValueStorage::Type::Link:
		a_File->Read(m_Data, sizeof(uint32_t), a_Offset);
		break;

	default:
		__debugbreak();
	}
}

void League::Bin::Load(StringView a_FilePath, OnLoadFunction a_OnLoadFunction, void * a_Argument)
{
	auto* t_File = FileSystem::GetFile(a_FilePath);

	struct LoadData
	{
		LoadData(Bin* a_Target, OnLoadFunction a_Function, void* a_Argument) :
			Target(a_Target), OnLoadFunction(a_Function), Argument(a_Argument)
		{}

		Bin* Target;
		OnLoadFunction OnLoadFunction;
		void* Argument;
	};
	auto* t_LoadData = new LoadData(this, a_OnLoadFunction, a_Argument);

	t_File->Load([](File* a_File, File::LoadState a_LoadState, void* a_Argument)
	{
		auto* t_LoadData = (LoadData*)a_Argument;
		auto* t_Bin = (Bin*)t_LoadData->Target;

		if (a_LoadState != File::LoadState::Loaded)
		{
			t_Bin->m_State = a_LoadState;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Bin, t_LoadData->Argument);

			FileSystem::CloseFile(*a_File);
			delete t_LoadData;
			return;
		}

		size_t t_Offset = 0;
		char t_PROP[4];
		a_File->Read((uint8_t*)t_PROP, 4, t_Offset);
		if (memcmp(t_PROP, "PROP", 4) != 0)
		{
			t_Bin->m_State = File::LoadState::FailedToLoad;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Bin, t_LoadData->Argument);

			FileSystem::CloseFile(*a_File);
			delete t_LoadData;
			return;
		}

		uint32_t t_Version;
		a_File->Get(t_Version, t_Offset);
		if (t_Version > 2)
		{
			t_Bin->m_State = File::LoadState::FailedToLoad;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Bin, t_LoadData->Argument);

			FileSystem::CloseFile(*a_File);
			delete t_LoadData;
			return;
		}

		if (t_Version == 2)
		{
			uint32_t t_LinkedFilesCount;
			a_File->Get(t_LinkedFilesCount, t_Offset);

			for (int i = 0; i < t_LinkedFilesCount; i++) 
			{
				uint16_t t_StringLength;
				a_File->Get(t_StringLength, t_Offset);

				char t_StringVector[256] = { 0 };
				a_File->Read((uint8_t*)t_StringVector, t_StringLength, t_Offset);

				t_Bin->m_LinkedFiles.push_back(t_StringVector);
			}
		}

		uint32_t t_EntryCount;
		a_File->Get(t_EntryCount, t_Offset);

		std::vector<uint32_t> t_Types;
		if (t_EntryCount != 0)
			a_File->Get(t_Types, t_EntryCount, t_Offset);
	
		for (int i = 0; i < t_EntryCount; i++)
		{
			uint32_t t_Length;
			a_File->Get(t_Length, t_Offset);
			uint32_t t_Begin = t_Offset;

			uint32_t t_Hash;
			a_File->Get(t_Hash, t_Offset);

			uint16_t t_ValueCount;
			a_File->Get(t_ValueCount, t_Offset);

			auto& t_Vector = t_Bin->m_Values[t_Hash];
			for (int j = 0; j < t_ValueCount; j++)
			{
				ValueStorage t_Storage;
				GetStorageData(a_File, t_Storage, t_Offset);
				t_Vector.push_back(t_Storage);
			}

			assert(t_Offset - t_Begin == t_Length);
		}

		t_Bin->m_State = File::LoadState::Loaded;
		if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Bin, t_LoadData->Argument);

		FileSystem::CloseFile(*a_File);
		delete t_LoadData;
	}, t_LoadData);
}

std::string League::Bin::GetAsJSON() const
{
	std::string t_Return = "{";

	bool t_First = true;
	for (auto& t_MapPair : m_Values)
	{
		if (t_First == false) t_Return += ",";

		t_Return += "\"" + std::to_string(t_MapPair.first) + "\": ["; // Hash

		bool t_First2 = true;
		for (int i = 0; i < t_MapPair.second.size(); i++)
		{
			if (t_First2 == false) t_Return += ",";
			t_Return += t_MapPair.second[i].GetAsJSON();

			t_First2 = false;
		}
		
		t_Return += "]";
		t_First = false;
	}

	return t_Return + "}";
}