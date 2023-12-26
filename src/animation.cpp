#include "animation.hpp"

#include <map>
#include <bitset>
#include <unordered_set>

namespace LeagueModel
{
	uint32_t StringToHash(const std::string& inString);

	void Animation::Load(const std::string& inFilePath, OnLoadFunction inOnLoadFunction)
	{
		file = Spek::File::Load(inFilePath.c_str(), [this, inOnLoadFunction](Spek::File::Handle inFile)
		{
			auto state = inFile ? inFile->GetLoadState() : Spek::File::LoadState::FailedToLoad;
			if (state != Spek::File::LoadState::Loaded)
			{
				printf("Animation %s.\n", state == Spek::File::LoadState::FailedToLoad ? "failed to load" : "was not found");
				loadState = state;
				if (inOnLoadFunction)
					inOnLoadFunction(*this);
				return;
			}

			size_t offset = 0;
			uint8_t signature[9];
			inFile->Read(signature, 8, offset);

			if (memcmp(signature, "r3d2anmd", 8) != 0 && memcmp(signature, "r3d2canm", 8) != 0)
			{
				signature[8] = 0;
				printf("Animation has signature %s, which is not known by this application!\n", (char*)signature);

				loadState = Spek::File::LoadState::FailedToLoad;
				if (inOnLoadFunction)
					inOnLoadFunction(*this);
				return;
			}

			uint32_t version;
			inFile->Get(version, offset);
		
			if (version > 5)
			{
				printf("Animation has an unsupported version: %u\n", version);
				loadState = Spek::File::LoadState::FailedToLoad;
				if (inOnLoadFunction)
					inOnLoadFunction(*this);
				return;
			}

			printf("Animation has version: %u\n", version);
			switch (version)
			{
			default:
			{
				printf("Animation has an unsupported version: %u\n", version);
				loadState = Spek::File::LoadState::FailedToLoad;
				if (inOnLoadFunction)
					inOnLoadFunction(*this);
				return;
			}

			case 1:
				state = LoadVersion1(inFile, offset);
				break;

			case 3:
				state = LoadVersion3(inFile, offset);
				break;

			case 4:
				state = LoadVersion4(inFile, offset);
				break;

			case 5:
				state = LoadVersion5(inFile, offset);
				break;
			}

			loadState = state;
			printf("Animation was %s with %llu bones.\n", state == Spek::File::LoadState::FailedToLoad ? "failed to load" : "loaded", bones.size());
			if (inOnLoadFunction)
				inOnLoadFunction(*this);
		});
	}

	glm::quat UncompressQuaternion(const uint16_t& dominantAxis, const uint16_t& inX, const uint16_t& inY, const uint16_t& inZ)
	{
		float fx = sqrt(2.0f) * ((int)inX - 16384) / 32768.0f;
		float fy = sqrt(2.0f) * ((int)inY - 16384) / 32768.0f;
		float fz = sqrt(2.0f) * ((int)inZ - 16384) / 32768.0f;
		float fw = sqrt(1.0f - fx * fx - fy * fy - fz * fz);

		glm::quat uq;

		switch (dominantAxis)
		{
		case 0:
			uq.x = fw;
			uq.y = fx;
			uq.z = fy;
			uq.w = fz;
			break;

		case 1:
			uq.x = fx;
			uq.y = fw;
			uq.z = fy;
			uq.w = fz;
			break;

		case 2:
			uq.x = -fx;
			uq.y = -fy;
			uq.z = -fw;
			uq.w = -fz;
			break;

		case 3:
			uq.x = fx;
			uq.y = fy;
			uq.z = fz;
			uq.w = fw;
			break;
		}

		return uq;
	}

	glm::vec3 UncompressVec3(const glm::vec3& inMin, const glm::vec3& inMax, const uint16_t& inX, const uint16_t& inY, const uint16_t& inZ)
	{
		glm::vec3 uncompressed;

		uncompressed = inMax - inMin;

		uncompressed.x *= (inX / 65535.0f);
		uncompressed.y *= (inY / 65535.0f);
		uncompressed.z *= (inZ / 65535.0f);

		uncompressed = uncompressed + inMin;

		return uncompressed;
	}

	float UncompressTime(const uint16_t& inCurrentTime, const float& inAnimationLength)
	{
		float uncompressedTime;

		uncompressedTime = inCurrentTime / 65535.0f;
		uncompressedTime = uncompressedTime * inAnimationLength;

		return uncompressedTime;
	}

	Spek::File::LoadState Animation::LoadVersion1(Spek::File::Handle inFile, size_t& inOffset)
	{
		int32_t fileSize;
		inFile->Read((uint8_t*)(&fileSize), 4, inOffset);
		fileSize += 12;

		inOffset += 8;

		uint32_t frameCount, boneCount;
		inFile->Read((uint8_t*)(&boneCount), 4, inOffset);
		int32_t entryCount;
		inFile->Read((uint8_t*)(&entryCount), 4, inOffset);
		inOffset += 4;

		inFile->Read((uint8_t*)(&duration), 4, inOffset);
		inFile->Read((uint8_t*)(&fps), 4, inOffset);
		frameCount = (size_t)(duration * fps);
		float frameDelay = 1.0f / fps;

		inOffset += 24;

		glm::vec3 translationMin;
		inFile->Read((uint8_t*)(&translationMin[0]), 4, inOffset);
		inFile->Read((uint8_t*)(&translationMin[1]), 4, inOffset);
		inFile->Read((uint8_t*)(&translationMin[2]), 4, inOffset);

		glm::vec3 translationMax;
		inFile->Read((uint8_t*)(&translationMax[0]), 4, inOffset);
		inFile->Read((uint8_t*)(&translationMax[1]), 4, inOffset);
		inFile->Read((uint8_t*)(&translationMax[2]), 4, inOffset);

		glm::vec3 scaleMin;
		inFile->Read((uint8_t*)(&scaleMin[0]), 4, inOffset);
		inFile->Read((uint8_t*)(&scaleMin[1]), 4, inOffset);
		inFile->Read((uint8_t*)(&scaleMin[2]), 4, inOffset);

		glm::vec3 scaleMax;
		inFile->Read((uint8_t*)(&scaleMax[0]), 4, inOffset);
		inFile->Read((uint8_t*)(&scaleMax[1]), 4, inOffset);
		inFile->Read((uint8_t*)(&scaleMax[2]), 4, inOffset);

		uint32_t entriesOffset, indicesOffset, hashesOffset;
		inFile->Read((uint8_t*)(&entriesOffset), 4, inOffset);
		inFile->Read((uint8_t*)(&indicesOffset), 4, inOffset);
		inFile->Read((uint8_t*)(&hashesOffset), 4, inOffset);

		entriesOffset += 12;
		indicesOffset += 12;
		hashesOffset += 12;

		std::vector<uint32_t> hashEntries;
		inOffset = hashesOffset;
		for (uint32_t i = 0; i < boneCount; ++i)
		{
			uint32_t hash;
			inFile->Read((uint8_t*)(&hash), 4, inOffset);
			hashEntries.push_back(hash);
		}

		inOffset = entriesOffset;

		std::vector<std::vector<std::pair<uint16_t, std::bitset<48>>>> compressedRotations, compressedTranslations, compressedScales;
		compressedRotations.resize(boneCount);
		compressedTranslations.resize(boneCount);
		compressedScales.resize(boneCount);

		for (size_t i = 0; i < entryCount; ++i)
		{
			enum FrameDataType : uint8_t
			{
				RotationType = 0,
				TranslationType = 64,
				ScaleType = 128
			};
			static_assert(sizeof(FrameDataType) == sizeof(uint8_t), "Cannot use FrameDataType as a standin for uint8_t!");

			uint16_t compressedTime;
			inFile->Read((uint8_t*)(&compressedTime), 2, inOffset);

			uint8_t hashIndex;
			inFile->Read((uint8_t*)(&hashIndex), 1, inOffset);

			FrameDataType dataType;
			inFile->Read((uint8_t*)(&dataType), 1, inOffset);

			std::bitset<48> compressedData;
			inFile->Read((uint8_t*)(&compressedData), 6, inOffset);

			uint32_t boneHash = hashEntries[hashIndex];
			assert(hashIndex < boneCount);

			switch (dataType)
			{
			case FrameDataType::RotationType:
				compressedRotations[hashIndex].push_back(std::pair<uint16_t, std::bitset<48>>(compressedTime, compressedData));
				break;

			case FrameDataType::TranslationType:
				compressedTranslations[hashIndex].push_back(std::pair<uint16_t, std::bitset<48>>(compressedTime, compressedData));
				break;

			case FrameDataType::ScaleType:
				compressedScales[hashIndex].push_back(std::pair<uint16_t, std::bitset<48>>(compressedTime, compressedData));
				break;
			}
		}

		for (uint32_t i = 0; i < boneCount; ++i)
		{
			uint32_t boneHash = hashEntries[i];
			Bone& boneEntry = bones.emplace_back();
			boneEntry.hash = boneHash;

			for (auto& compressedTranslation : compressedTranslations[i])
			{
				float time = UncompressTime(compressedTranslation.first, duration);

				std::bitset<48> mask = 0xFFFF;
				uint16_t x = static_cast<uint16_t>((compressedTranslation.second & mask).to_ulong());
				uint16_t y = static_cast<uint16_t>((compressedTranslation.second >> 16 & mask).to_ulong());
				uint16_t z = static_cast<uint16_t>((compressedTranslation.second >> 32 & mask).to_ulong());

				glm::vec3 translation = UncompressVec3(translationMin, translationMax, x, y, z);

				boneEntry.translation.push_back(Bone::TranslationFrame(time, translation));
			}

			for (auto& compressedRotation : compressedRotations[i])
			{
				float time = UncompressTime(compressedRotation.first, duration);

				std::bitset<48> mask = 0x7FFF;
				uint16_t dominantAxis = static_cast<uint16_t>((compressedRotation.second >> 45).to_ulong());
				uint16_t x = static_cast<uint16_t>((compressedRotation.second >> 30 & mask).to_ulong());
				uint16_t y = static_cast<uint16_t>((compressedRotation.second >> 15 & mask).to_ulong());
				uint16_t z = static_cast<uint16_t>((compressedRotation.second & mask).to_ulong());

				glm::quat rotation = UncompressQuaternion(dominantAxis, x, y, z);

				boneEntry.rotation.push_back(Bone::RotationFrame(time, rotation));
			}

			for (auto& compressedScale : compressedScales[i])
			{
				float time = UncompressTime(compressedScale.first, duration);

				std::bitset<48> mask = 0xFFFF;
				uint16_t x = static_cast<uint16_t>((compressedScale.second & mask).to_ulong());
				uint16_t y = static_cast<uint16_t>((compressedScale.second >> 16 & mask).to_ulong());
				uint16_t z = static_cast<uint16_t>((compressedScale.second >> 32 & mask).to_ulong());

				glm::vec3 scale = UncompressVec3(scaleMin, scaleMax, x, y, z);
				boneEntry.scale.push_back(Bone::ScaleFrame(time, scale));
			}
		}

		return Spek::File::LoadState::Loaded;
	}

	Spek::File::LoadState Animation::LoadVersion3(Spek::File::Handle inFile, size_t& inOffset)
	{
		inOffset += 4;
		uint32_t boneCount, frameCount;

		inFile->Read((uint8_t*)(&boneCount), 4, inOffset);
		inFile->Read((uint8_t*)(&frameCount), 4, inOffset);

		uint32_t ufps;
		inFile->Get(ufps, inOffset);
		fps = ufps;

		float frameDelay = 1.0f / fps;
		duration = frameDelay * frameCount;

		bones.resize(boneCount);

		for (uint32_t i = 0; i < boneCount; ++i)
		{
			char name[32];
			inFile->Read((uint8_t*)name, 32, inOffset);

			bones[i].hash = StringToHash(name);

			inOffset += 4;

			bones[i].translation.resize(frameCount);
			bones[i].rotation.resize(frameCount);
			bones[i].scale.resize(frameCount);

			float cumulativeFrameDelay = 0.0f;

			for (uint32_t j = 0; j < frameCount; ++j)
			{
				bones[i].rotation[j].time = cumulativeFrameDelay;
				bones[i].translation[j].time = cumulativeFrameDelay;
				bones[i].scale[j].time = cumulativeFrameDelay;

				inFile->Read((uint8_t*)(&bones[i].rotation[j].frameData.x), 4, inOffset);
				inFile->Read((uint8_t*)(&bones[i].rotation[j].frameData.y), 4, inOffset);
				inFile->Read((uint8_t*)(&bones[i].rotation[j].frameData.z), 4, inOffset);
				inFile->Read((uint8_t*)(&bones[i].rotation[j].frameData.w), 4, inOffset);

				inFile->Read((uint8_t*)(&bones[i].translation[j].frameData.x), 4, inOffset);
				inFile->Read((uint8_t*)(&bones[i].translation[j].frameData.y), 4, inOffset);
				inFile->Read((uint8_t*)(&bones[i].translation[j].frameData.z), 4, inOffset);

				bones[i].scale[j].frameData[0] = bones[i].scale[j].frameData[1] = bones[i].scale[j].frameData[2] = 1.0f;
				cumulativeFrameDelay += frameDelay;
			}
		}

		return Spek::File::LoadState::Loaded;
	}

	Spek::File::LoadState Animation::LoadVersion4(Spek::File::Handle inFile, size_t& inOffset)
	{
		inOffset += 16;
		uint32_t boneCount, frameCount;
		float frameDelay;
		inFile->Read((uint8_t*)(&boneCount), 4, inOffset);
		inFile->Read((uint8_t*)(&frameCount), 4, inOffset);
		inFile->Read((uint8_t*)(&frameDelay), 4, inOffset);
		inOffset += 12;

		duration = frameDelay * frameCount;
		fps = 1.0f / frameDelay;

		uint32_t translationDataOffset, rotationDataOffset, frameDataOffset;
		inFile->Read((uint8_t*)(&translationDataOffset), 4, inOffset);
		translationDataOffset += 12;
		inFile->Read((uint8_t*)(&rotationDataOffset), 4, inOffset);
		rotationDataOffset += 12;
		inFile->Read((uint8_t*)(&frameDataOffset), 4, inOffset);
		frameDataOffset += 12;

		std::vector<glm::vec3> translationOrScaleEntries;
		inOffset = translationDataOffset;
		while (inOffset != rotationDataOffset)
		{
			glm::vec3 vector;
			inFile->Read((uint8_t*)(&vector.x), 4, inOffset);
			inFile->Read((uint8_t*)(&vector.y), 4, inOffset);
			inFile->Read((uint8_t*)(&vector.z), 4, inOffset);
			translationOrScaleEntries.push_back(vector);
		}

		std::vector<glm::quat> rotationEntries;
		inOffset = rotationDataOffset;
		while (inOffset != frameDataOffset)
		{
			glm::quat rotationEntry;
			inFile->Read((uint8_t*)(&rotationEntry.x), 4, inOffset);
			inFile->Read((uint8_t*)(&rotationEntry.y), 4, inOffset);
			inFile->Read((uint8_t*)(&rotationEntry.z), 4, inOffset);
			inFile->Read((uint8_t*)(&rotationEntry.w), 4, inOffset);
			rotationEntries.push_back(rotationEntry);
		}

		struct FrameIndices
		{
			uint16_t TranslationIndex;
			uint16_t RotationIndex;
			uint16_t ScaleIndex;
		};

		std::map<uint32_t, std::vector<FrameIndices>> boneMap;

		inOffset = frameDataOffset;

		for (uint32_t i = 0; i < boneCount; ++i)
		{
			for (uint32_t j = 0; j < frameCount; ++j)
			{
				uint32_t boneHash;
				FrameIndices frameIndexData;

				inFile->Read((uint8_t*)(&boneHash), 4, inOffset);
				inFile->Read((uint8_t*)(&frameIndexData.TranslationIndex), 2, inOffset);
				inFile->Read((uint8_t*)(&frameIndexData.ScaleIndex), 2, inOffset);
				inFile->Read((uint8_t*)(&frameIndexData.RotationIndex), 2, inOffset);
				inOffset += 2;

				boneMap[boneHash].push_back(frameIndexData);
			}
		}

		for (auto& boneIndex : boneMap)
		{
			float currentTime = 0.0f;
			Bone bone;
			bone.hash = boneIndex.first;
			for (auto& frame : boneIndex.second)
			{
				bone.translation.push_back(Bone::TranslationFrame(currentTime, translationOrScaleEntries[frame.TranslationIndex]));
				bone.rotation.push_back(Bone::RotationFrame(currentTime, rotationEntries[frame.RotationIndex]));
				bone.scale.push_back(Bone::ScaleFrame(currentTime, translationOrScaleEntries[frame.ScaleIndex]));
				currentTime += frameDelay;
			}

			bones.push_back(bone);
		}

		return Spek::File::LoadState::Loaded;
	}

	Spek::File::LoadState Animation::LoadVersion5(Spek::File::Handle inFile, size_t& inOffset)
	{
		int32_t fileSize;
		inFile->Read((uint8_t*)(&fileSize), 4, inOffset);
		fileSize += 12;

		inOffset += 12;

		uint32_t boneCount, frameCount;
		float frameDelay;
		inFile->Read((uint8_t*)(&boneCount), 4, inOffset);
		inFile->Read((uint8_t*)(&frameCount), 4, inOffset);
		inFile->Read((uint8_t*)(&frameDelay), 4, inOffset);

		duration = (float)frameCount * frameDelay;
		fps = (float)frameCount / duration;

		int32_t translationFileOffset, rotationFileOffset, frameFileOffset, hashesOffset;

		inFile->Read((uint8_t*)(&hashesOffset), 4, inOffset);
		inOffset += 8;
		inFile->Read((uint8_t*)(&translationFileOffset), 4, inOffset);
		inFile->Read((uint8_t*)(&rotationFileOffset), 4, inOffset);
		inFile->Read((uint8_t*)(&frameFileOffset), 4, inOffset);

		translationFileOffset += 12;
		rotationFileOffset += 12;
		frameFileOffset += 12;
		hashesOffset += 12;

		std::vector<glm::vec3> translations;

		size_t translationCount = (size_t)(rotationFileOffset - translationFileOffset) / (sizeof(float) * 3);

		inOffset = translationFileOffset;

		for (size_t i = 0; i < translationCount; ++i)
		{
			glm::vec3 translationEntry;
			inFile->Read((uint8_t*)(&translationEntry), 12, inOffset);
			translations.push_back(translationEntry);
		}

		std::vector<std::bitset<48>> rotationEntries;

		size_t rotationCount = (size_t)(hashesOffset - rotationFileOffset) / 6;

		inOffset = rotationFileOffset;

		for (size_t i = 0; i < rotationCount; ++i)
		{
			std::bitset<48> rotationEntry;
			inFile->Read((uint8_t*)(&rotationEntry), 6, inOffset);
			rotationEntries.push_back(rotationEntry);
		}

		std::vector<uint32_t> hashEntries;

		size_t hashCount = (size_t)(frameFileOffset - hashesOffset) / sizeof(uint32_t);

		inOffset = hashesOffset;

		for (size_t i = 0; i < hashCount; ++i)
		{
			uint32_t hashEntry;
			inFile->Read((uint8_t*)(&hashEntry), 4, inOffset);
			hashEntries.push_back(hashEntry);
		}

		bones.resize(boneCount);

		inOffset = frameFileOffset;

		float currentTime = 0.0f;

		for (size_t i = 0; i < frameCount; ++i)
		{
			for (size_t j = 0; j < boneCount; ++j)
			{
				bones[j].hash = hashEntries[j];

				uint16_t translationIndex, rotationIndex, scaleIndex;
				inFile->Read((uint8_t*)(&translationIndex), 2, inOffset);
				inFile->Read((uint8_t*)(&scaleIndex), 2, inOffset);
				inFile->Read((uint8_t*)(&rotationIndex), 2, inOffset);

				bones[j].translation.push_back(Bone::TranslationFrame(currentTime, translations[translationIndex]));

				std::bitset<48> mask = 0x7FFF;
				uint16_t flag = (uint16_t)(rotationEntries[rotationIndex] >> 45).to_ulong();
				uint16_t sx = (uint16_t)(rotationEntries[rotationIndex] >> 30 & mask).to_ulong();
				uint16_t sy = (uint16_t)(rotationEntries[rotationIndex] >> 15 & mask).to_ulong();
				uint16_t sz = (uint16_t)(rotationEntries[rotationIndex] & mask).to_ulong();

				glm::quat rotationEntry = UncompressQuaternion(flag, sx, sy, sz);

				bones[j].rotation.push_back(Bone::RotationFrame(currentTime, rotationEntry));
				bones[j].scale.push_back(Bone::ScaleFrame(currentTime, translations[scaleIndex]));
			}

			currentTime += frameDelay;
		}

		return Spek::File::LoadState::Loaded;
	}

	const Animation::Bone* Animation::GetBone(u32 hash) const
	{
		for (auto& bone : bones)
			if (bone.hash == hash)
				return &bone;
		return nullptr;
	}
}
