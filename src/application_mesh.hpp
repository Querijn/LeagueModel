#pragma once

#include <renderer.hpp>
#include <league/skeleton.hpp>

namespace League { class Animation; }

class ApplicationMesh
{
public:
	void ApplyAnimation(const League::Animation& a_Animation);
	void Draw(size_t a_SubMeshIndex, float a_Time, ShaderProgram& a_Program, glm::mat4& a_MVP, Texture* a_Diffuse, std::vector<glm::mat4>* a_BoneTransforms);

	std::shared_ptr<League::Skeleton> Skeleton = nullptr;
	const League::Animation* Animation = nullptr;

	VertexBuffer<glm::vec3>* PositionBuffer;
	VertexBuffer<glm::vec2>* UVBuffer;
	VertexBuffer<glm::vec3>* NormalBuffer;
	VertexBuffer<glm::vec4>* BoneIndexBuffer;
	VertexBuffer<glm::vec4>* BoneWeightBuffer;

	struct SubMesh
	{
		glm::mat4 GetTransformMatrix() const;
		void SetTexture(String a_FilePath);

		IndexBuffer<uint16_t>* IndexBuffer;

		bool HasImage = false;
		Texture Image;

		glm::vec3 Position = glm::vec3(0.0);
		glm::quat Rotation = glm::quat();
		glm::vec3 Scale = glm::vec3(1.0);
	};

	std::vector<SubMesh> SubMeshes;

private:
	void SetupAnimation(std::vector<glm::mat4>& a_BoneTransforms, float a_Time);
	void SetupHierarchy(const glm::mat4& a_InverseRoot, std::vector<glm::mat4>& a_Bones, const League::Skeleton::Bone& a_SkeletonBone, const glm::mat4& a_Parent, float a_Time);
};