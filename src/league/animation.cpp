#include "animation.hpp"
#include "skeleton.hpp"
#include <string>

#include <bitset>
#include <unordered_set>

uint32_t StringToHash(const std::string& a_String);

League::Animation::Animation(Skeleton & a_Skeleton) :
	m_Skeleton(a_Skeleton)
{
}

void League::Animation::Load(std::string a_FilePath, OnLoadFunction a_OnLoadFunction, void * a_Argument)
{
	struct LoadData
	{
		LoadData(Animation* a_Target, OnLoadFunction a_Function, void* a_Argument) :
			Target(a_Target), OnLoadFunction(a_Function), Argument(a_Argument)
		{}

		Animation* Target;
		OnLoadFunction OnLoadFunction;
		void* Argument;
	};
	auto* t_LoadData = new LoadData(this, a_OnLoadFunction, a_Argument);

	auto* t_File = FileSystem::GetFile(a_FilePath);
	t_File->Load([](File* a_File, File::LoadState a_LoadState, void* a_Argument)
	{
		auto* t_LoadData = (LoadData*)a_Argument;
		auto* t_Animation = (Animation*)t_LoadData->Target;

		if (a_LoadState != File::LoadState::Loaded)
		{
			t_Animation->m_State = a_LoadState;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Animation, t_LoadData->Argument);

			FileSystem::CloseFile(*a_File);
			delete t_LoadData;
			return;
		}

		size_t t_Offset = 0;
		uint8_t t_Signature[8];
		a_File->Read(t_Signature, 8, t_Offset);

		if (memcmp(t_Signature, "r3d2anmd", 8) != 0 && memcmp(t_Signature, "r3d2canm", 8) != 0)
		{
			t_Animation->m_State = File::LoadState::FailedToLoad;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Animation, t_LoadData->Argument);

			FileSystem::CloseFile(*a_File);
			delete t_LoadData;
			return;
		}

		uint32_t t_Version;
		a_File->Get(t_Version, t_Offset);

		std::map<uint32_t, std::string> t_BoneNameHashes;
		auto t_Bones = t_Animation->m_Skeleton.GetBones();
		for (auto& t_Bone : t_Bones)
			t_BoneNameHashes[t_Bone.Hash] = t_Bone.Name;
		
		if (t_Version > 5)
		{
			t_Animation->m_State = File::LoadState::FailedToLoad;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Animation, t_LoadData->Argument);

			FileSystem::CloseFile(*a_File);
			delete t_LoadData;
			return;
		}

		switch (t_Version)
		{
		default:
		{
			t_Animation->m_State = File::LoadState::FailedToLoad;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Animation, t_LoadData->Argument);

			FileSystem::CloseFile(*a_File);
			delete t_LoadData;
			return;
		}

		case 1:
			t_Animation->m_State = t_Animation->LoadVersion1(t_BoneNameHashes, *a_File, t_Offset);
			break;

		case 3:
			t_Animation->m_State = t_Animation->LoadVersion3(t_BoneNameHashes, *a_File, t_Offset);
			break;

		case 4:
			t_Animation->m_State = t_Animation->LoadVersion4(t_BoneNameHashes, *a_File, t_Offset);
			break;
		}

		if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Animation, t_LoadData->Argument);

		FileSystem::CloseFile(*a_File);
		delete t_LoadData;
	}, t_LoadData);
}


glm::quat UncompressQuaternion(const uint16_t& t_DominantAxis, const uint16_t& a_X, const uint16_t& a_Y, const uint16_t& a_Z)
{
	float fx = sqrt(2.0f) * ((int)a_X - 16384) / 32768.0f;
	float fy = sqrt(2.0f) * ((int)a_Y - 16384) / 32768.0f;
	float fz = sqrt(2.0f) * ((int)a_Z - 16384) / 32768.0f;
	float fw = sqrt(1.0f - fx * fx - fy * fy - fz * fz);
	
	glm::quat uq;

	switch (t_DominantAxis)
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

glm::vec3 UncompressVec3(const glm::vec3& a_Min, const glm::vec3& a_Max, const uint16_t& a_X, const uint16_t& a_Y, const uint16_t& a_Z)
{
	glm::vec3 t_Uncompressed;

	t_Uncompressed = a_Max - a_Min;

	t_Uncompressed.x *= (a_X / 65535.0f);
	t_Uncompressed.y *= (a_Y / 65535.0f);
	t_Uncompressed.z *= (a_Z / 65535.0f);

	t_Uncompressed = t_Uncompressed + a_Min;

	return t_Uncompressed;
}

float UncompressTime(const uint16_t& a_CurrentTime, const float& a_AnimationLength)
{
	float t_UncompressedTime;

	t_UncompressedTime = a_CurrentTime / 65535.0f;
	t_UncompressedTime = t_UncompressedTime * a_AnimationLength;

	return t_UncompressedTime;
}

const League::Animation::Bone* League::Animation::GetBone(std::string a_Name) const
{
	for (const auto& t_Bone : m_Bones)
		if (t_Bone.Name == a_Name)
			return &t_Bone;

	return nullptr;
}

File::LoadState League::Animation::LoadVersion1(const std::map<uint32_t, std::string>& a_BoneNameHashes, File& a_File, size_t & a_Offset)
{
	int32_t t_FileSize;
	a_File.Read((uint8_t*)(&t_FileSize), 4, a_Offset);
	t_FileSize += 12;

	a_Offset += 8;

	uint32_t t_FrameCount, t_BoneCount;
	a_File.Read((uint8_t*)(&t_BoneCount), 4, a_Offset);
	int32_t t_EntryCount;
	a_File.Read((uint8_t*)(&t_EntryCount), 4, a_Offset);
	a_Offset += 4;

	a_File.Read((uint8_t*)(&m_Duration), 4, a_Offset);
	a_File.Read((uint8_t*)(&m_FPS), 4, a_Offset);
	t_FrameCount = (size_t)(m_Duration * m_FPS);
	float t_FrameDelay = 1.0f / m_FPS;

	a_Offset += 24;

	glm::vec3 t_TranslationMin;
	a_File.Read((uint8_t*)(&t_TranslationMin[0]), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_TranslationMin[1]), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_TranslationMin[2]), 4, a_Offset);

	glm::vec3 t_TranslationMax;
	a_File.Read((uint8_t*)(&t_TranslationMax[0]), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_TranslationMax[1]), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_TranslationMax[2]), 4, a_Offset);

	glm::vec3 t_ScaleMin;
	a_File.Read((uint8_t*)(&t_ScaleMin[0]), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_ScaleMin[1]), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_ScaleMin[2]), 4, a_Offset);

	glm::vec3 t_ScaleMax;
	a_File.Read((uint8_t*)(&t_ScaleMax[0]), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_ScaleMax[1]), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_ScaleMax[2]), 4, a_Offset);

	int t_EntriesOffset, t_IndicesOffset, t_HashesOffset;
	a_File.Read((uint8_t*)(&t_EntriesOffset), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_IndicesOffset), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_HashesOffset), 4, a_Offset);

	t_EntriesOffset += 12;
	t_IndicesOffset += 12;
	t_HashesOffset += 12;

	std::vector<uint32_t> t_HashEntries;
	a_Offset = t_HashesOffset;
	for (int i = 0; i < t_BoneCount; ++i)
	{
		uint32_t t_Hash;
		a_File.Read((uint8_t*)(&t_Hash), 4, a_Offset);
		t_HashEntries.push_back(t_Hash);
	}

	a_Offset = t_EntriesOffset;
	
	std::vector<std::vector<std::pair<uint16_t, std::bitset<48>>>> t_CompressedRotations, t_CompressedTranslations, t_CompressedScales;
	t_CompressedRotations.resize(t_BoneCount);
	t_CompressedTranslations.resize(t_BoneCount);
	t_CompressedScales.resize(t_BoneCount);

	for (int i = 0; i < t_EntryCount; ++i)
	{
		enum FrameDataType : uint8_t
		{
			RotationType = 0,
			TranslationType = 64,
			ScaleType = 128
		};
		static_assert(sizeof(FrameDataType) == sizeof(uint8_t), "Cannot use FrameDataType as a standin for uint8_t!");

		uint16_t t_CompressedTime;
		a_File.Read((uint8_t*)(&t_CompressedTime), 2, a_Offset);

		uint8_t t_HashIndex;
		a_File.Read((uint8_t*)(&t_HashIndex), 1, a_Offset);

		FrameDataType t_DataType;
		a_File.Read((uint8_t*)(&t_DataType), 1, a_Offset);

		std::bitset<48> t_CompressedData;
		a_File.Read((uint8_t*)(&t_CompressedData), 6, a_Offset);

		int t_BoneHash = t_HashEntries.at(t_HashIndex);

		switch (t_DataType)
		{
		case FrameDataType::RotationType:
			t_CompressedRotations.at(t_HashIndex).push_back(std::pair<uint16_t, std::bitset<48>>(t_CompressedTime, t_CompressedData));
			break;

		case FrameDataType::TranslationType:
			t_CompressedTranslations.at(t_HashIndex).push_back(std::pair<uint16_t, std::bitset<48>>(t_CompressedTime, t_CompressedData));
			break;

		case FrameDataType::ScaleType:
			t_CompressedScales.at(t_HashIndex).push_back(std::pair<uint16_t, std::bitset<48>>(t_CompressedTime, t_CompressedData));
			break;
		}
	}

	for (int i = 0; i < t_BoneCount; ++i)
	{
		unsigned int t_BoneHash = t_HashEntries.at(i);

		if (a_BoneNameHashes.find(t_BoneHash) == a_BoneNameHashes.end())
			continue;

		Bone t_BoneEntry;
		t_BoneEntry.Name = a_BoneNameHashes.at(t_BoneHash);

		for (auto &t_CompressedTranslation : t_CompressedTranslations.at(i))
		{
			float t_Time = UncompressTime(t_CompressedTranslation.first, m_Duration);

			std::bitset<48> t_Mask = 0xFFFF;
			uint16_t t_X = static_cast<uint16_t>((t_CompressedTranslation.second & t_Mask).to_ulong());
			uint16_t t_Y = static_cast<uint16_t>((t_CompressedTranslation.second >> 16 & t_Mask).to_ulong());
			uint16_t t_Z = static_cast<uint16_t>((t_CompressedTranslation.second >> 32 & t_Mask).to_ulong());

			glm::vec3 t_Translation = UncompressVec3(t_TranslationMin, t_TranslationMax, t_X, t_Y, t_Z);

			t_BoneEntry.Translation.push_back(Bone::TranslationFrame(t_Time, t_Translation));
		}

		for (auto &t_CompressedRotation : t_CompressedRotations.at(i))
		{
			float t_Time = UncompressTime(t_CompressedRotation.first, m_Duration);

			std::bitset<48> t_Mask = 0x7FFF;
			uint16_t t_DominantAxis = static_cast<uint16_t>((t_CompressedRotation.second >> 45).to_ulong());
			uint16_t t_X = static_cast<uint16_t>((t_CompressedRotation.second >> 30 & t_Mask).to_ulong());
			uint16_t t_Y = static_cast<uint16_t>((t_CompressedRotation.second >> 15 & t_Mask).to_ulong());
			uint16_t t_Z = static_cast<uint16_t>((t_CompressedRotation.second & t_Mask).to_ulong());

			glm::quat t_Rotation = UncompressQuaternion(t_DominantAxis, t_X, t_Y, t_Z);

			t_BoneEntry.Rotation.push_back(Bone::RotationFrame(t_Time, t_Rotation));
		}

		for (auto &t_CompressedScale : t_CompressedScales.at(i))
		{
			float t_Time = UncompressTime(t_CompressedScale.first, m_Duration);

			std::bitset<48> t_Mask = 0xFFFF;
			uint16_t t_X = static_cast<uint16_t>((t_CompressedScale.second & t_Mask).to_ulong());
			uint16_t t_Y = static_cast<uint16_t>((t_CompressedScale.second >> 16 & t_Mask).to_ulong());
			uint16_t t_Z = static_cast<uint16_t>((t_CompressedScale.second >> 32 & t_Mask).to_ulong());

			glm::vec3 t_Scale = UncompressVec3(t_ScaleMin, t_ScaleMax, t_X, t_Y, t_Z);
			t_BoneEntry.Scale.push_back(Bone::ScaleFrame(t_Time, t_Scale));
		}

		m_Bones.push_back(t_BoneEntry);
	}

	return File::LoadState::Loaded;
}

File::LoadState League::Animation::LoadVersion3(const std::map<uint32_t, std::string>& a_BoneNameHashes, File & a_File, size_t & a_Offset)
{
	a_Offset += 4;
	uint32_t numBones, numFrames;

	a_File.Read((uint8_t*)(&numBones), 4, a_Offset);
	a_File.Read((uint8_t*)(&numFrames), 4, a_Offset);

	uint32_t t_FPS;
	a_File.Get(t_FPS, a_Offset);
	m_FPS = t_FPS;
	
	float frameDelay = 1.0f / m_FPS;
	m_Duration = frameDelay * numFrames;

	m_Bones.resize(numBones);

	for (int i = 0; i < numBones; ++i)
	{
		char Name[32];
		a_File.Read((uint8_t*)Name, 32, a_Offset);

		unsigned int boneHash = StringToHash(Name);

		if (a_BoneNameHashes.find(boneHash) != a_BoneNameHashes.end())
		{
			m_Bones.at(i).Name = a_BoneNameHashes.at(boneHash);
		}

		else
		{
			m_Bones.at(i).Name = Name;
		}

		a_Offset += 4;

		m_Bones.at(i).Translation.resize(numFrames);
		m_Bones.at(i).Rotation.resize(numFrames);
		m_Bones.at(i).Scale.resize(numFrames);

		float cumulativeFrameDelay = 0.0f;

		for (int j = 0; j < numFrames; ++j)
		{
			m_Bones.at(i).Rotation.at(j).Time = cumulativeFrameDelay;
			m_Bones.at(i).Translation.at(j).Time = cumulativeFrameDelay;
			m_Bones.at(i).Scale.at(j).Time = cumulativeFrameDelay;

			a_File.Read((uint8_t*)(&m_Bones.at(i).Rotation.at(j).FrameData.x), 4, a_Offset);
			a_File.Read((uint8_t*)(&m_Bones.at(i).Rotation.at(j).FrameData.y), 4, a_Offset);
			a_File.Read((uint8_t*)(&m_Bones.at(i).Rotation.at(j).FrameData.z), 4, a_Offset);
			a_File.Read((uint8_t*)(&m_Bones.at(i).Rotation.at(j).FrameData.w), 4, a_Offset);

			a_File.Read((uint8_t*)(&m_Bones.at(i).Translation.at(j).FrameData.x), 4, a_Offset);
			a_File.Read((uint8_t*)(&m_Bones.at(i).Translation.at(j).FrameData.y), 4, a_Offset);
			a_File.Read((uint8_t*)(&m_Bones.at(i).Translation.at(j).FrameData.z), 4, a_Offset);

			m_Bones.at(i).Scale.at(j).FrameData[0] = m_Bones.at(i).Scale.at(j).FrameData[1] = m_Bones.at(i).Scale.at(j).FrameData[2] = 1.0f;
			cumulativeFrameDelay += frameDelay;
		}
	}

	return File::LoadState::Loaded;
}

File::LoadState League::Animation::LoadVersion4(const std::map<uint32_t, std::string>& a_BoneNameHashes, File & a_File, size_t & a_Offset)
{
	a_Offset += 16;
	uint32_t t_BoneCount, t_FrameCount;
	float t_FrameDelay;
	a_File.Read((uint8_t*)(&t_BoneCount), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_FrameCount), 4, a_Offset);
	a_File.Read((uint8_t*)(&t_FrameDelay), 4, a_Offset);
	a_Offset += 12;

	m_Duration = t_FrameDelay * t_FrameCount;
	m_FPS = 1.0f / t_FrameDelay;

	int t_TranslationDataOffset, t_RotationDataOffset, t_FrameDataOffset;
	a_File.Read((uint8_t*)(&t_TranslationDataOffset), 4, a_Offset);
	t_TranslationDataOffset += 12;
	a_File.Read((uint8_t*)(&t_RotationDataOffset), 4, a_Offset);
	t_RotationDataOffset += 12;
	a_File.Read((uint8_t*)(&t_FrameDataOffset), 4, a_Offset);
	t_FrameDataOffset += 12;

	std::vector<glm::vec3> t_TranslationOrScaleEntries;
	a_Offset = t_TranslationDataOffset;
	while (a_Offset != t_RotationDataOffset)
	{
		glm::vec3 t_Vector;
		a_File.Read((uint8_t*)(&t_Vector.x), 4, a_Offset);
		a_File.Read((uint8_t*)(&t_Vector.y), 4, a_Offset);
		a_File.Read((uint8_t*)(&t_Vector.z), 4, a_Offset);
		t_TranslationOrScaleEntries.push_back(t_Vector);
	}

	std::vector<glm::quat> t_RotationEntries;
	a_Offset = t_RotationDataOffset;
	while (a_Offset != t_FrameDataOffset)
	{
		glm::quat t_RotationEntry;
		a_File.Read((uint8_t*)(&t_RotationEntry.x), 4, a_Offset);
		a_File.Read((uint8_t*)(&t_RotationEntry.y), 4, a_Offset);
		a_File.Read((uint8_t*)(&t_RotationEntry.z), 4, a_Offset);
		a_File.Read((uint8_t*)(&t_RotationEntry.w), 4, a_Offset);
		t_RotationEntries.push_back(t_RotationEntry);
	}

	struct FrameIndices
	{
		uint16_t TranslationIndex;
		uint16_t RotationIndex;
		uint16_t ScaleIndex;
	};

	std::map<uint32_t, std::vector<FrameIndices>> t_BoneMap;

	a_Offset = t_FrameDataOffset;

	for (int i = 0; i < t_BoneCount; ++i)
	{
		for (int j = 0; j < t_FrameCount; ++j)
		{
			uint32_t t_BoneHash;
			FrameIndices t_FrameIndexData;

			a_File.Read((uint8_t*)(&t_BoneHash), 4, a_Offset);
			a_File.Read((uint8_t*)(&t_FrameIndexData.TranslationIndex), 2, a_Offset);
			a_File.Read((uint8_t*)(&t_FrameIndexData.ScaleIndex), 2, a_Offset);
			a_File.Read((uint8_t*)(&t_FrameIndexData.RotationIndex), 2, a_Offset);
			a_Offset += 2;

			t_BoneMap[t_BoneHash].push_back(t_FrameIndexData);
		}
	}

	for (auto& t_BoneIndex : t_BoneMap)
	{
		float t_CurrentTime = 0.0f;

		auto t_NameHash = a_BoneNameHashes.find(t_BoneIndex.first);
		if (t_NameHash == a_BoneNameHashes.end()) continue;

		Bone t_Bone;
		t_Bone.Name = t_NameHash->second;

		for (auto& t_Frame : t_BoneIndex.second)
		{
			t_Bone.Translation.push_back(Bone::TranslationFrame(t_CurrentTime, t_TranslationOrScaleEntries[t_Frame.TranslationIndex]));
			t_Bone.Rotation.push_back(Bone::RotationFrame(t_CurrentTime, t_RotationEntries[t_Frame.RotationIndex]));
			t_Bone.Scale.push_back(Bone::ScaleFrame(t_CurrentTime, t_TranslationOrScaleEntries[t_Frame.ScaleIndex]));
			t_CurrentTime += t_FrameDelay;
		}

		m_Bones.push_back(t_Bone);
	}

	return File::LoadState::Loaded;
}