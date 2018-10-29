#include "league_model/league_animation.hpp"
#include "league_model/league_skeleton.hpp"

#include <vector>
#include <bitset>
#include <unordered_set>

#include <glm/glm.hpp>

League::Animation::Animation()
{

}

glm::quat uncompressQuaternion(const unsigned short& flag, const unsigned short& sx, const unsigned short& sy, const unsigned short& sz)
{
	float fx = sqrt(2.0f) * ((int)sx - 16384) / 32768.0f;
	float fy = sqrt(2.0f) * ((int)sy - 16384) / 32768.0f;
	float fz = sqrt(2.0f) * ((int)sz - 16384) / 32768.0f;
	float fw = sqrt(1.0f - fx * fx - fy * fy - fz * fz);

	glm::quat uq;

	switch (flag)
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

glm::vec3 uncompressVector(const glm::vec3& min, const glm::vec3& max, const unsigned short& sx, const unsigned short& sy, const unsigned short& sz)
{
	glm::vec3 uv;

	uv = max - min;

	uv.x *= (sx / 65535.0f);
	uv.y *= (sy / 65535.0f);
	uv.z *= (sz / 65535.0f);

	uv = uv + min;

	return uv;
}

float uncompressTime(const unsigned short& ct, const float& animationLength)
{
	float ut;

	ut = ct / 65535.0f;
	ut = ut * animationLength;

	return ut;
}

unsigned int StringToHash(const std::string& s)
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


void League::Animation::Load(const std::string & a_FileName, const League::Skeleton& a_Skeleton, League::Animation::OnLoadFunction a_OnLoadFunction)
{
	FileSystem t_FileSystem;
	t_FileSystem.OpenFile(a_FileName, [&](BaseFile* a_File, BaseFile::LoadState a_LoadState)
	{
		if (a_LoadState != BaseFile::LoadState::Loaded)
		{
			if (a_OnLoadFunction) a_OnLoadFunction(nullptr, a_LoadState);
			return;
		}

		std::map<uint32_t, std::string> boneHashes;
		uint8_t magicNumber[8];
		a_File->Read(magicNumber, 8);

		if (memcmp(magicNumber, "r3d2anmd", 8) != 0 && memcmp(magicNumber, "r3d2canm", 8) != 0)
		{
			if (a_OnLoadFunction) a_OnLoadFunction(nullptr, BaseFile::LoadState::FailedToLoad);
			return;
		}

		int version;
		a_File->Read((uint8_t*)&version, 4);

		int numBones, numFrames;
		float frameDelay;

		if (version > 5)
		{
			if (a_OnLoadFunction) a_OnLoadFunction(nullptr, BaseFile::LoadState::FailedToLoad);
			return;
		}

		if (version == 1)
		{
			int fileSize;
			a_File->Read((uint8_t*)&fileSize, 4);
			fileSize += 12;
			a_File->Seek(8, BaseFile::SeekType::FromCurrent);

			a_File->Read((uint8_t*)&numBones, 4);
			int numEntries;
			a_File->Read((uint8_t*)&numEntries, 4);
			a_File->Seek(4, BaseFile::SeekType::FromCurrent);

			float animationLength;
			a_File->Read((uint8_t*)&animationLength, 4);
			float framesPerSecond;
			a_File->Read((uint8_t*)&framesPerSecond, 4);
			numFrames = (int)(animationLength * framesPerSecond);
			frameDelay = 1.0f / framesPerSecond;

			a_File->Seek(24, BaseFile::SeekType::FromCurrent);

			glm::vec3 minTranslation;
			a_File->Read((uint8_t*)&minTranslation[0], 4);
			a_File->Read((uint8_t*)&minTranslation[1], 4);
			a_File->Read((uint8_t*)&minTranslation[2], 4);

			glm::vec3 maxTranslation;
			a_File->Read((uint8_t*)&maxTranslation[0], 4);
			a_File->Read((uint8_t*)&maxTranslation[1], 4);
			a_File->Read((uint8_t*)&maxTranslation[2], 4);

			glm::vec3 minScale;
			a_File->Read((uint8_t*)&minScale[0], 4);
			a_File->Read((uint8_t*)&minScale[1], 4);
			a_File->Read((uint8_t*)&minScale[2], 4);

			glm::vec3 maxScale;
			a_File->Read((uint8_t*)&maxScale[0], 4);
			a_File->Read((uint8_t*)&maxScale[1], 4);
			a_File->Read((uint8_t*)&maxScale[2], 4);

			int entriesOffset, indicesOffset, hashesOffset;
			a_File->Read((uint8_t*)&entriesOffset, 4);
			a_File->Read((uint8_t*)&indicesOffset, 4);
			a_File->Read((uint8_t*)&hashesOffset, 4);

			entriesOffset += 12;
			indicesOffset += 12;
			hashesOffset += 12;

			const int hashBlock = 4;
			std::vector<unsigned int> hashEntries;

			a_File->Seek(hashesOffset, BaseFile::SeekType::FromBeginning);

			for (int i = 0; i < numBones; ++i)
			{
				unsigned int hashEntry;
				a_File->Read((uint8_t*)&hashEntry, 4);
				hashEntries.push_back(hashEntry);
			}

			a_File->Seek(entriesOffset, BaseFile::SeekType::FromBeginning);

			const unsigned char quaternionType = 0;
			const unsigned char translationType = 64;
			const unsigned char scaleType = 128;

			std::vector<std::vector<std::pair<unsigned short, std::bitset<48>>>> compressedQuaternions, compressedTranslations, compressedScales;
			compressedQuaternions.resize(numBones);
			compressedTranslations.resize(numBones);
			compressedScales.resize(numBones);

			for (int i = 0; i < numEntries; ++i)
			{
				unsigned short compressedTime;
				a_File->Read((uint8_t*)&compressedTime, 2);

				unsigned char hashIndex;
				a_File->Read((uint8_t*)&hashIndex, 1);

				unsigned char dataType;
				a_File->Read((uint8_t*)&dataType, 1);

				std::bitset<48> compressedData;
				a_File->Read((uint8_t*)&compressedData, 6);

				int boneHash = hashEntries.at(hashIndex);

				if (dataType == quaternionType)
				{
					compressedQuaternions.at(hashIndex).push_back(std::pair<unsigned short, std::bitset<48>>(compressedTime, compressedData));
				}

				else if (dataType == translationType)
				{
					compressedTranslations.at(hashIndex).push_back(std::pair<unsigned short, std::bitset<48>>(compressedTime, compressedData));
				}

				else if (dataType == scaleType)
				{
					compressedScales.at(hashIndex).push_back(std::pair<unsigned short, std::bitset<48>>(compressedTime, compressedData));
				}
			}

			for (int i = 0; i < numBones; ++i)
			{
				unsigned int boneHash = hashEntries[i];

				if (boneHashes.find(boneHash) == boneHashes.end())
				{
					continue;
				}

				Bone boneEntry;
				boneEntry.name = boneHashes[boneHash].c_str();

				std::unordered_set<short> translationsTimeSet;

				for (auto &t : compressedTranslations[i])
				{
					auto res = translationsTimeSet.insert(t.first);

					if (!res.second)
					{
						continue;
					}

					float uncompressedTime = uncompressTime(t.first, animationLength);

					std::bitset<48> mask = 0xFFFF;
					unsigned short sx = static_cast<unsigned short>((t.second & mask).to_ulong());
					unsigned short sy = static_cast<unsigned short>((t.second >> 16 & mask).to_ulong());
					unsigned short sz = static_cast<unsigned short>((t.second >> 32 & mask).to_ulong());

					glm::vec3 translationEntry = uncompressVector(minTranslation, maxTranslation, sx, sy, sz);

					boneEntry.translation.push_back(std::pair<float, glm::vec3>(uncompressedTime, translationEntry));
				}

				std::unordered_set<short> rotationsTimeSet;

				for (auto &r : compressedQuaternions[i])
				{

					auto res = rotationsTimeSet.insert(r.first);

					if (!res.second)
					{
						continue;
					}

					float uncompressedTime = uncompressTime(r.first, animationLength);

					std::bitset<48> mask = 0x7FFF;
					unsigned short flag = static_cast<unsigned short>((r.second >> 45).to_ulong());
					unsigned short sx = static_cast<unsigned short>((r.second >> 30 & mask).to_ulong());
					unsigned short sy = static_cast<unsigned short>((r.second >> 15 & mask).to_ulong());
					unsigned short sz = static_cast<unsigned short>((r.second & mask).to_ulong());

					glm::quat quaterionEntry = uncompressQuaternion(flag, sx, sy, sz);

					boneEntry.quaternion.push_back(std::pair<float, glm::quat>(uncompressedTime, quaterionEntry));
				}

				std::unordered_set<short> scaleTimeSet;

				for (auto &s : compressedScales[i])
				{

					auto res = scaleTimeSet.insert(s.first);

					if (!res.second)
					{
						continue;
					}

					float uncompressedTime = uncompressTime(s.first, animationLength);

					std::bitset<48> mask = 0xFFFF;
					unsigned short sx = static_cast<unsigned short>((s.second & mask).to_ulong());
					unsigned short sy = static_cast<unsigned short>((s.second >> 16 & mask).to_ulong());
					unsigned short sz = static_cast<unsigned short>((s.second >> 32 & mask).to_ulong());

					glm::vec3 scaleEntry = uncompressVector(minScale, maxScale, sx, sy, sz);

					boneEntry.scale.push_back(std::pair<float, glm::vec3>(uncompressedTime, scaleEntry));
				}

				Bones.push_back(boneEntry);
			}
		}

		else if (version == 3)
		{
			a_File->Seek(4, BaseFile::SeekType::FromCurrent);
			a_File->Read((uint8_t*)(&numBones), 4);
			a_File->Read((uint8_t*)(&numFrames), 4);
			int framesPerSecond;
			a_File->Read((uint8_t*)(&framesPerSecond), 4);
			frameDelay = 1.0f / framesPerSecond;

			Bones.resize(numBones);

			for (int i = 0; i < numBones; ++i)
			{
				char name[32];
				a_File->Read((uint8_t*)name, 32);

				unsigned int boneHash = StringToHash(name);

				if (boneHashes.find(boneHash) != boneHashes.end())
				{
					Bones[i].name = boneHashes.at(boneHash);
				}

				else
				{
					Bones[i].name = name;
				}

				a_File->Seek(4, BaseFile::SeekType::FromCurrent);

				Bones[i].translation.resize(numFrames);
				Bones[i].quaternion.resize(numFrames);
				Bones[i].scale.resize(numFrames);

				float cumulativeFrameDelay = 0.0f;

				for (int j = 0; j < numFrames; ++j)
				{
					Bones[i].quaternion[j].first = cumulativeFrameDelay;
					Bones[i].translation[j].first = cumulativeFrameDelay;
					Bones[i].scale[j].first = cumulativeFrameDelay;
					a_File->Read((uint8_t*)(&Bones[i].quaternion[j].second), 16);
					a_File->Read((uint8_t*)(&Bones[i].translation[j].second), 12);

					Bones[i].scale[j].second[0] = Bones[i].scale[j].second[1] = Bones[i].scale[j].second[2] = 1.0f;
					cumulativeFrameDelay += frameDelay;
				}
			}
		}

		else if (version == 4)
		{
			a_File->Seek(16, BaseFile::SeekType::FromCurrent);
			a_File->Read((uint8_t*)&numBones, 4);
			a_File->Read((uint8_t*)&numFrames, 4);
			a_File->Read((uint8_t*)&frameDelay, 4);
			a_File->Seek(12, BaseFile::SeekType::FromCurrent);

			int translationsOffset, quaternionsOffset, framesOffset;
			a_File->Read((uint8_t*)&translationsOffset, 4);
			translationsOffset += 12;
			a_File->Read((uint8_t*)&quaternionsOffset, 4);
			quaternionsOffset += 12;
			a_File->Read((uint8_t*)&framesOffset, 4);
			framesOffset += 12;

			const int positionBlockSize = 12;
			std::vector<glm::vec3> translationEntries;

			int numTranslationEntries = (int)(quaternionsOffset - translationsOffset) / positionBlockSize;

			a_File->Seek(translationsOffset, BaseFile::SeekType::FromBeginning);

			for (int i = 0; i < numTranslationEntries; ++i)
			{
				glm::vec3 translationEntry;
				a_File->Read((uint8_t*)&translationEntry, 12);
				translationEntries.push_back(translationEntry);
			}

			const int quaternionBlockSize = 16;
			std::vector<glm::quat> quaternionEntries;

			int numQuaternionEntries = (int)(framesOffset - quaternionsOffset) / quaternionBlockSize;

			a_File->Seek(quaternionsOffset, BaseFile::SeekType::FromBeginning);

			for (int i = 0; i < numQuaternionEntries; ++i)
			{
				glm::quat quaternionEntry;
				a_File->Read((uint8_t*)&quaternionEntry.x, 4);
				a_File->Read((uint8_t*)&quaternionEntry.y, 4);
				a_File->Read((uint8_t*)&quaternionEntry.z, 4);
				a_File->Read((uint8_t*)&quaternionEntry.w, 4);
				quaternionEntries.push_back(quaternionEntry);
			}

			struct FrameIndices
			{
				short translationIndex;
				short quaternionIndex;
				short scaleIndex;
			};

			std::map<unsigned int, std::vector<FrameIndices>> boneMap;

			a_File->Seek(framesOffset, BaseFile::SeekType::FromBeginning);

			for (int i = 0; i < numBones; ++i)
			{
				for (int j = 0; j < numFrames; ++j)
				{
					unsigned int boneHash;
					FrameIndices fi;

					a_File->Read((uint8_t*)&boneHash, 4);
					a_File->Read((uint8_t*)&fi.translationIndex, 2);
					a_File->Read((uint8_t*)&fi.scaleIndex, 2);
					a_File->Read((uint8_t*)&fi.quaternionIndex, 2);
					a_File->Seek(2, BaseFile::SeekType::FromCurrent);

					boneMap[boneHash].push_back(fi);
				}
			}

			for (auto& i : boneMap)
			{
				float cumulativeFrameDelay = 0.0f;

				if (boneHashes.find(i.first) == boneHashes.end())
				{
					continue;
				}

				Bone boneEntry;
				boneEntry.name = boneHashes[i.first].c_str();

				for (auto& f : i.second)
				{
					boneEntry.translation.push_back(std::pair<float, glm::vec3>(cumulativeFrameDelay, translationEntries[f.translationIndex]));
					boneEntry.quaternion.push_back(std::pair<float, glm::quat>(cumulativeFrameDelay, quaternionEntries[f.quaternionIndex]));
					boneEntry.scale.push_back(std::pair<float, glm::vec3>(cumulativeFrameDelay, translationEntries[f.scaleIndex]));
					cumulativeFrameDelay += frameDelay;
				}

				Bones.push_back(boneEntry);
			}
		}

		else if (version == 5)
		{
			int fileSize;
			a_File->Read((uint8_t*)&fileSize, 4);
			fileSize += 12;

			a_File->Seek(12, BaseFile::SeekType::FromCurrent);

			a_File->Read((uint8_t*)&numBones, 4);
			a_File->Read((uint8_t*)&numFrames, 4);
			a_File->Read((uint8_t*)&frameDelay, 4);

			int translationsOffset, quaternionsOffset, framesOffset, hashesOffset;

			a_File->Read((uint8_t*)&hashesOffset, 4);
			a_File->Seek(8, BaseFile::SeekType::FromCurrent);
			a_File->Read((uint8_t*)&translationsOffset, 4);
			a_File->Read((uint8_t*)&quaternionsOffset, 4);
			a_File->Read((uint8_t*)&framesOffset, 4);

			translationsOffset += 12;
			quaternionsOffset += 12;
			framesOffset += 12;
			hashesOffset += 12;

			const int translationBlock = 12;
			std::vector<glm::vec3> translationEntries;

			int numTranslationEntries = (int)(quaternionsOffset - translationsOffset) / translationBlock;

			a_File->Seek(translationsOffset, BaseFile::SeekType::FromBeginning);

			for (int i = 0; i < numTranslationEntries; ++i)
			{
				glm::vec3 translationEntry;
				a_File->Read((uint8_t*)&translationEntry, 12);
				translationEntries.push_back(translationEntry);
			}

			const int quaternionBlock = 6;
			std::vector<std::bitset<48>> quaternionEntries;

			int numQuaternionEntries = (int)(hashesOffset - quaternionsOffset) / quaternionBlock;

			a_File->Seek(quaternionsOffset, BaseFile::SeekType::FromBeginning);

			for (int i = 0; i < numQuaternionEntries; ++i)
			{
				std::bitset<48> quaternionEntry;
				a_File->Read((uint8_t*)&quaternionEntry, 6);
				quaternionEntries.push_back(quaternionEntry);
			}

			const int hashBlock = 4;
			std::vector<unsigned int> hashEntries;

			int numHashEntries = (int)(framesOffset - hashesOffset) / hashBlock;

			a_File->Seek(hashesOffset, BaseFile::SeekType::FromBeginning);

			for (int i = 0; i < numHashEntries; ++i)
			{
				unsigned int hashEntry;
				a_File->Read((uint8_t*)&hashEntry, 4);
				hashEntries.push_back(hashEntry);
			}

			Bones.resize(numBones);

			a_File->Seek(framesOffset, BaseFile::SeekType::FromBeginning);

			float cumulativeFrameDelay = 0.0f;

			for (int i = 0; i < numFrames; ++i)
			{
				for (int j = 0; j < numBones; ++j)
				{
					if (boneHashes.find(hashEntries[j]) != boneHashes.end())
					{
						Bones[j].name = boneHashes.at(hashEntries[j]).c_str();
					}

					short translationIndex, quaternionIndex, scaleIndex;
					a_File->Read((uint8_t*)&translationIndex, 2);
					a_File->Read((uint8_t*)&scaleIndex, 2);
					a_File->Read((uint8_t*)&quaternionIndex, 2);

					Bones[j].translation.push_back(std::pair<float, glm::vec3>(cumulativeFrameDelay, translationEntries.at(translationIndex)));

					std::bitset<48> mask = 0x7FFF;
					unsigned short flag = static_cast<unsigned short>((quaternionEntries.at(quaternionIndex) >> 45).to_ulong());
					unsigned short sx = static_cast<unsigned short>((quaternionEntries.at(quaternionIndex) >> 30 & mask).to_ulong());
					unsigned short sy = static_cast<unsigned short>((quaternionEntries.at(quaternionIndex) >> 15 & mask).to_ulong());
					unsigned short sz = static_cast<unsigned short>((quaternionEntries.at(quaternionIndex) & mask).to_ulong());

					glm::quat quaterionEntry = uncompressQuaternion(flag, sx, sy, sz);

					Bones[j].quaternion.push_back(std::pair<float, glm::quat>(cumulativeFrameDelay, quaterionEntry));

					Bones[j].scale.push_back(std::pair<float, glm::vec3>(cumulativeFrameDelay, translationEntries.at(scaleIndex)));
				}

				cumulativeFrameDelay += frameDelay;
			}

			auto it = Bones.begin();

			while (it != Bones.end())
			{
				if (!isalpha(it->name[0]))
				{
					it = Bones.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		if (a_OnLoadFunction) a_OnLoadFunction(this, BaseFile::LoadState::Loaded);
	});
}
