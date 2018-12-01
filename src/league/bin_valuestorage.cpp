#include "bin_valuestorage.hpp"

#include <glm/glm.hpp>

std::map<std::string, std::string> m_HashMap;

uint32_t FNV1Hash(std::string a_String)
{
	size_t t_Hash = 0x811c9dc5;
	const char* t_Chars = a_String.c_str();
	for (int i = 0; i < a_String.length(); i++)
		t_Hash = ((t_Hash ^ tolower(t_Chars[i])) * 0x01000193) % 0x100000000;

	return t_Hash;
}

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

bool League::BaseValueStorage::Is(const std::string & a_Name) const
{
	return FNV1Hash(a_Name) == m_Hash;
}

League::BaseValueStorage * League::BaseValueStorage::Create(Type a_Type, uint32_t a_Hash)
{
	switch (a_Type)
	{
	case Bool: return new NumberValueStorage<bool>(a_Type, a_Hash);
	case S8: return new NumberValueStorage<int8_t>(a_Type, a_Hash);
	case U8: return new NumberValueStorage<uint8_t>(a_Type, a_Hash);
	case S16: return new NumberValueStorage<int16_t>(a_Type, a_Hash);
	case U16: return new NumberValueStorage<uint16_t>(a_Type, a_Hash);
	case Link: return new NumberValueStorage<uint32_t>(a_Type, a_Hash);
	case S32: return new NumberValueStorage<int32_t>(a_Type, a_Hash);
	case U32: return new NumberValueStorage<uint32_t>(a_Type, a_Hash);
	case S64: return new NumberValueStorage<int64_t>(a_Type, a_Hash);
	case U64: return new NumberValueStorage<uint64_t>(a_Type, a_Hash);
	case Float: return new NumberValueStorage<float>(a_Type, a_Hash);
	case Hash: return new NumberValueStorage<uint32_t>(a_Type, a_Hash);
	case Padding: return new NumberValueStorage<uint8_t>(a_Type, a_Hash);

	case String: return new StringValueStorage(a_Type, a_Hash);

	case RGBA: return new NumberVectorValueStorage<glm::ivec4, glm::ivec4::value_type, uint8_t, 4>(a_Type, a_Hash);
	case FVec2: return new NumberVectorValueStorage<glm::vec2, glm::vec2::value_type, float, 2>(a_Type, a_Hash);
	case FVec3: return new NumberVectorValueStorage<glm::vec3, glm::vec3::value_type, float, 3>(a_Type, a_Hash);
	case FVec4: return new NumberVectorValueStorage<glm::vec4, glm::vec4::value_type, float, 4>(a_Type, a_Hash);
	case U16Vec3: return new NumberVectorValueStorage<glm::ivec3, glm::ivec3::value_type, uint16_t, 3>(a_Type, a_Hash);

	case Struct:
	case Embedded:
		return new StructValueStorage(a_Type, a_Hash);

	case Mat4: return new MatrixValueStorage(a_Type, a_Hash);
	case Container: return new ContainerValueStorage(a_Type, a_Hash);
	case Array: return new ArrayValueStorage(a_Type, a_Hash);
	case Map: return new MapValueStorage(a_Type, a_Hash);

	default: throw 0;
	};
}

League::BaseValueStorage* League::BaseValueStorage::Create(File * a_File, size_t & a_Offset)
{
	Type t_Type;
	uint32_t t_Hash;
	a_File->Get(t_Hash, a_Offset);
	a_File->Get(t_Type, a_Offset);

	return League::BaseValueStorage::Create(t_Type, t_Hash);
}

std::string League::BaseValueStorage::GetHashJSONPrefix() const
{
	return "\"" + GetStringByHash(m_Hash) + "\": ";
}

std::string League::StringValueStorage::DebugPrint() const
{
	return m_Data;
}

std::string League::StringValueStorage::GetAsJSON(bool a_ExposeHash) const
{
	if (a_ExposeHash)
		return GetHashJSONPrefix() + m_Data;
	else return m_Data;
}

void League::StringValueStorage::FetchDataFromFile(File * a_File, size_t & a_Offset)
{
	uint16_t t_StringLength;
	a_File->Get(t_StringLength, a_Offset);

	m_Data = std::string(t_StringLength, '\0');
	a_File->Read((uint8_t*)m_Data.c_str(), t_StringLength, a_Offset);
}

std::vector<const League::BaseValueStorage*> League::ArrayValueStorage::Find(FindConditionFunction a_Function, void * a_UserData) const
{
	std::vector<const League::BaseValueStorage*> t_Results;

	for (auto& t_Value : m_Data)
	{
		if (a_Function(*t_Value, a_UserData))
			t_Results.push_back(t_Value);

		auto t_Results2 = t_Value->Find(a_Function, a_UserData);
		if (t_Results2.size() != 0)
			t_Results.insert(t_Results.end(), t_Results2.begin(), t_Results2.end());
	}

	return t_Results;
}

League::BaseValueStorage * League::StructValueStorage::GetChild(const std::string & a_Child) const
{
	auto t_Hash = FNV1Hash(a_Child);
	for (auto& t_Element : m_Data)
		if (t_Element->GetHash() == t_Hash)
			return t_Element;

	return nullptr;
}

void League::StructValueStorage::FetchDataFromFile(File* a_File, size_t & a_Offset)
{
	uint32_t t_Hash;
	a_File->Get(t_Hash, a_Offset);

	uint32_t t_Size;
	a_File->Get(t_Size, a_Offset);

	uint16_t t_Count;
	a_File->Get(t_Count, a_Offset);

	m_Data.resize(t_Count);
	for (int i = 0; i < t_Count; i++)
	{
		m_Data[i] = League::BaseValueStorage::Create(a_File, a_Offset);
		m_Data[i]->FetchDataFromFile(a_File, a_Offset);
	}
}

League::BaseValueStorage * League::ContainerValueStorage::GetChild(const std::string & a_Child) const
{
	auto t_Hash = FNV1Hash(a_Child);
	for (auto& t_Element : m_Data)
		if (t_Element->GetHash() == t_Hash)
			return t_Element;

	return nullptr;
}

void League::ContainerValueStorage::FetchDataFromFile(File * a_File, size_t & a_Offset)
{
	BaseValueStorage::Type t_Type;
	a_File->Get(t_Type, a_Offset);

	uint32_t t_Size;
	a_File->Get(t_Size, a_Offset);

	uint32_t t_Count;
	a_File->Get(t_Count, a_Offset);

	m_Data.resize(t_Count);
	for (int i = 0; i < t_Count; i++)
	{
		m_Data[i] = BaseValueStorage::Create(t_Type);
		m_Data[i]->FetchDataFromFile(a_File, a_Offset);
	}
}

std::string League::ArrayValueStorage::DebugPrint() const
{
	std::string t_Result = "Array\n(\n";
	for (int i = 0; i < m_Data.size(); i++)
		t_Result += "\t" + m_Data[i]->DebugPrint() + ",\n";
	return t_Result + ")";
}

std::string League::ArrayValueStorage::GetAsJSON(bool a_ExposeHash) const
{
	std::string t_Result = "[ ";
	bool t_First = true;
	for (int i = 0; i < m_Data.size(); i++)
	{
		if (!t_First) t_Result += ",";
		t_Result += m_Data[i]->GetAsJSON(false);

		t_First = false;
	}

	return t_Result + "]";
}

League::BaseValueStorage * League::ArrayValueStorage::GetChild(size_t a_Index) const
{
	return m_Data[a_Index];
}

void League::ArrayValueStorage::FetchDataFromFile(File * a_File, size_t & a_Offset)
{
	BaseValueStorage::Type t_Type;
	a_File->Get(t_Type, a_Offset);

	uint8_t t_Count;
	a_File->Get(t_Count, a_Offset);

	m_Data.resize(t_Count);
	for (int i = 0; i < t_Count; i++)
	{
		m_Data[i] = League::BaseValueStorage::Create(t_Type);
		m_Data[i]->FetchDataFromFile(a_File, a_Offset);
	}
}

std::string League::MatrixValueStorage::DebugPrint() const
{
	std::string t_Result = "Matrix\n(";
	bool t_First = true;
	for (int x = 0; x < 4; x++)
	{
		t_Result += "\n";

		bool t_First = true;
		for (int y = 0; y < 4; y++)
		{
			t_Result += t_First ? "\t" : ",";

			t_Result += std::to_string(m_Data[x][y]);

			t_First = false;
		}

		t_Result += "\n";
	}

	return t_Result + ")";
}

std::string League::MatrixValueStorage::GetAsJSON(bool a_ExposeHash) const
{
	return std::string();
}

void League::MatrixValueStorage::FetchDataFromFile(File * a_File, size_t & a_Offset)
{
	for (int x = 0; x < 4; x++)
		for (int y = 0; y < 4; y++)
			a_File->Get(m_Data[x][y], a_Offset);
}

std::vector<const League::BaseValueStorage*> League::MapValueStorage::Find(FindConditionFunction a_Function, void * a_UserData) const
{
	std::vector<const League::BaseValueStorage*> t_Results;

	for (auto& t_Value : m_Data)
	{
		if (a_Function(*t_Value.second, a_UserData))
			t_Results.push_back(t_Value.second);

		auto t_Results2 = t_Value.second->Find(a_Function, a_UserData);
		if (t_Results2.size() != 0)
			t_Results.insert(t_Results.end(), t_Results2.begin(), t_Results2.end());
	}

	return t_Results;
}

std::string League::MapValueStorage::DebugPrint() const
{
	return std::string();
}

std::string League::MapValueStorage::GetAsJSON(bool a_ExposeHash) const
{
	return std::string();
}

League::BaseValueStorage * League::MapValueStorage::GetChild(const std::string & a_Child) const
{
	auto t_Hash = FNV1Hash(a_Child);
	for (auto& t_Element : m_Data)
	{
		if (t_Element.first->GetType() == League::BaseValueStorage::Type::Hash)
		{
			auto t_HashContainer = (const League::NumberValueStorage<uint32_t>*)t_Element.first;

			if (t_HashContainer->Get() == t_Hash)
				return t_Element.second;
		}

		if (t_Element.second->GetHash() != 0 && t_Element.second->GetHash() == t_Hash)
			return t_Element.second;
	}
		

	return nullptr;
}

void League::MapValueStorage::FetchDataFromFile(File * a_File, size_t & a_Offset)
{
	Type t_KeyType;
	a_File->Get(t_KeyType, a_Offset);

	Type t_ValueType;
	a_File->Get(t_ValueType, a_Offset);

	uint32_t t_Size;
	a_File->Get(t_Size, a_Offset);

	uint32_t t_Count;
	a_File->Get(t_Count, a_Offset);

	for (int i = 0; i < t_Count; i++)
	{
		auto t_Key = BaseValueStorage::Create(t_KeyType);
		auto t_Value = BaseValueStorage::Create(t_ValueType);

		t_Key->FetchDataFromFile(a_File, a_Offset);
		t_Value->FetchDataFromFile(a_File, a_Offset);
		m_Data[t_Key] = t_Value;
	}
}
