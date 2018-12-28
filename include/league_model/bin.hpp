#pragma once

#include <league_model/file/file_system.hpp>

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

		~Bin();

		void Load(const std::string& a_FilePath, OnLoadFunction a_OnLoadFunction = nullptr, void* a_Argument = nullptr);
		std::string GetAsJSON() const;

		File::LoadState GetLoadState() const { return m_State; }
		const std::vector<std::string>& GetLinkedFiles() const { return m_LinkedFiles; }

		std::vector<const ValueStorage*> Find(FindConditionFunction a_Function, void* a_UserData = nullptr) const;
		const ValueStorage* Get(const std::string& a_Name) const;
		const std::vector<ValueStorage*>* GetTopLevel(uint32_t a_Hash) const;

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