#pragma once

#include <file_system.hpp>
#include <string.hpp>

#include <glm/glm.hpp>

#include <vector>

namespace League
{
	class Skin;

	class Skeleton
	{
	public:
		using OnLoadFunction = void(*)(League::Skeleton& a_Skeleton, void* a_Argument);

		Skeleton(League::Skin& a_Skin);

		void Load(StringView a_FilePath, OnLoadFunction a_OnLoadFunction = nullptr, void* a_Argument = nullptr);
		
		struct Bone
		{
			StringView Name;
			uint32_t Hash;

			int16_t ID;
			int16_t ParentID;

			glm::mat4 LocalMatrix;
			glm::mat4 InverseGlobalMatrix;
			glm::mat4 GlobalMatrix;

			Bone* Parent;
			std::vector<Bone*> Children;
		};

		enum Type : uint32_t
		{
			Classic = 0x746C6B73,
			Version2 = 0x22FD4FC3
		};

		const std::vector<Bone>& GetBones() const;

		File::LoadState GetLoadState() const { return m_State; }

		const Skeleton::Bone* GetBone(StringView a_Name) const;

	private:
		File::LoadState m_State = File::LoadState::NotLoaded;

		League::Skin& m_Skin;
		Type m_Type;
		std::vector<Bone> m_Bones;

		File::LoadState ReadClassic(File& a_File, size_t& a_Offset);
		File::LoadState ReadVersion2(File& a_File, size_t& a_Offset);
	};
}