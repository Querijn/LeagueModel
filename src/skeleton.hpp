#pragma once

#include "types.hpp"

#include <vector>
#include <glm/mat4x4.hpp>
#include <spek/file/file.hpp>

namespace LeagueModel
{
	struct Skeleton
	{
		struct Bone
		{
			std::string name;
			uint32_t hash;

			int16_t id;
			int16_t parentID;

			glm::mat4 global;
			glm::mat4 local;
			glm::mat4 inverseGlobal;

			Bone* parent;
			std::vector<Bone*> children;
		};

		enum class Type : u32
		{
			Unknown = 0x0,
			Classic = 0x746C6B73,
			Version2 = 0x22FD4FC3
		};

		Spek::File::LoadState state = Spek::File::LoadState::NotLoaded;
		Type type;
		uint32_t version;
		std::vector<Bone> bones;
		std::vector<u32> boneIndices;

		using OnLoadFunction = std::function<void(LeagueModel::Skeleton& inSkeleton)>;
		void Load(const std::string& inFilePath, OnLoadFunction inOnLoadFunction = nullptr);

		const Skeleton::Bone* GetBone(u32 inNameHash) const;

	private:
		Spek::File::Handle file;
		Spek::File::LoadState ReadClassic(Spek::File::Handle inFile, size_t& inOffset);
		Spek::File::LoadState ReadVersion2(Spek::File::Handle inFile, size_t& inOffset);
	};
}