#include "league_model/league_skin.hpp"
#include "file_system.hpp"

#include <fstream>
#include <string>

#pragma pack(push, 1)
struct Box
{
	glm::vec3 Min;
	glm::vec3 Max;
};

struct Sphere
{
	float Center[3];
	float Radius;
};

struct Header
{
	int32_t Signature;
	uint16_t Major;
	uint16_t Minor;
	uint32_t MaterialHeaderCount;
};

struct MaterialHeader
{
	char Material[64];
	uint32_t VertexOffset;
	uint32_t VertexCount;
	uint32_t IndexOffset;
	uint32_t IndexCount;
};

struct InfoHeader
{
	uint32_t IndexCount;
	uint32_t VertexCount;
};

struct InfoHeaderV4
{
	uint32_t Unknown;
	InfoHeader Base;
	uint32_t VertexSize;
	uint32_t HasTangents;
	Box BoundingBox;
	Sphere BoundingSphere;
};

struct Vertex
{
	glm::vec3 Position;
	int8_t BoneIndices[4];
	glm::vec4 Weights;
	glm::vec3 Normal;
	glm::vec2 UV;
};

struct VertexV4
{
	Vertex Base;
	uint8_t Tangent[4];
};
#pragma pack(pop)

void League::Skin::Load(const std::string & a_Path, OnLoadFunction a_OnLoad)
{
	FileSystem t_FileSystem;
	t_FileSystem.OpenFile(a_Path, [&](BaseFile* a_File, BaseFile::LoadState a_LoadState)
	{
		if (a_LoadState != BaseFile::LoadState::Loaded)
		{
			if (a_OnLoad) a_OnLoad(nullptr, a_LoadState);
			return;
		}

		File* t_File = (File*)a_File;

		Header t_Header;
		t_File->Read((uint8_t*)&t_Header, sizeof(t_Header));

		if (t_Header.Major > 4) 
		{
			if (a_OnLoad) a_OnLoad(nullptr, BaseFile::LoadState::FailedToLoad);
			return;
		}

		std::vector<MaterialHeader> t_MaterialHeaders;
		if (t_Header.Major > 0) t_File->Get(t_MaterialHeaders, t_Header.MaterialHeaderCount);
		if (t_Header.Major == 4) t_File->Seek(4, BaseFile::SeekType::FromCurrent);

		uint32_t numIndices, numVertices;
		t_File->Read(reinterpret_cast<uint8_t*>(&numIndices), 4);
		t_File->Read(reinterpret_cast<uint8_t*>(&numVertices), 4);

		BoundingMin = glm::vec3(4e4);
		BoundingMax = glm::vec3(-4e4);

		uint32_t t_HasTangents = 0;
		if (t_Header.Major == 4)
		{
			uint32_t t_VertexSize;			
			t_File->Get(t_VertexSize);
			t_File->Get(t_HasTangents);
			t_File->Get(BoundingMin);
			t_File->Get(BoundingMax);
			t_File->Seek(sizeof(Sphere), BaseFile::SeekType::FromCurrent);
		}

		t_File->Get(Indices, numIndices);

		std::vector<VertexV4> t_Vertices;
		if (t_HasTangents) 
			t_File->Get(t_Vertices, numVertices);
		else
		{
			t_Vertices.resize(numVertices);
			for (int i = 0; i < numVertices; ++i)
				t_File->Get(t_Vertices[i].Base);
		}
		
		for (auto& t_Vertex : t_Vertices)
		{
			// Calculate bounding box if not present in file
			if (t_Header.Major != 4)
			{
				if (BoundingMin.x > t_Vertex.Base.Position.x) BoundingMin.x = t_Vertex.Base.Position.x;
				if (BoundingMin.y > t_Vertex.Base.Position.y) BoundingMin.y = t_Vertex.Base.Position.y;
				if (BoundingMin.z > t_Vertex.Base.Position.z) BoundingMin.z = t_Vertex.Base.Position.z;

				if (BoundingMax.x < t_Vertex.Base.Position.x) BoundingMax.x = t_Vertex.Base.Position.x;
				if (BoundingMax.y < t_Vertex.Base.Position.y) BoundingMax.y = t_Vertex.Base.Position.y;
				if (BoundingMax.z < t_Vertex.Base.Position.z) BoundingMax.z = t_Vertex.Base.Position.z;
			}

			Positions.push_back(t_Vertex.Base.Position);
			UVs.push_back(t_Vertex.Base.UV);
			Normals.push_back(t_Vertex.Base.Normal);
			Weights.push_back(t_Vertex.Base.Weights);
			BoneIndices.push_back({ t_Vertex.Base.BoneIndices[0], t_Vertex.Base.BoneIndices[1], t_Vertex.Base.BoneIndices[2], t_Vertex.Base.BoneIndices[3] });
		}
		
		if (a_OnLoad) a_OnLoad(this, BaseFile::LoadState::Loaded);
	});
}

League::Skin::Skin()
{
}
