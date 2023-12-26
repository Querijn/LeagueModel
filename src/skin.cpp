#include "skin.hpp"

namespace LeagueModel
{
	uint32_t FNV1Hash(const std::string& string)
	{
		uint32_t currentHash = 0x811c9dc5;
		const char* chars = string.c_str();
		for (size_t i = 0; i < string.length(); i++)
			currentHash = ((currentHash ^ tolower(chars[i])) * 0x01000193) % 0x100000000;

		return currentHash;
	}

	struct SubMeshHeader
	{
		std::string name = "";
		uint32_t vertexOffset;
		uint32_t VertexCount;
		uint32_t indexOffset;
		uint32_t indexCount;
	};


	Skin::Mesh::Mesh(const std::string& inName) : 
		name(inName),
		hash(FNV1Hash(inName))
	{
	}

	void Skin::Load(const std::string& inFilePath, OnLoadFunction inOnLoadFunction)
	{
		file = Spek::File::Load(inFilePath.c_str(), [this, inOnLoadFunction](Spek::File::Handle inFile, Spek::File::LoadState inLoadState)
		{
			if (inLoadState != Spek::File::LoadState::Loaded)
			{
				printf("Skin %s.\n", inLoadState == Spek::File::LoadState::FailedToLoad ? "failed to load" : "was not found");
				loadState = inLoadState;
				if (inOnLoadFunction) 
					inOnLoadFunction(*this);
				return;
			}

			uint32_t signature = 0;
			uint32_t subMeshHeaderCount;

			size_t offset = 0;

			inFile->Get(signature, offset);
			if (signature != 0x112233)
			{
				loadState = Spek::File::LoadState::FailedToLoad;
				printf("Skin has no valid signature, this is not a skn file!\n");
				if (inOnLoadFunction)
					inOnLoadFunction(*this);
				return;
			}

			inFile->Get(majorVersion, offset);
			inFile->Get(minorVersion, offset);
			if (majorVersion > 4)
			{
				loadState = Spek::File::LoadState::FailedToLoad;
				printf("Skin has got a skn file version that we don't support!\n");
				if (inOnLoadFunction) 
					inOnLoadFunction(*this);
				return;
			}

			inFile->Get(subMeshHeaderCount, offset);

			std::vector<SubMeshHeader> subMeshHeaders;
			if (majorVersion > 0)
			{
				for (uint32_t i = 0; i < subMeshHeaderCount; i++)
				{
					SubMeshHeader mesh;

					char name[64];
					inFile->Read((uint8_t*)name, 64, offset);
					mesh.name = name;

					inFile->Get(mesh.vertexOffset, offset);
					inFile->Get(mesh.VertexCount, offset);
					inFile->Get(mesh.indexOffset, offset);
					inFile->Get(mesh.indexCount, offset);

					subMeshHeaders.push_back(mesh);
				}

			}

			if (majorVersion == 4) offset += 4;

			uint32_t indexCount, vertexCount;
			inFile->Get(indexCount, offset);
			inFile->Get(vertexCount, offset);

			boundingBox.min = glm::vec3(6e6);
			boundingBox.max = glm::vec3(-6e6);

			uint32_t hasColour = 0;
			if (majorVersion == 4)
			{
				uint32_t vertexSize;
				inFile->Get(vertexSize, offset);
				inFile->Get(hasColour, offset);

				inFile->Get(boundingBox.min.x, offset);
				inFile->Get(boundingBox.min.y, offset);
				inFile->Get(boundingBox.min.z, offset);

				inFile->Get(boundingBox.max.x, offset);
				inFile->Get(boundingBox.max.y, offset);
				inFile->Get(boundingBox.max.z, offset);

				// Skip bounding sphere
				offset += sizeof(float) * 4;
			}

			indices.resize(indexCount);
			for (uint32_t i = 0; i < indexCount; i++)
				inFile->Get(indices[i], offset);

			vertices.resize(vertexCount);
			for (uint32_t i = 0; i < vertexCount; i++)
			{
				Vertex& vertex = vertices[i];
				inFile->Get(vertex.position.x, offset);
				inFile->Get(vertex.position.y, offset);
				inFile->Get(vertex.position.z, offset);

				for (int j = 0; j < 4; j++)
				{
					uint8_t bone;
					inFile->Get(bone, offset);

					vertex.boneIndices[j] = bone;
				}

				inFile->Get(vertex.weights.x, offset);
				inFile->Get(vertex.weights.y, offset);
				inFile->Get(vertex.weights.z, offset);
				inFile->Get(vertex.weights.w, offset);

				inFile->Get(vertex.normal.x, offset);
				inFile->Get(vertex.normal.y, offset);
				inFile->Get(vertex.normal.z, offset);

				inFile->Get(vertex.uv.x, offset);
				inFile->Get(vertex.uv.y, offset);

				if (hasColour)
					offset += sizeof(uint8_t) * 4;

				auto weightError = fabsf(vertex.weights.x + vertex.weights.y + vertex.weights.z + vertex.weights.w - 1.0f);
				if (weightError > 0.02f)
				{
					loadState = Spek::File::LoadState::FailedToLoad;
					if (inOnLoadFunction)
						inOnLoadFunction(*this);
					return;
				}
			}

			for (auto& meshHeader : subMeshHeaders)
			{
				Mesh mesh(meshHeader.name);
				mesh.indexCount = meshHeader.indexCount;
				mesh.vertexCount = meshHeader.VertexCount;

				mesh.vertices = &vertices[meshHeader.vertexOffset];
				mesh.indices = &indices[meshHeader.indexOffset];

				meshes.push_back(mesh);
			}

			printf("Skin version %u was succesfully loaded: %u vertices.\n", majorVersion, vertexCount);
			loadState = Spek::File::LoadState::Loaded;
			if (inOnLoadFunction)
				inOnLoadFunction(*this);
		});
	}
}
