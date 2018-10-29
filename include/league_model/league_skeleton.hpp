#pragma once

#include <league_model/math.hpp>
#include <file_system.hpp>

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <map>

namespace League
{
	class Skin;

    class Skeleton
    {
    public:
		using OnLoadFunction = std::function<void(League::Skeleton* a_Skeleton, BaseFile::LoadState a_LoadState)>;

        struct Bone
		{
			char name[32];
			unsigned int hash;
			int16_t id;
			int16_t parent;
			glm::vec3 scale;
			glm::mat4 localMatrix;
			glm::mat4 globalMatrix;

			std::vector<Bone*> Children;
		};

		Skeleton(League::Skin& a_Skin);
		
		void Load(const std::string& a_File, OnLoadFunction a_OnLoadFunction = nullptr);

        std::vector<Bone> Bones;
		std::vector<uint16_t> BoneIndices;
		std::map<uint32_t, uint32_t> BoneIndexMap;

	private:
		League::Skin& m_Skin;

		BaseFile::LoadState ReadClassic(BaseFile& a_File);
		BaseFile::LoadState ReadVersion2(BaseFile& a_File);
    };
};
