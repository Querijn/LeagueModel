#pragma once

#include <file_system.hpp>

#include <map>
#include <string>

namespace League
{
	class BaseValueStorage;

	class Bin
	{
	public:
		using ValueStorage = BaseValueStorage;
		using OnLoadFunction = void(*)(League::Bin& a_Bin, void* a_Argument);
		using FindConditionFunction = bool(*)(const ValueStorage& a_Value, void* a_UserData);

		void Load(std::string a_FilePath, OnLoadFunction a_OnLoadFunction = nullptr, void* a_Argument = nullptr);
		std::string GetAsJSON() const;

		File::LoadState GetLoadState() const { return m_State; }

		std::vector<const ValueStorage*> Find(FindConditionFunction a_Function, void* a_UserData = nullptr) const;
		const ValueStorage* Get(std::string a_Name) const;

		friend class BaseValueStorage;
	protected:
		void AddFlatValueStorage(ValueStorage* a_Storage);

	private:
		File::LoadState m_State = File::LoadState::NotLoaded;

		std::vector<ValueStorage*> m_FlatOverview;
		std::map<uint32_t, std::vector<ValueStorage*>> m_Values;
		std::vector<std::string> m_LinkedFiles;
	};
}