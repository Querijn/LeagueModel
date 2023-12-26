#include "skeleton.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

namespace LeagueModel
{
	uint32_t StringToHash(const std::string& s)
	{
		unsigned int hash = 0;
		unsigned int temp = 0;

		for (auto& c : s)
		{
			hash = (hash << 4) + tolower(c);
			temp = hash & 0xf0000000;

			if (temp != 0)
			{
				hash = hash ^ (temp >> 24);
				hash = hash ^ temp;
			}
		}

		return hash;
	}

	static void RecursiveInvertGlobalMatrices(const glm::mat4& parent, Skeleton::Bone& bone)
	{
		auto global = parent * bone.local;
		bone.global = global;
		bone.inverseGlobal = glm::inverse(global);

		for (auto child : bone.children)
			RecursiveInvertGlobalMatrices(global, *child);
	}

	void Skeleton::Load(const std::string& inFilePath, OnLoadFunction onLoadFunction)
	{
		file = Spek::File::Load(inFilePath.c_str(), [onLoadFunction, this](Spek::File::Handle file)
		{
			state = file ? file->GetLoadState() : Spek::File::LoadState::FailedToLoad;
			if (state != Spek::File::LoadState::Loaded)
			{
				printf("Skeleton %s.\n", state == Spek::File::LoadState::FailedToLoad ? "failed to load" : "was not found");
				return;
			}

			size_t offset = 0;
			u32 signature;
			Skeleton::Type skeletonType;
			file->Get(signature, offset);
			file->Get(skeletonType, offset);
			file->Get(version, offset);

			printf("Skeleton type == %s\n", skeletonType == Skeleton::Type::Classic ? "classic" : "v2");

			switch (skeletonType)
			{
			case Skeleton::Type::Classic:
				state = ReadClassic(file, offset);
				break;

			case Skeleton::Type::Version2:
				state = ReadVersion2(file, offset);
				break;

			default:
				state = Spek::File::LoadState::FailedToLoad;
				break;
			};

			printf("Skeleton %s with %llu bones.\n", state == Spek::File::LoadState::FailedToLoad ? "failed to load" : "was loaded", bones.size());
			if (onLoadFunction)
				onLoadFunction(*this);
		});
	}

	const Skeleton::Bone* Skeleton::GetBone(uint32_t inNameHash) const
	{
		for (const auto& bone : bones)
			if (bone.hash == inNameHash)
				return &bone;

		return nullptr;
	}

	Spek::File::LoadState Skeleton::ReadClassic(Spek::File::Handle inFile, size_t& inOffset)
	{
		inOffset += sizeof(uint32_t);

		uint32_t boneCount;
		inFile->Get(boneCount, inOffset);

		bones.resize(boneCount);

		char name[32];
		for (int i = 0; i < boneCount; i++)
		{
			auto& bone = bones[i];

			inFile->Read((uint8_t*)name, 32, inOffset);
			bone.hash = StringToHash(name);
			bone.name = name;

			bone.id = i;
			inFile->Read((uint8_t*)(&bone.parentID), 4, inOffset);
			if (bone.parentID >= 0)
			{
				bone.parent = &bones[bone.parentID];
				bone.parent->children.push_back(&bone);
			}
			else
			{
				bone.parent = nullptr;
			}

			float scale;
			inFile->Get(scale, inOffset);

			for (int y = 0; y < 3; y++)
				for (int x = 0; x < 4; x++)
					inFile->Get(bone.global[x][y], inOffset);

			bone.global[3] [3] = 1.0f;
			bone.inverseGlobal = glm::inverse(bone.global);
		}

		if (version < 2)
		{
			boneIndices.resize(boneCount);
			for (int i = 0; i < boneCount; i++)
				boneIndices[i] = i;
		}
		else if (version == 2)
		{
			uint32_t boneIndexCount;
			inFile->Get(boneIndexCount, inOffset);
			inFile->Get(boneIndices, boneIndexCount, inOffset);
		}

		return Spek::File::LoadState::Loaded;
	}

	Spek::File::LoadState Skeleton::ReadVersion2(Spek::File::Handle inFile, size_t& inOffset)
	{
		inOffset += sizeof(uint16_t);

		uint16_t boneCount;
		inFile->Get(boneCount, inOffset);

		uint32_t boneIndexCount;
		inFile->Get(boneIndexCount, inOffset);

		uint16_t dataOffset;
		inFile->Get(dataOffset, inOffset);

		inOffset += sizeof(uint16_t);

		uint32_t boneIndexMapOffset;
		inFile->Get(boneIndexMapOffset, inOffset);

		uint32_t boneIndicesOffset;
		inFile->Get(boneIndicesOffset, inOffset);

		inOffset += sizeof(uint32_t);
		inOffset += sizeof(uint32_t);

		uint32_t boneNamesOffset;
		inFile->Get(boneNamesOffset, inOffset);

		inOffset = dataOffset;

		bones.resize(boneCount);
		for (int i = 0; i < boneCount; ++i)
		{
			Bone& bone = bones[i];
			inOffset += sizeof(uint16_t);

			inFile->Get(bone.id, inOffset);
			if (bone.id != i)
			{
				printf("League Skeleton noticed an unexpected id value for bone %d: %d\n", i, bone.id);
				return Spek::File::LoadState::FailedToLoad;
			}

			inFile->Get(bone.parentID, inOffset);
			bone.parent = bone.parentID >= 0 ? &bones[bone.parentID] : nullptr;

			inOffset += sizeof(uint16_t);

			inFile->Get(bone.hash, inOffset);

			inOffset += sizeof(uint32_t);

			glm::vec3 position;
			inFile->Get(position.x, inOffset);
			inFile->Get(position.y, inOffset);
			inFile->Get(position.z, inOffset);

			glm::vec3 scale;
			inFile->Get(scale.x, inOffset);
			inFile->Get(scale.y, inOffset);
			inFile->Get(scale.z, inOffset);

			glm::quat rotation;
			inFile->Get(rotation.x, inOffset);
			inFile->Get(rotation.y, inOffset);
			inFile->Get(rotation.z, inOffset);
			inFile->Get(rotation.w, inOffset);

			inOffset += 44;

			bone.local = glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(scale);

			if (bone.parentID != -1)
				bones[bone.parentID].children.push_back(&bone);
		}

		// Unused
		/*inOffset = boneIndexMapOffset;
		std::map<uint32_t, uint32_t> boneIndexMap;
		for (int i = 0; i < boneCount; i++)
		{
			uint32_t skeletonID;
			inFile->Get(skeletonID, inOffset);

			uint32_t animationID;
			inFile->Get(animationID, inOffset);

			boneIndexMap[animationID] = skeletonID;
		}*/

		inOffset = boneIndicesOffset;
		boneIndices.reserve(boneIndexCount);
		for (int i = 0; i < boneIndexCount; i++)
		{
			uint16_t boneIndex;
			inFile->Get(boneIndex, inOffset);
			boneIndices.push_back(boneIndex);
		}

		inOffset = boneNamesOffset;

		// Get file part with bone names
		size_t nameChunkSize = 32 * boneCount;
		std::vector<uint8_t> start(nameChunkSize);
		memset(start.data(), 0, nameChunkSize);
		inFile->Read(start.data(), nameChunkSize, inOffset);

		// Go through all the names and store them on our bones.
		char* pointer = (char*)start.data();
		for (int i = 0; i < boneCount; ++i)
		{
			bones[i].name = pointer;
			size_t nameLength = strlen(pointer);
			std::string name = pointer;
			pointer += nameLength;
			while (*pointer == 0) pointer++; // eat all \0s
		}

		for (auto& bone : bones)
		{
			if (bone.parentID != -1) continue;
			RecursiveInvertGlobalMatrices(glm::identity<glm::mat4>(), bone);
		}

		return Spek::File::LoadState::Loaded;
	}
}
