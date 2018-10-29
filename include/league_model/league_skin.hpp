#pragma once

#include <league_model/math.hpp>

#include <file/base_file.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <functional>

namespace League
{
	class Skin
	{
	public:
		using OnLoadFunction = std::function<void(League::Skin*, BaseFile::LoadState)>;

		Skin();

		void Load(const std::string& a_Path, OnLoadFunction a_OnLoad = nullptr);

		struct Mesh
		{
			std::string MaterialName;
			size_t VertexCount;

			glm::vec3* Positions;
			glm::vec2* UVs;
			glm::vec3* Normals;
			glm::vec4* Tangents;
			glm::vec4* Weights;
			uint8_t* BoneIndices;

			uint16_t* Indices;
			size_t IndexCount;
		};

		uint16_t Major, Minor;

		glm::vec3 BoundingMin, BoundingMax;

		std::vector<glm::vec3> Positions;
		std::vector<glm::vec2> UVs;
		std::vector<glm::vec3> Normals;
		std::vector<glm::vec4> Tangents;
		std::vector<glm::vec4> Weights;
		std::vector<glm::vec4> BoneIndices;
		std::vector<uint16_t> Indices;

		std::vector<Mesh> Meshes;
	};
}
