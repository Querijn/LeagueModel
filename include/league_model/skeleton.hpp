#pragma once

#include <league_model/file/file_system.hpp>
#include <string>

#include <glm/glm.hpp>

#include <vector>

namespace League
{
	class Skin;

	class Skeleton
	{
	public:
		using OnLoadFunction = void(*)(League::Skeleton& a_Skeleton, void* a_Argument);
		void Load(const std::string& a_FilePath, OnLoadFunction a_OnLoadFunction = nullptr, void* a_Argument = nullptr);
		
		Skeleton() {}
		Skeleton(const Skeleton& a_Skeleton);

		struct Bone
		{
			std::string Name;
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

		const Skeleton::Bone* GetBone(uint32_t a_NameHash) const;

		void ApplyToSkin(League::Skin& a_Skin) const;

	private:
		File::LoadState m_State = File::LoadState::NotLoaded;

		Type m_Type;
		uint32_t m_Version;
		std::vector<Bone> m_Bones;
		std::vector<uint32_t> m_BoneIndices;

		File::LoadState ReadClassic(File& a_File, size_t& a_Offset);
		File::LoadState ReadVersion2(File& a_File, size_t& a_Offset);
	};
}