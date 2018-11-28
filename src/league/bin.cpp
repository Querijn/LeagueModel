#include "bin.hpp"

#include <file_system.hpp>

#include <algorithm>
#include <initializer_list>
#include <map>

// TODO: get rid?
#include <string>

uint32_t FNV1Hash(std::string a_String)
{
	size_t t_Hash = 0x811c9dc5;
	const char* t_Chars = a_String.c_str();
	for (int i = 0; i < a_String.length(); i++)
		t_Hash = ((t_Hash ^ tolower(t_Chars[i])) * 0x01000193) % 0x100000000;

	return t_Hash;
}

std::map<std::string, std::string> m_HashMap;
void InitBinHashMap()
{
	if (m_HashMap.size() != 0) return;

	auto* t_File = FileSystem::GetFile("data/cdtb/cdragontoolbox/hashes.bin.txt");
	t_File->Load([](File* a_File, File::LoadState a_LoadState, void* a_UserData)
	{
		if (a_LoadState != File::LoadState::Loaded)
		{
			FileSystem::CloseFile(*a_File);
			return;
		}

		static const size_t t_ReadLength = 1024;
		size_t t_Offset = 0;
		std::string t_String(t_ReadLength, '\0');
		
		bool t_WasEnd = false;
		do
		{
			t_WasEnd = a_File->Read((uint8_t*)t_String.c_str(), t_ReadLength, t_Offset) != t_ReadLength;

			size_t i;
			for (i = 0; i < t_ReadLength; i++)
			{
				// Check for end of buffer
				size_t t_NewLineOffset = t_String.find('\n', i);
				if (t_NewLineOffset == std::string::npos)
				{
					size_t t_Diff = t_ReadLength - i;
					std::string t_DebugLine = t_String.substr(i, t_Diff);
					t_Offset -= t_Diff;
					break;
				}

				// Make sure to handle \r cases
				bool t_HasCaretReturn = t_String[t_NewLineOffset - 1] == '\r';
				size_t t_LineLength = t_NewLineOffset - i - (t_HasCaretReturn ? 1 : 0);
				std::string t_Line = t_String.substr(i, t_LineLength);

				size_t t_HashEnd = t_Line.find(' '); // Should always be 8
				std::string t_Hash = t_Line.substr(0, t_HashEnd);
				std::string t_Value = t_Line.substr(t_HashEnd + 1);

				m_HashMap[t_Hash.c_str()] = t_Value.c_str();
				i = t_NewLineOffset;
			}
		} while (!t_WasEnd);
	});
}

std::string NumberToHexString(uint32_t a_Number)
{
	static const size_t t_Size = sizeof(uint32_t) << 1;
	static const char* t_Digits = "0123456789abcdef";
	std::string t_Result(t_Size, '0');
	for (size_t i = 0, j = (t_Size - 1) * 4; i < t_Size; ++i, j -= 4)
		t_Result[i] = t_Digits[(a_Number >> j) & 0x0f];

	return t_Result;
}

std::string GetStringByHash(uint32_t a_Hash)
{
	InitBinHashMap();

	auto t_Index = m_HashMap.find(NumberToHexString(a_Hash));
	if (t_Index != m_HashMap.end())
		return t_Index->second;

	return NumberToHexString(a_Hash);
}

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
		throw 0;
	}
}

std::vector<const League::Bin::ValueStorage*> League::Bin::Find(League::Bin::FindConditionFunction a_Function, void* a_UserData) const
{
	std::vector<const League::Bin::ValueStorage*> t_Results;
	for (const auto& t_VectorReference : m_Values)
	{
		const auto& t_ValueVector = t_VectorReference.second;
		for (int i = 0; i < t_ValueVector.size(); i++)
		{
			if (a_Function(t_ValueVector[i], a_UserData))
				t_Results.push_back(&t_ValueVector[i]);

			auto t_Results2 = t_ValueVector[i].Find(a_Function, a_UserData);
			if (t_Results2.size() != 0)
				t_Results.insert(t_Results.end(), t_Results2.begin(), t_Results2.end());
		}
	}

	return t_Results;
}

const League::Bin::ValueStorage * League::Bin::Get(std::string a_Name) const
{
	const auto t_Hash = FNV1Hash(a_Name.c_str());
	for (const auto& t_VectorReference : m_Values)
	{
		const auto& t_ValueVector = t_VectorReference.second;
		for (int i = 0; i < t_ValueVector.size(); i++)
		{
			auto t_ValueHash = t_ValueVector[i].GetHash();
			if (t_ValueHash == t_Hash)
				return &t_ValueVector[i];
		}
	}

	return nullptr;
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
		printf("int64 = %lld\n", Read<int64_t>());
		return;

	case ValueStorage::Type::U64:
		printf("uint64 = %llu\n", Read<uint64_t>());
		return;

	case ValueStorage::Type::Float:
		printf("uint64 = %f\n", Read<float>());
		return;

	case ValueStorage::Type::StringT:
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

std::string League::Bin::ValueStorage::GetAsJSON(bool a_ForceToString, bool a_ExposeHash) const
{
	std::string t_Result;

	switch (m_Type)
	{
	case ValueStorage::Type::Bool:
		t_Result = (Read<bool>() ? "true" : "false");
		break;

	case ValueStorage::Type::S8:
		t_Result = std::to_string(Read<int8_t>());
		break;

	case ValueStorage::Type::Padding:
	case ValueStorage::Type::U8:
		t_Result = std::to_string(Read<uint8_t>());
		break;

	case ValueStorage::Type::S16:
		t_Result = std::to_string(Read<int16_t>());
		break;

	case ValueStorage::Type::U16:
		t_Result = std::to_string(Read<uint16_t>());
		break;

	case ValueStorage::Type::S32:
		t_Result = std::to_string(Read<int32_t>());
		break;

	case ValueStorage::Type::U32:
	case ValueStorage::Type::Link: // TODO
		t_Result = std::to_string(Read<uint32_t>());
		break;

	case ValueStorage::Type::Hash:
		t_Result = "\"" + GetStringByHash(Read<uint32_t>()) + "\"";
		break;

	case ValueStorage::Type::S64:
		t_Result = std::to_string(Read<int64_t>());
		break;

	case ValueStorage::Type::U64:
		t_Result = std::to_string(Read<uint64_t>());
		break;

	case ValueStorage::Type::Float:
		t_Result = std::to_string(Read<float>());
		break;

	case ValueStorage::Type::U16Vec3:
		t_Result = GetJSONVector<uint16_t>(m_Data, 3);
		break;

	case ValueStorage::Type::FVec2:
		t_Result = GetJSONVector<float>(m_Data, 2);
		break;

	case ValueStorage::Type::FVec3:
		t_Result = GetJSONVector<float>(m_Data, 3);
		break;

	case ValueStorage::Type::FVec4:
		t_Result = GetJSONVector<float>(m_Data, 4);
		break;

	case ValueStorage::Type::RGBA:
		t_Result = GetJSONVector<uint8_t>(m_Data, 4, { 'r', 'g', 'b', 'a' });
		break;

	case ValueStorage::Type::StringT:
		t_Result =  "\"" + std::string((const char*)m_Data) + "\"";
		break;

	case ValueStorage::Type::Map:
	{
		std::string t_Return = "{";

		auto& t_Values = *(std::map<ValueStorage, ValueStorage, ValueStorageCompare>*)m_Pointer;

		bool t_First = true;
		for (auto& t_MapPair : t_Values)
		{
			if (t_First == false) t_Return += ",";

			t_Return += t_MapPair.first.GetAsJSON(true, false) + ": ";
			t_Return += t_MapPair.second.GetAsJSON(false, false);

			t_First = false;
		}

		t_Result = t_Return + "}";
		break;
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

			t_Return += t_ValueStorage.GetAsJSON(false, false);

			t_First = false;
		}

		t_Result = t_Return + "]";
		break;
	}

	case ValueStorage::Type::Struct:
	case ValueStorage::Type::Embedded:
	{
		std::string t_Return = "{";

		auto& t_Values = *(std::vector<ValueStorage>*)m_Pointer;

		bool t_First = true;
		for (const auto& t_ValueStorage : t_Values)
		{
			if (t_First == false) t_Return += ",";

			t_Return += t_ValueStorage.GetAsJSON(false, true);

			t_First = false;
		}

		t_Result = t_Return + "}";
		break;
	}

	default:
		throw 0;
	}

	if (a_ForceToString)
	{
		if (t_Result[0] != '"')
			t_Result = "\"" + t_Result;

		auto t_Last = t_Result.length() - 1;
		auto t_LastByte = t_Result[t_Last];
		if (t_LastByte != '"')
			t_Result += "\"";
	}
	else if (a_ExposeHash)
		return "\"" + GetStringByHash(m_Hash) + "\": " + t_Result;

	return t_Result;
}

std::vector<const League::Bin::ValueStorage*> League::Bin::ValueStorage::Find(FindConditionFunction a_Function, void * a_UserData) const
{
	std::vector<const ValueStorage*> t_Results;

	switch (m_Type)
	{
		case U16Vec3:
		case Bool:
		case S8:
		case U8:
		case S16:
		case U16:
		case S32:
		case U32:
		case S64:
		case U64:
		case Float:
		case FVec2:
		case FVec3:
		case FVec4:
		case Mat4:
		case RGBA:
		case Hash:
		case Link:
		case Padding:
			break;

		case StringT:
			break;

		case ValueStorage::Type::Struct:
		case ValueStorage::Type::Embedded:
		case ValueStorage::Type::Array:
		case ValueStorage::Type::Container:
		{
			auto& t_Values = *(std::vector<ValueStorage>*)m_Pointer;
			for (auto& t_Value : t_Values)
			{
				if (a_Function(t_Value, a_UserData))
					t_Results.push_back(&t_Value);

				auto t_Results2 = t_Value.Find(a_Function, a_UserData);
				if (t_Results2.size() != 0)
					t_Results.insert(t_Results.end(), t_Results2.begin(), t_Results2.end());
			}
			break;
		}

		// TODO: Somehow improve this?
		case Map:
		{
			auto& t_Values = *(std::map<ValueStorage, ValueStorage, ValueStorageCompare>*)m_Pointer;

			for (auto& t_Value : t_Values)
			{
				if (a_Function(t_Value.second, a_UserData))
					t_Results.push_back(&t_Value.second);

				auto t_Results2 = t_Value.second.Find(a_Function, a_UserData);
				if (t_Results2.size() != 0)
					t_Results.insert(t_Results.end(), t_Results2.begin(), t_Results2.end());
			}
		}
	}

	return t_Results;
}

bool League::Bin::ValueStorage::Is(const std::string& a_Name) const
{
	return FNV1Hash(a_Name) == m_Hash;
}

const League::Bin::ValueStorage* League::Bin::ValueStorage::Get(std::string a_Name) const
{
	auto t_Hash = FNV1Hash(a_Name.c_str());
	switch (m_Type)
	{
	case ValueStorage::Type::Map:
	{
		auto& t_Values = *(std::map<ValueStorage, ValueStorage, ValueStorageCompare>*)m_Pointer;

		for (auto& t_KeyValuePair : t_Values)
		{
			if (t_KeyValuePair.first.GetType() == ValueStorage::Type::Hash && t_Hash == t_KeyValuePair.first.Read<uint32_t>())
				return &t_KeyValuePair.second;

			if (t_Hash == t_KeyValuePair.second.GetHash())
				return &t_KeyValuePair.second;

			if (t_Hash == t_KeyValuePair.first.GetHash())
				return &t_KeyValuePair.second;
		}
	}

	case ValueStorage::Type::Struct:
	case ValueStorage::Type::Embedded:
	case ValueStorage::Type::Array:
	case ValueStorage::Type::Container:
	{
		auto& t_Values = *(std::vector<ValueStorage>*)m_Pointer;
		for (auto& t_Value : t_Values)
			if (t_Value.GetHash() != 0 && t_Value.GetHash() == t_Hash)
				return &t_Value;
	}

	default:
		break;
	}

	return nullptr;
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
		break;

	case ValueStorage::Type::StringT:
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
		throw 0;
	}
}

void League::Bin::Load(std::string a_FilePath, OnLoadFunction a_OnLoadFunction, void * a_Argument)
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

				std::string t_String(t_StringLength, '\0');
				a_File->Read((uint8_t*)&t_String[0], t_StringLength, t_Offset);

				t_Bin->m_LinkedFiles.push_back(t_String);
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

		t_Return += "\"" + GetStringByHash(t_MapPair.first) + "\": {"; // Hash

		bool t_First2 = true;
		for (int i = 0; i < t_MapPair.second.size(); i++)
		{
			if (t_First2 == false) t_Return += ",";
			t_Return += t_MapPair.second[i].GetAsJSON(false, true);

			t_First2 = false;
		}
		
		t_Return += "}";
		t_First = false;
	}

	return t_Return + "}";
}