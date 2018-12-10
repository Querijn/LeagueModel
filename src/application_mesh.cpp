#include <application_mesh.hpp>
#include <application.hpp>

#include <league/animation.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

template<typename T>
T Interpolate(T a_Low, T a_High, float a_Progress)
{
	return glm::lerp(a_Low, a_High, a_Progress);
}

// glm::lerp between 2 quaternions are not doing what I want them to do, so I have to overwrite it for FindNearestTime.
template<>
glm::quat Interpolate(glm::quat a_Low, glm::quat a_High, float a_Progress)
{
	glm::quat t_Return = glm::quat(0, 0, 0, 1);
	float t_Dot = a_Low.w * a_High.w + a_Low.x * a_High.x + a_Low.y * a_High.y + a_Low.z * a_High.z;
	float t_InvertedBlend = 1.0f - a_Progress;

	if (t_Dot < 0) 
	{
		t_Return.w = t_InvertedBlend * a_Low.w + a_Progress * -a_High.w;
		t_Return.x = t_InvertedBlend * a_Low.x + a_Progress * -a_High.x;
		t_Return.y = t_InvertedBlend * a_Low.y + a_Progress * -a_High.y;
		t_Return.z = t_InvertedBlend * a_Low.z + a_Progress * -a_High.z;
	}
	else 
	{
		t_Return.w = t_InvertedBlend * a_Low.w + a_Progress * a_High.w;
		t_Return.x = t_InvertedBlend * a_Low.x + a_Progress * a_High.x;
		t_Return.y = t_InvertedBlend * a_Low.y + a_Progress * a_High.y;
		t_Return.z = t_InvertedBlend * a_Low.z + a_Progress * a_High.z;
	}

	return glm::normalize(t_Return);
}

template<typename T>
T FindNearestTime(const std::vector<League::Animation::Bone::Frame<T>>& a_Vector, float a_Time)
{
	auto t_Min = a_Vector[0];
	auto t_Max = a_Vector[a_Vector.size() - 1];

	for (size_t i = 0; i < a_Vector.size(); i++)
	{
		const auto& t_Current = a_Vector[i];

		if (t_Current.Time <= a_Time)
		{
			t_Min = t_Current;
			continue;
		}

		if (t_Current.Time > a_Time)
		{
			t_Max = t_Current;
			break;
		}

		printf("Yikes! FindNearestTime could not find a time compatible with this animation!\n");
		throw 0; // Should not happen
	}

	float t_Div = t_Max.Time - t_Min.Time;
	float t_LerpValue = (t_Div == 0) ? 1 : (a_Time - t_Min.Time) / t_Div;
	return Interpolate(t_Min.FrameData, t_Max.FrameData, t_LerpValue);
}

void ApplicationMesh::SetupHierarchy(const glm::mat4& a_InverseRoot, std::vector<glm::mat4>& a_Bones, const League::Skeleton::Bone& a_SkeletonBone, const glm::mat4& a_Parent, float a_Time)
{
	glm::mat4 t_GlobalTransform = a_Parent;

	const auto* t_AnimBone = Animations[CurrentAnimation]->GetBone(a_SkeletonBone.Name);
	if (t_AnimBone != nullptr)
	{
		glm::vec3 t_Translation = FindNearestTime(t_AnimBone->Translation, a_Time);
		glm::quat t_Rotation = FindNearestTime(t_AnimBone->Rotation, a_Time);
		glm::vec3 t_Scale = FindNearestTime(t_AnimBone->Scale, a_Time);

		auto t_LocalTransform = glm::translate(t_Translation) * glm::mat4_cast(t_Rotation) * glm::scale(t_Scale);
		t_GlobalTransform = a_Parent * t_LocalTransform;
	}
	//else printf("Animation bone %s is not found\n", a_SkeletonBone.Name.c_str());

	a_Bones[a_SkeletonBone.ID] = t_GlobalTransform * a_SkeletonBone.InverseGlobalMatrix;

	for (auto& t_Child : a_SkeletonBone.Children)
		SetupHierarchy(a_InverseRoot, a_Bones, *t_Child, t_GlobalTransform, a_Time);
};

void ApplicationMesh::Draw(size_t a_SubMeshIndex, float a_Time, ShaderProgram& a_Program, glm::mat4& a_VP, Texture* a_Diffuse, std::vector<glm::mat4>* a_BoneTransforms)
{
	a_Program.Use();

	if (PositionBuffer) PositionBuffer->Use();
	if (UVBuffer) UVBuffer->Use();
	if (NormalBuffer) NormalBuffer->Use();
	if (BoneIndexBuffer) BoneIndexBuffer->Use();
	if (BoneWeightBuffer) BoneWeightBuffer->Use();

	auto& t_Submesh = SubMeshes[a_SubMeshIndex];

	if (a_Diffuse) *a_Diffuse = t_Submesh.HasImage ? t_Submesh.Image : Application::Instance->GetDefaultTexture();

	glm::vec4 t_Center(0);

	if (Skeleton && a_BoneTransforms)
	{
		const auto& t_AnimationIndex = Animations.find(CurrentAnimation);
		if (t_AnimationIndex != Animations.end())
		{
			float t_AnimationDuration = t_AnimationIndex->second->GetDuration();
			while (a_Time > t_AnimationDuration) a_Time -= t_AnimationDuration;

			auto& t_Bones = Skeleton->GetBones();
			a_BoneTransforms->resize(t_Bones.size());
			SetupAnimation(*a_BoneTransforms, a_Time);
		}
		else
		{
			auto& t_Bones = Skeleton->GetBones();
			a_BoneTransforms->resize(t_Bones.size());
			for (int i = 0; i < a_BoneTransforms->size(); i++)
				a_BoneTransforms->at(i) = glm::identity<glm::mat4>();
		}
	}

	a_VP *= t_Submesh.GetTransformMatrix();

	a_Program.Update();
	t_Submesh.IndexBuffer->Draw();
}

void ApplicationMesh::SetupAnimation(std::vector<glm::mat4>& a_BoneTransforms, float a_Time)
{
	glm::mat4 t_InverseRoot = glm::identity<glm::mat4>();
	const auto& t_Bones = Animations[CurrentAnimation]->GetBones();

	for (size_t i = 0; i < t_Bones.size(); i++)
	{
		const auto& t_AnimBone = t_Bones[i];
		const auto* t_SkellyBone = Skeleton->GetBone(t_AnimBone.Name);

		if (t_SkellyBone->Parent != nullptr) continue;

		SetupHierarchy(t_InverseRoot, a_BoneTransforms, *t_SkellyBone, glm::identity<glm::mat4>(), a_Time);
	}
}

glm::mat4 ApplicationMesh::SubMesh::GetTransformMatrix() const
{
	return glm::translate(Position) * glm::mat4_cast(Rotation) * glm::scale(Scale);
}

void ApplicationMesh::SubMesh::SetTexture(std::string a_FilePath)
{
	printf("Attempting to load texture %s\n", a_FilePath.c_str());
	Image.Load(a_FilePath, [](Texture& a_Texture, void* a_UserData)
	{
		auto& t_SubMesh = *(SubMesh*)a_UserData;
		if (a_Texture.GetLoadState() != File::LoadState::Loaded)
		{
			printf("Texture %s.\n", a_Texture.GetLoadState() != File::LoadState::FailedToLoad ? "failed to load" : "was not found");
			return;
		}

		t_SubMesh.HasImage = true;
		t_SubMesh.Image = a_Texture;
		printf("Texture loaded!\n");
	}, this);
}

void ApplicationMesh::AddAnimationReference(const std::string& a_Name, const League::Animation & a_Animation)
{
	Animations[a_Name] = &a_Animation;
}

void ApplicationMesh::ApplyAnimation(const std::string& a_Animation)
{
	const auto& t_AnimationIndex = Animations.find(a_Animation);
	if (t_AnimationIndex == Animations.end()) return;

	CurrentAnimation = a_Animation;
	const auto& t_Animation = Animations[CurrentAnimation];
	const auto& t_Bones = t_Animation->GetBones();

	float t_HighestY(-9e9), t_LowestY(9e9);
	size_t t_Count = 0;
	for (auto& t_Bone : t_Bones)
	{
		for (auto& t_Translation : t_Bone.Translation)
		{
			if (t_Translation.FrameData.y > t_HighestY)
				t_HighestY = t_Translation.FrameData.y;
			else if (t_Translation.FrameData.y < t_LowestY)
				t_LowestY = t_Translation.FrameData.y;

			t_Count++;
		}
	}

	if (abs(t_HighestY) + abs(t_LowestY) < 400)
		Center.y = -(t_HighestY + t_LowestY) * 0.5f;
	else Center.y = -75;
}
