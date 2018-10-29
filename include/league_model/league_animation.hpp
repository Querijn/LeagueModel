#pragma once

#include <league_model/math.hpp>
#include <file_system.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <vector>

namespace League
{
	class Skeleton;

    class Animation
    {
    public:
		struct Bone
		{
			std::string name;
			std::vector<std::pair<float, glm::vec3>> translation;
			std::vector<std::pair<float, glm::quat>> quaternion;
			std::vector<std::pair<float, glm::vec3>> scale;
		};

		int BoneCount;
		int FrameCount;
		float FrameDelay;
		std::vector<Bone> Bones;

		using OnLoadFunction = std::function<void(League::Animation* a_Skeleton, BaseFile::LoadState a_LoadState)>;

		Animation();
		
		void Load(const std::string& a_File, const League::Skeleton& a_Skeleton, OnLoadFunction a_OnLoadFunction = nullptr);
    };
};