#include "skin.hpp"
#include <string>

struct SubMeshHeader
{
	std::string Material = "";
	uint32_t VertexOffset;
	uint32_t VertexCount;
	uint32_t IndexOffset;
	uint32_t IndexCount;
};

void League::Skin::Load(std::string a_FilePath, OnLoadFunction a_OnLoadFunction, void * a_Argument)
{
	auto* t_File = FileSystem::GetFile(a_FilePath);

	struct LoadData
	{
		LoadData(Skin* a_Target, OnLoadFunction a_Function, void* a_Argument) :
			Target(a_Target), OnLoadFunction(a_Function), Argument(a_Argument)
		{}

		Skin* Target;
		OnLoadFunction OnLoadFunction;
		void* Argument;
	};
	auto* t_LoadData = new LoadData(this, a_OnLoadFunction, a_Argument);

	t_File->Load([](File* a_File, File::LoadState a_LoadState, void* a_Argument)
	{
		auto* t_LoadData = (LoadData*)a_Argument;
		auto* t_Skin = (Skin*)t_LoadData->Target;

		if (a_LoadState != File::LoadState::Loaded)
		{
			t_Skin->m_State = a_LoadState;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Skin, t_LoadData->Argument);

			FileSystem::CloseFile(*a_File);
			delete t_LoadData;
			return;
		}

		uint32_t t_Signature = 0;
		uint32_t t_SubMeshHeaderCount;

		size_t t_Offset = 0;

		a_File->Get(t_Signature, t_Offset);
		if (t_Signature != 0x112233)
		{
			t_Skin->m_State = File::LoadState::FailedToLoad;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Skin, t_LoadData->Argument);

			FileSystem::CloseFile(*a_File);
			delete t_LoadData;
			return;
		}

		a_File->Get(t_Skin->m_Major, t_Offset);
		a_File->Get(t_Skin->m_Minor, t_Offset);
		if (t_Skin->m_Major > 4)
		{
			t_Skin->m_State = File::LoadState::FailedToLoad;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Skin, t_LoadData->Argument);

			FileSystem::CloseFile(*a_File);
			delete t_LoadData;
			return;
		}

		a_File->Get(t_SubMeshHeaderCount, t_Offset);
		
		std::vector<SubMeshHeader> t_SubMeshHeaders;
		if (t_Skin->m_Major > 0)
		{
			for (uint32_t i = 0; i < t_SubMeshHeaderCount; i++)
			{
				SubMeshHeader t_Mesh;

				char t_MaterialName[64];
				a_File->Read((uint8_t*)t_MaterialName, 64, t_Offset);
				t_Mesh.Material = t_MaterialName;

				a_File->Get(t_Mesh.VertexOffset, t_Offset);
				a_File->Get(t_Mesh.VertexCount, t_Offset);
				a_File->Get(t_Mesh.IndexOffset, t_Offset);
				a_File->Get(t_Mesh.IndexCount, t_Offset);

				t_SubMeshHeaders.push_back(t_Mesh);
			}
			
		}

		if (t_Skin->m_Major == 4) t_Offset += 4;

		uint32_t t_IndexCount, t_VertexCount;
		a_File->Get(t_IndexCount, t_Offset);
		a_File->Get(t_VertexCount, t_Offset);

		t_Skin->m_BoundingBox.Min = glm::vec3(6e6);
		t_Skin->m_BoundingBox.Max = glm::vec3(-6e6);

		uint32_t t_HasTangents = 0;
		if (t_Skin->m_Major == 4)
		{
			uint32_t t_VertexSize;
			a_File->Get(t_VertexSize, t_Offset);
			a_File->Get(t_HasTangents, t_Offset);

			a_File->Get(t_Skin->m_BoundingBox.Min.x, t_Offset);
			a_File->Get(t_Skin->m_BoundingBox.Min.y, t_Offset);
			a_File->Get(t_Skin->m_BoundingBox.Min.z, t_Offset);

			a_File->Get(t_Skin->m_BoundingBox.Max.x, t_Offset);
			a_File->Get(t_Skin->m_BoundingBox.Max.y, t_Offset);
			a_File->Get(t_Skin->m_BoundingBox.Max.z, t_Offset);

			// Skip bounding sphere
			t_Offset += sizeof(float) * 4;
		}

		t_Skin->m_Indices.resize(t_IndexCount);
		for (uint32_t i = 0; i < t_IndexCount; i++)
			a_File->Get(t_Skin->m_Indices[i], t_Offset);

		t_Skin->m_Positions.resize(t_VertexCount);
		t_Skin->m_UVs.resize(t_VertexCount);
		t_Skin->m_Normals.resize(t_VertexCount);
		t_Skin->m_Tangents.resize(t_VertexCount);
		t_Skin->m_Weights.resize(t_VertexCount);
		t_Skin->m_BoneIndices.resize(t_VertexCount);
		for (uint32_t i = 0; i < t_VertexCount; i++)
		{
			a_File->Get(t_Skin->m_Positions[i].x, t_Offset);
			a_File->Get(t_Skin->m_Positions[i].y, t_Offset);
			a_File->Get(t_Skin->m_Positions[i].z, t_Offset);

			for (int j = 0; j < 4; j++)
			{
				uint8_t t_Bone;
				a_File->Get(t_Bone, t_Offset);

				t_Skin->m_BoneIndices[i][j] = t_Bone;
			}

			a_File->Get(t_Skin->m_Weights[i].x, t_Offset);
			a_File->Get(t_Skin->m_Weights[i].y, t_Offset);
			a_File->Get(t_Skin->m_Weights[i].z, t_Offset);
			a_File->Get(t_Skin->m_Weights[i].w, t_Offset);

			a_File->Get(t_Skin->m_Normals[i].x, t_Offset);
			a_File->Get(t_Skin->m_Normals[i].y, t_Offset);
			a_File->Get(t_Skin->m_Normals[i].z, t_Offset);

			a_File->Get(t_Skin->m_UVs[i].x, t_Offset);
			a_File->Get(t_Skin->m_UVs[i].y, t_Offset);

			// Skip tangents
			if (t_HasTangents) t_Offset += sizeof(uint8_t) * 4;

			auto t_WeightError = fabsf(t_Skin->m_Weights[i].x + t_Skin->m_Weights[i].y + t_Skin->m_Weights[i].z + t_Skin->m_Weights[i].w - 1.0f);
			if (t_WeightError > 0.02f)
			{
				t_Skin->m_State = File::LoadState::FailedToLoad;
				if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Skin, t_LoadData->Argument);

				FileSystem::CloseFile(*a_File);
				delete t_LoadData;
				return;
			}
		}

		for (auto& t_MeshHeader : t_SubMeshHeaders)
		{
			Mesh t_Mesh(t_MeshHeader.Material);
			t_Mesh.VertexCount = t_MeshHeader.VertexCount;
			t_Mesh.IndexCount = t_MeshHeader.IndexCount;

			t_Mesh.Positions = &t_Skin->m_Positions[t_MeshHeader.VertexOffset];
			t_Mesh.UVs = &t_Skin->m_UVs[t_MeshHeader.VertexOffset];
			t_Mesh.Normals = &t_Skin->m_Normals[t_MeshHeader.VertexOffset];
			t_Mesh.Tangents = &t_Skin->m_Tangents[t_MeshHeader.VertexOffset];
			t_Mesh.Weights = &t_Skin->m_Weights[t_MeshHeader.VertexOffset];
			t_Mesh.BoneIndices = &t_Skin->m_BoneIndices[t_MeshHeader.VertexOffset];
			t_Mesh.Indices = &t_Skin->m_Indices[t_MeshHeader.IndexOffset];

			t_Skin->m_Meshes.push_back(t_Mesh);
		}

		t_Skin->m_State = File::LoadState::Loaded;
		if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(*t_Skin, t_LoadData->Argument);

		FileSystem::CloseFile(*a_File);
		delete t_LoadData;
	}, t_LoadData);
}

const League::Skin::BoundingBox& League::Skin::GetBoundingBox() const
{
	return m_BoundingBox;
}

const std::vector<League::Skin::Mesh>& League::Skin::GetMeshes() const
{
	return m_Meshes;
}

const std::vector<glm::vec3>& League::Skin::GetPositions() const
{
	return m_Positions;
}

const std::vector<glm::vec2>& League::Skin::GetUVs() const
{
	return m_UVs;
}

const std::vector<glm::vec3>& League::Skin::GetNormals() const
{
	return m_Normals;
}

const std::vector<glm::vec4>& League::Skin::GetTangents() const
{
	return m_Tangents;
}

const std::vector<glm::vec4>& League::Skin::GetWeights() const
{
	return m_Weights;
}

const std::vector<glm::vec4>& League::Skin::GetBoneIndices() const
{
	return m_BoneIndices;
}

const std::vector<uint16_t>& League::Skin::GetIndices() const
{
	return m_Indices;
}
