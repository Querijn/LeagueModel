#include "animation.hpp"
#include "skeleton.hpp"
#include <string>

#include <bitset>
#include <unordered_set>

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
		}

		if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Animation, t_LoadData->Argument);

		FileSystem::CloseFile(*a_File);
		delete t_LoadData;
	}, t_LoadData);
}


glm::quat uncompressQuaternion(const uint16_t& t_DominantAxis, const uint16_t& t_X, const uint16_t& t_Y, const uint16_t& t_Z)
{
	float fx = sqrt(2.0f) * ((int)t_X - 16384) / 32768.0f;
	float fy = sqrt(2.0f) * ((int)t_Y - 16384) / 32768.0f;
	float fz = sqrt(2.0f) * ((int)t_Z - 16384) / 32768.0f;
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

glm::vec3 uncompressVector(const glm::vec3& min, const glm::vec3& max, const uint16_t& t_X, const uint16_t& t_Y, const uint16_t& t_Z)
{
	glm::vec3 uv;

	uv = max - min;

	uv.x *= (t_X / 65535.0f);
	uv.y *= (t_Y / 65535.0f);
	uv.z *= (t_Z / 65535.0f);

	uv = uv + min;

	return uv;
}

float uncompressTime(const uint16_t& ct, const float& animationLength)
{
	float ut;

	ut = ct / 65535.0f;
	ut = ut * animationLength;

	return ut;
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

	size_t t_FrameCount, t_BoneCount;
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
			float t_Time = uncompressTime(t_CompressedTranslation.first, m_Duration);

			std::bitset<48> t_Mask = 0xFFFF;
			uint16_t t_X = static_cast<uint16_t>((t_CompressedTranslation.second & t_Mask).to_ulong());
			uint16_t t_Y = static_cast<uint16_t>((t_CompressedTranslation.second >> 16 & t_Mask).to_ulong());
			uint16_t t_Z = static_cast<uint16_t>((t_CompressedTranslation.second >> 32 & t_Mask).to_ulong());

			glm::vec3 t_Translation = uncompressVector(t_TranslationMin, t_TranslationMax, t_X, t_Y, t_Z);

			t_BoneEntry.Translation.push_back(Bone::TranslationFrame(t_Time, t_Translation));
		}

		for (auto &t_CompressedRotation : t_CompressedRotations.at(i))
		{
			float t_Time = uncompressTime(t_CompressedRotation.first, m_Duration);

			std::bitset<48> t_Mask = 0x7FFF;
			uint16_t t_DominantAxis = static_cast<uint16_t>((t_CompressedRotation.second >> 45).to_ulong());
			uint16_t t_X = static_cast<uint16_t>((t_CompressedRotation.second >> 30 & t_Mask).to_ulong());
			uint16_t t_Y = static_cast<uint16_t>((t_CompressedRotation.second >> 15 & t_Mask).to_ulong());
			uint16_t t_Z = static_cast<uint16_t>((t_CompressedRotation.second & t_Mask).to_ulong());

			glm::quat t_Rotation = uncompressQuaternion(t_DominantAxis, t_X, t_Y, t_Z);

			t_BoneEntry.Rotation.push_back(Bone::RotationFrame(t_Time, t_Rotation));
		}

		for (auto &t_CompressedScale : t_CompressedScales.at(i))
		{
			float t_Time = uncompressTime(t_CompressedScale.first, m_Duration);

			std::bitset<48> t_Mask = 0xFFFF;
			uint16_t t_X = static_cast<uint16_t>((t_CompressedScale.second & t_Mask).to_ulong());
			uint16_t t_Y = static_cast<uint16_t>((t_CompressedScale.second >> 16 & t_Mask).to_ulong());
			uint16_t t_Z = static_cast<uint16_t>((t_CompressedScale.second >> 32 & t_Mask).to_ulong());

			glm::vec3 t_Scale = uncompressVector(t_ScaleMin, t_ScaleMax, t_X, t_Y, t_Z);
			t_BoneEntry.Scale.push_back(Bone::ScaleFrame(t_Time, t_Scale));
		}

		m_Bones.push_back(t_BoneEntry);
	}

	return File::LoadState::Loaded;
}
