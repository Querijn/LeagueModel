#pragma once

#include "types.hpp"

#include <glm/glm.hpp>
#include <spek/file/file.hpp>

namespace LeagueModel
{
	struct Skeleton;
	struct Skin
	{
		struct Vertex
		{
			glm::vec3 position;
			glm::vec2 uv;
			glm::vec3 normal;
			glm::vec4 boneIndices;
			glm::vec4 weights;
		};

		struct Mesh
		{
			Mesh(const std::string& inName);

			std::string name;
			uint32_t hash;

			Vertex* vertices = nullptr;
			size_t vertexCount = 0;

			u16* indices = nullptr;
			size_t indexCount = 0;

			bool initialVisibility = false;
		};

		struct BoundingBox
		{
			glm::vec3 min;
			glm::vec3 max;
		};

		using OnLoadFunction = std::function<void(Skin& skin)>;
		void Load(const std::string& inFilePath, OnLoadFunction inOnLoadFunction = nullptr);

		Spek::File::LoadState loadState = Spek::File::LoadState::NotLoaded;

		u16 majorVersion;
		u16 minorVersion;

		BoundingBox boundingBox;
		std::vector<Vertex> vertices;
		std::vector<u16> indices;

		std::vector<Mesh> meshes;

	private:
		Spek::File::Handle file;
	};
}