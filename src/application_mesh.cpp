#include <application_mesh.hpp>
#include <application.hpp>

#include <league_model/animation.hpp>
#include <profiling.hpp>

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
T FindNearestTime(const std::vector<League::Animation::Bone::Frame<T>>& a_Vector, float a_Time, size_t& a_Index)
{
	auto t_Min = a_Vector[0];
	auto t_Max = a_Vector[a_Vector.size() - 1];

	if (a_Time < a_Vector[a_Index].Time)
		a_Index = 0;

	a_Index = a_Index ? a_Index - 1 : 0;

	for (; a_Index < a_Vector.size(); a_Index++)
	{
		const auto& t_Current = a_Vector[a_Index];

		if (t_Current.Time <= a_Time)
		{
			t_Min = t_Current;
			continue;
		}

		t_Max = t_Current;
		break;
	}

	float t_Div = t_Max.Time - t_Min.Time;
	float t_LerpValue = (t_Div == 0) ? 1 : (a_Time - t_Min.Time) / t_Div;
	return Interpolate(t_Min.FrameData, t_Max.FrameData, t_LerpValue);
}

void ApplicationMesh::SetupHierarchy(const glm::mat4& a_InverseRoot, std::vector<glm::mat4>& a_Bones, const League::Skeleton::Bone& a_SkeletonBone, const glm::mat4& a_Parent, float a_Time)
{
	Profiler::ContextWithInfo t(__FUNCTION__, a_SkeletonBone.Name.c_str());

	glm::mat4 t_GlobalTransform = a_Parent;

	const auto* t_AnimBone = Animations[CurrentAnimation].GetBone(a_SkeletonBone.Hash);
	if (t_AnimBone != nullptr)
	{
		glm::vec3 t_Translation = FindNearestTime(t_AnimBone->Translation, a_Time, m_CurrentFrame[a_SkeletonBone.ID].Translation);
		glm::quat t_Rotation = FindNearestTime(t_AnimBone->Rotation, a_Time, m_CurrentFrame[a_SkeletonBone.ID].Rotation);
		glm::vec3 t_Scale = FindNearestTime(t_AnimBone->Scale, a_Time, m_CurrentFrame[a_SkeletonBone.ID].Scale);

		auto t_LocalTransform = glm::translate(t_Translation) * glm::mat4_cast(t_Rotation) * glm::scale(t_Scale);
		t_GlobalTransform = a_Parent * t_LocalTransform;
	}
	//else printf("Animation bone %s is not found\n", a_SkeletonBone.Name.c_str());

	a_Bones[a_SkeletonBone.ID] = t_GlobalTransform * a_SkeletonBone.InverseGlobalMatrix;

	for (auto& t_Child : a_SkeletonBone.Children)
		SetupHierarchy(a_InverseRoot, a_Bones, *t_Child, t_GlobalTransform, a_Time);
};

void ApplicationMesh::Draw(float a_Time, ShaderProgram& a_Program, glm::mat4& a_VP, Texture* a_Diffuse, std::vector<glm::mat4>* a_BoneTransforms)
{
	Profiler::Frame t(__FUNCTION__, a_Time);

	a_Program.Use();

	if (PositionBuffer) PositionBuffer->Use();
	if (UVBuffer) UVBuffer->Use();
	if (NormalBuffer) NormalBuffer->Use();
	if (BoneIndexBuffer) BoneIndexBuffer->Use();
	if (BoneWeightBuffer) BoneWeightBuffer->Use();

	bool t_HasVisible = false;
	for (auto& t_Submesh : SubMeshes)
	{
		if (t_Submesh.Visible)
		{
			t_HasVisible = true;
			break;
		}
	}

	if (!t_HasVisible)
		return;

	if (Skeleton && a_BoneTransforms)
	{
		const auto& t_AnimationIndex = Animations.find(CurrentAnimation);
		if (t_AnimationIndex != Animations.end())
		{
			float t_AnimationDuration = t_AnimationIndex->second.GetDuration();
			while (a_Time > t_AnimationDuration)
			{
				a_Time -= t_AnimationDuration;
				for (auto t_AnimationEvent : AnimationEvents[CurrentAnimation])
					t_AnimationEvent->Reset();
			}

			float t_FPS = t_AnimationIndex->second.GetFPS();
			for (auto t_AnimationEvent : AnimationEvents[CurrentAnimation])
				t_AnimationEvent->Update(a_Time * t_FPS);

			auto& t_Bones = Skeleton->GetBones();
			while (m_CurrentFrame.size() < t_Bones.size())
				m_CurrentFrame.push_back(BoneFrameIndexCache());

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

	for (auto& t_Submesh : SubMeshes)
	{
		if (t_Submesh.Visible == false) continue;

		if (a_Diffuse) *a_Diffuse = t_Submesh.HasImage ? t_Submesh.Image : Application::Instance->GetDefaultTexture();


		a_VP *= t_Submesh.GetTransformMatrix();

		a_Program.Update();
		t_Submesh.IndexBuffer->Draw();
	}
}

void ApplicationMesh::SetupAnimation(std::vector<glm::mat4>& a_BoneTransforms, float a_Time)
{
	Profiler::Context t(__FUNCTION__);

	glm::mat4 t_InverseRoot = glm::identity<glm::mat4>();
	const auto& t_Bones = Animations[CurrentAnimation].GetBones();

	for (size_t i = 0; i < t_Bones.size(); i++)
	{
		const auto& t_AnimBone = t_Bones[i];
		const auto* t_SkellyBone = Skeleton->GetBone(t_AnimBone.Hash);

		if (t_SkellyBone == nullptr || t_SkellyBone->Parent != nullptr) continue;

		SetupHierarchy(t_InverseRoot, a_BoneTransforms, *t_SkellyBone, glm::identity<glm::mat4>(), a_Time);
	}
}

glm::mat4 ApplicationMesh::SubMesh::GetTransformMatrix() const
{
	return glm::translate(Position) * glm::mat4_cast(Rotation) * glm::scale(Scale);
}

void ApplicationMesh::SubMesh::SetTexture(const std::string& a_FilePath)
{
	Profiler::Context t(__FUNCTION__);

	printf("Attempting to set texture '%s' for this submesh\n", a_FilePath.c_str());
	Image.Load(a_FilePath, [](Texture& a_Texture, void* a_UserData)
	{
		Profiler::Context t("ApplicationMesh::SubMesh::SetTexture->OnLoad");

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

ApplicationMesh::~ApplicationMesh()
{
	for (auto t_AnimationEventList : AnimationEvents)
		for (auto t_AnimationEvent : t_AnimationEventList.second)
			delete t_AnimationEvent;
	AnimationEvents.clear();
}

void ApplicationMesh::AddAnimationReference(const std::string& a_Name, const League::Animation & a_Animation)
{
	Animations[a_Name] = a_Animation;
}

void ApplicationMesh::ApplyAnimation(const std::string& a_Animation)
{
	Profiler::Context t(__FUNCTION__);

	const auto& t_AnimationIndex = Animations.find(a_Animation);
	if (t_AnimationIndex == Animations.end())
	{
		printf("There was a request to play the animation %s, but I haven't got that one in my system!\n", a_Animation.c_str());
		return;
	}

	CurrentAnimation = a_Animation;
	const auto& t_Animation = Animations[CurrentAnimation];
	const auto& t_Bones = t_Animation.GetBones();

	printf("Applying an animation with the following data:\n");
	printf("Name: %s\n", CurrentAnimation.c_str());
	printf("Duration: %f seconds\n", t_Animation.GetDuration());
	printf("FPS: %f\n", t_Animation.GetFPS());
	printf("Bone count: %lu\n", t_Bones.size());

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

	printf("Height: %f\n", Center.y);
}

ApplicationMesh::SwapMeshAnimationEvent::SwapMeshAnimationEvent(ApplicationMesh & a_Mesh, float a_TriggerFrame, const std::vector<size_t>& a_ToShow, const std::vector<size_t>& a_ToHide) :
	AnimationEvent(a_Mesh), m_TriggerFrame(a_TriggerFrame), m_SubmeshesToShow(a_ToShow), m_SubmeshesToHide(a_ToHide)
{
}

void ApplicationMesh::SwapMeshAnimationEvent::Reset()
{
	for (size_t i = 0; i < m_Parent.SubMeshes.size(); i++)
		m_Parent.SubMeshes[i].Visible = m_Parent.SubMeshes[i].InitialVisibility;
	m_Triggered = false;
}

void ApplicationMesh::SwapMeshAnimationEvent::Update(float a_Frame)
{
	if (a_Frame < m_TriggerFrame || m_Triggered) return;
	m_Triggered = true;

	for (size_t i = 0; i < m_SubmeshesToHide.size(); i++)
		m_Parent.SubMeshes[m_SubmeshesToHide[i]].Visible = false;

	for (size_t i = 0; i < m_SubmeshesToShow.size(); i++)
		m_Parent.SubMeshes[m_SubmeshesToShow[i]].Visible = true;
}
