#include "bin.hpp"
#include "bin_valuestorage.hpp"

#include <profiling.hpp>

#include <cassert>

uint32_t FNV1Hash(const std::string& a_String);
std::string GetStringByHash(uint32_t a_Hash);

League::Bin::~Bin()
{
	for (auto t_Element : m_FlatOverview)
		LM_DEL(t_Element);
}

void League::Bin::Load(const std::string& a_FilePath, OnLoadFunction a_OnLoadFunction, void * a_Argument)
{
	Profiler::Context t(__FUNCTION__);

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
	auto* t_LoadData = LM_NEW(LoadData(this, a_OnLoadFunction, a_Argument));

	t_File->Load([](File* a_File, File::LoadState a_LoadState, void* a_Argument)
	{
		Profiler::Context t("League::Bin::Load->OnFileReceived");

		auto* t_LoadData = (LoadData*)a_Argument;
		auto* t_Bin = (Bin*)t_LoadData->Target;

		if (a_LoadState != File::LoadState::Loaded)
		{
			t_Bin->m_State = a_LoadState;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Bin, t_LoadData->Argument);

			LM_DEL(t_LoadData);
			return;
		}

		size_t t_Offset = 0;
		char t_PROP[4];
		a_File->Read((uint8_t*)t_PROP, 4, t_Offset);
		if (memcmp(t_PROP, "PROP", 4) != 0)
		{
			t_Bin->m_State = File::LoadState::FailedToLoad;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Bin, t_LoadData->Argument);

			LM_DEL(t_LoadData);
			return;
		}

		uint32_t t_Version;
		a_File->Get(t_Version, t_Offset);
		if (t_Version > 2)
		{
			t_Bin->m_State = File::LoadState::FailedToLoad;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Bin, t_LoadData->Argument);

			LM_DEL(t_LoadData);
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

#if !defined(__EMSCRIPTEN__)
				std::string t_String(t_StringLength, '\0');
				a_File->Read((uint8_t*)&t_String[0], t_StringLength, t_Offset);

				t_Bin->m_LinkedFiles.push_back(t_String);
#else
				t_Offset += t_StringLength;
#endif
			}
		}

		uint32_t t_EntryCount;
		a_File->Get(t_EntryCount, t_Offset);

		std::vector<uint32_t> t_Types;
		if (t_EntryCount != 0)
			a_File->Get(t_Types, t_EntryCount, t_Offset);
	
		for (uint32_t i = 0; i < t_EntryCount; i++)
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
				BaseValueStorage* t_Storage = League::BaseValueStorage::Create(*t_Bin, a_File, t_Offset, nullptr);
				t_Storage->FetchDataFromFile(a_File, t_Offset);
				t_Vector.push_back(t_Storage);
			}

			assert(t_Offset - t_Begin == t_Length);
		}

		t_Bin->m_State = File::LoadState::Loaded;
		if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Bin, t_LoadData->Argument);

		LM_DEL(t_LoadData);
	}, t_LoadData);
}

std::string League::Bin::GetAsJSON() const
{
	std::string t_Return = "{";

	bool t_First = true;
	for (auto& t_MapPair : m_Values)
	{
		if (t_First == false) t_Return += ",";

		t_Return += "\"" + GetStringByHash(t_MapPair.first) + "\": ["; // Hash

		bool t_First2 = true;
		for (int i = 0; i < t_MapPair.second.size(); i++)
		{
			if (t_First2 == false) t_Return += ",";
			t_Return += t_MapPair.second[i]->GetAsJSON(false, false);

			t_First2 = false;
		}
		
		t_Return += " ]";
		t_First = false;
	}

	return t_Return + "}";
}

std::vector<const League::BaseValueStorage*> League::Bin::Find(FindConditionFunction a_Function, void * a_UserData) const
{
	std::vector<const League::Bin::ValueStorage*> t_Results;
	for (const auto& t_Storage : m_FlatOverview)
		if (a_Function(*t_Storage, a_UserData))
			t_Results.push_back(t_Storage);

	return t_Results;
}

const League::Bin::ValueStorage * League::Bin::Get(const std::string& a_Name) const
{
	const auto t_Hash = FNV1Hash(a_Name.c_str());
	for (const auto& t_VectorReference : m_Values)
	{
		const auto& t_ValueVector = t_VectorReference.second;
		for (int i = 0; i < t_ValueVector.size(); i++)
		{
			auto t_ValueHash = t_ValueVector[i]->GetHash();
			if (t_ValueHash == t_Hash)
				return t_ValueVector[i];
		}
	}

	return nullptr;
}

void League::Bin::AddFlatValueStorage(ValueStorage * a_Storage)
{
	m_FlatOverview.push_back(a_Storage);
}