#pragma once
#include <string.hpp>
#include <file_system.hpp>

#include <glm/glm.hpp>

namespace League
{
	class Bin
	{
	public:
		using OnLoadFunction = void(*)(League::Bin& a_Bin, void* a_Argument);

		void Load(StringView a_FilePath, OnLoadFunction a_OnLoadFunction = nullptr, void* a_Argument = nullptr);
		std::string GetAsJSON() const;

		File::LoadState GetLoadState() const { return m_State; }

		class ValueStorage
		{
		public:
			enum Type : uint8_t
			{
				U16Vec3 = 0,
				Bool = 1,
				S8 = 2,
				U8 = 3,
				S16 = 4,
				U16 = 5,
				S32 = 6,
				U32 = 7,
				S64 = 8,
				U64 = 9,
				Float = 10,
				FVec2 = 11,
				FVec3 = 12,
				FVec4 = 13,
				Mat4 = 14,
				RGBA = 15,
				String = 16,
				Hash = 17,
				Container = 18,
				Struct = 19,
				Embedded = 20,
				Link = 21,
				Array = 22,
				Map = 23,
				Padding = 24,
			};

			template<typename T> T Read() const { return *(T*)m_Data; }

			void DebugPrint();
			std::string GetAsJSON(bool a_ExposeHash) const;

			inline uint32_t GetHash() const { return m_Hash; }
			inline Type GetType() const { return m_Type; }
			inline const uint8_t* GetData() const { return m_Data; }
			inline const void* GetPointer() const { return m_Pointer; }

			friend class League::Bin;
		protected:
			void FetchDataFromFile(File* a_File, size_t& a_Offset);

			Type m_Type;
			uint8_t m_Data[256] = { 0 };
			void* m_Pointer = nullptr;
			uint32_t m_Hash = 0;
		};

		friend class League::Bin::ValueStorage;
	protected:
		static void GetStorageData(File* a_File, League::Bin::ValueStorage& t_Storage, size_t& t_Offset);

	private:
		static inline size_t GetSizeByType(League::Bin::ValueStorage::Type a_Type);

		File::LoadState m_State = File::LoadState::NotLoaded;

		std::map<uint32_t, std::vector<ValueStorage>> m_Values;
		std::vector<StringView> m_LinkedFiles;
	};
}