#include "league_model/league_skeleton.hpp"
#include "league_model/league_skin.hpp"

#include <fstream>
#include <algorithm>
#include <map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

#pragma pack(push, 1)
enum SkeletonType : uint32_t
{
	Classic = 0x746C6B73,
	Version2 = 0x22FD4FC3
};

struct Header
{
	uint32_t Signature;
	SkeletonType Version;
};

struct ClassicSkeletonHeader
{
	uint32_t Version;
	uint32_t Unknown2;
	uint32_t BoneCount;
};

namespace Version2Skeleton
{
	struct Bone
	{
		uint16_t Unknown1;
		int16_t ID;
		int16_t ParentID;
		uint16_t Unknown2;
		uint32_t Hash;
		uint32_t Unknown3;
		glm::vec3 Position;
		glm::vec3 Scale;
		glm::quat Rotation;
		char Unknown4[44];
	};

	struct Header
	{
		uint32_t Version;
		uint16_t Unknown2;
		uint16_t BoneCount;
		uint32_t BoneIndexCount;
		uint16_t DataOffset;
		uint16_t Unknown3;
		uint32_t BoneIndexMapOffset;
		uint32_t BoneIndicesOffset;
		uint32_t Unknown5;
		uint32_t Unknown6;
		uint32_t BoneNamesOffset;
	};
};
#pragma pack(pop)

League::Skeleton::Skeleton(League::Skin & a_Skin) :
	m_Skin(a_Skin)
{
}

void League::Skeleton::Load(const std::string & a_FilePath, OnLoadFunction a_OnLoadFunction)
{
	FileSystem t_FileSystem;
	t_FileSystem.OpenFile(a_FilePath, [&](BaseFile* a_File, BaseFile::LoadState a_LoadState)
	{
		if (a_LoadState != BaseFile::LoadState::Loaded)
		{
			if (a_OnLoadFunction) a_OnLoadFunction(nullptr, a_LoadState);
			return;
		}
		
		uint32_t t_Signature;
		SkeletonType t_SkeletonType;
		a_File->Get(t_Signature);
		a_File->Get(t_SkeletonType);

		BaseFile::LoadState t_State;
		switch (t_SkeletonType)
		{
			case SkeletonType::Classic:
				t_State = ReadClassic(*a_File);

			case SkeletonType::Version2:
				t_State = ReadVersion2(*a_File);
		};

		if (a_OnLoadFunction) a_OnLoadFunction(t_State == BaseFile::LoadState::Loaded ? this : nullptr, t_State);
	});
}

BaseFile::LoadState League::Skeleton::ReadClassic(BaseFile& a_File)
{
	printf("We haven't tested classic skeletons yet!\n");
	throw 0; // Not yet tested

	ClassicSkeletonHeader t_Header;
	a_File.Get(t_Header);

	Bones.resize(t_Header.BoneCount);

	for (int i = 0; i < t_Header.BoneCount; ++i)
	{
		memset(Bones[i].name, 0, 64);
		a_File.Read(reinterpret_cast<uint8_t*>(&Bones[i].name), 32);
		a_File.Read(reinterpret_cast<uint8_t*>(&Bones[i].parent), 4);
		a_File.Read(reinterpret_cast<uint8_t*>(&Bones[i].scale.x), 4);
		a_File.Read(reinterpret_cast<uint8_t*>(&Bones[i].globalMatrix), 48);

		Bones[i].scale.x *= 10.0f;
		Bones[i].scale.y = Bones[i].scale.z = Bones[i].scale.x;

		Bones[i].globalMatrix[3][0] = Bones[i].globalMatrix[3][1] = Bones[i].globalMatrix[3][2] = 0.0f;
		Bones[i].globalMatrix[3][3] = 1.0f;

		Bones[i].globalMatrix = glm::transpose(Bones[i].globalMatrix);
	}

	for (auto& i : Bones)
	{
		glm::mat4 relativeMatrix, inverseMatrix;

		if (i.parent != -1)
		{
			inverseMatrix = glm::inverse(Bones[i.parent].globalMatrix);
			i.localMatrix = inverseMatrix * i.globalMatrix;
		}

		else
		{
			i.localMatrix = i.globalMatrix;
		}
	}

	if (m_Skin.Major == 0 || m_Skin.Major == 1)
	{
		BoneIndices.resize(t_Header.BoneCount);

		for (size_t i = 0; i < BoneIndices.size(); i++)
			BoneIndices[i] = i;
	}

	else if (m_Skin.Major == 2)
	{
		uint32_t t_BoneIndexCount;
		a_File.Get(t_BoneIndexCount);
		a_File.Get(BoneIndices, t_BoneIndexCount);
	}

	return BaseFile::LoadState::Loaded;
}

void RecursiveFixGlobalMatrix(const glm::mat4& a_Parent, League::Skeleton::Bone& a_Bone)
{
	auto t_Global = a_Parent * a_Bone.localMatrix;
	a_Bone.globalMatrix = glm::inverse(t_Global);

	for (auto t_Child : a_Bone.Children)
		RecursiveFixGlobalMatrix(t_Global, *t_Child);
}

BaseFile::LoadState League::Skeleton::ReadVersion2(BaseFile & a_File)
{
	Version2Skeleton::Header t_Header;
	a_File.Get(t_Header);

	std::vector<Version2Skeleton::Bone> t_Bones;
	a_File.Seek(t_Header.DataOffset, BaseFile::SeekType::FromBeginning);
	a_File.Get(t_Bones, t_Header.BoneCount);
	Bones.resize(t_Header.BoneCount);

	for (int i = 0; i < t_Header.BoneCount; ++i)
	{
		Bones[i].hash = t_Bones[i].Hash;
		Bones[i].id = t_Bones[i].ID;
		Bones[i].parent = t_Bones[i].ParentID;

		Bones[i].localMatrix = glm::translate(t_Bones[i].Position) * glm::mat4_cast(t_Bones[i].Rotation);
	}

	a_File.Seek(t_Header.BoneIndexMapOffset, BaseFile::SeekType::FromBeginning);

	for (int i = 0; i < t_Header.BoneCount; ++i) // Inds for version 4 animation.
	{
		// 8 bytes
		uint32_t sklID;
		a_File.Get(sklID);
		uint32_t anmID;
		a_File.Get(anmID);

		BoneIndexMap[anmID] = sklID;
	}

	a_File.Seek(t_Header.BoneIndicesOffset, BaseFile::SeekType::FromBeginning);
	a_File.Get(BoneIndices, t_Header.BoneIndexCount);

	a_File.Seek(t_Header.BoneIndicesOffset, BaseFile::SeekType::FromBeginning);
	a_File.Get(BoneIndices, t_Header.BoneIndexCount);
	
	a_File.Seek(t_Header.BoneNamesOffset, BaseFile::SeekType::FromBeginning);

	// Get file part with bone names
	size_t t_NameChunkSize = 32 * t_Header.BoneCount;
	std::vector<uint8_t> t_Start(t_NameChunkSize);
	memset(t_Start.data(), 0, t_NameChunkSize);
	a_File.Read(t_Start.data(), t_NameChunkSize);

	char* t_Pointer = (char*)t_Start.data();
	for (int i = 0; i < t_Header.BoneCount; ++i)
	{
		strcpy(Bones[i].name, t_Pointer);
		size_t t_NameLength = strlen(t_Pointer);
		std::string t_Name = t_Pointer;
		t_Pointer += t_NameLength;
		while (*t_Pointer == 0) t_Pointer++; // eat all \0s

		if (Bones[i].parent != -1) Bones[Bones[i].parent].Children.push_back(&Bones[i]);
	}

	for (auto& t_Bone : Bones)
	{
		if (t_Bone.parent != -1) continue;
		RecursiveFixGlobalMatrix(glm::identity<glm::mat4>(), t_Bone);
	}

	return BaseFile::LoadState::Loaded;
}
