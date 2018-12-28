#pragma once

#include <league_model/file/file_system.hpp>
#include <string>

#include <glm/glm.hpp>

namespace League 
{
	class Skeleton;

	class Skin
	{
	public:
		using OnLoadFunction = void(*)(League::Skin& a_Skin, void* a_Argument);
		
		void Load(const std::string& a_FilePath, OnLoadFunction a_OnLoadFunction = nullptr, void* a_Argument = nullptr);

		struct Mesh
		{
			Mesh(const std::string& a_Name);

			std::string Name;
			uint32_t Hash;
			size_t VertexCount;

			glm::vec3* Positions;
			glm::vec2* UVs;
			glm::vec3* Normals;
			glm::vec4* Tangents;
			glm::vec4* Weights;
			glm::vec4* BoneIndices;

			uint16_t* Indices;
			size_t IndexCount;
		};

		struct BoundingBox
		{
			glm::vec3 Min;
			glm::vec3 Max;
		};

		const BoundingBox& GetBoundingBox() const;
		const std::vector<Mesh>& GetMeshes() const;

		const std::vector<glm::vec3>& GetPositions() const;
		const std::vector<glm::vec2>& GetUVs() const;
		const std::vector<glm::vec3>& GetNormals() const;
		const std::vector<glm::vec4>& GetTangents() const;
		const std::vector<glm::vec4>& GetWeights() const;
		const std::vector<glm::vec4>& GetBoneIndices() const;
		const std::vector<uint16_t>& GetIndices() const;

		File::LoadState GetLoadState() const { return m_State; }

		// Allow the skeleton to modify the bones
		friend class League::Skeleton;
	protected:
		std::vector<glm::vec4> m_BoneIndices;

	private:
		File::LoadState m_State = File::LoadState::NotLoaded;

		uint16_t m_Major;
		uint16_t m_Minor;

		BoundingBox m_BoundingBox;

		std::vector<glm::vec3> m_Positions;
		std::vector<glm::vec2> m_UVs;
		std::vector<glm::vec3> m_Normals;
		std::vector<glm::vec4> m_Tangents;
		std::vector<glm::vec4> m_Weights;
		std::vector<uint16_t> m_Indices;

		std::vector<Mesh> m_Meshes;
	};
}