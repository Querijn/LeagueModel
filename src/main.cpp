//// Taliyaah
//auto t_TextureFileLocation = "output/assets/characters/taliyah/skins/base/taliyah_base_tx_cm.dds"; 
//auto t_SkinFileLocation = "output/assets/characters/taliyah/skins/base/taliyah_base.skn";
//auto t_SkeletonFileLocation = "output/assets/characters/taliyah/skins/base/taliyah_base.skl";

//// Morde
//auto t_TextureFileLocation = "output/assets/characters/mordekaiser/skins/base/mordekaiser.dds"; 
//auto t_SkinFileLocation = "output/assets/characters/mordekaiser/skins/base/mordekaiser.skn";
//auto t_SkeletonFileLocation = "output/assets/characters/mordekaiser/skins/base/mordekaiser.skl";
//auto t_AnimationFileLocation =	"output/assets/characters/mordekaiser/skins/base/animations/mordekaiser_idle1.anm";

// Poppy
auto t_TextureFileLocation =	"data/output/assets/characters/poppy/skins/base/poppy_base_tx_cm.dds";
auto t_SkinFileLocation =		"data/output/assets/characters/poppy/skins/base/poppy.skn";
auto t_SkeletonFileLocation =	"data/output/assets/characters/poppy/skins/base/poppy.skl";
auto t_AnimationFileLocation =	"data/output/assets/characters/poppy/skins/base/animations/poppy_run.anm";

#include <league_model/league_skin.hpp>
#include <league_model/league_skeleton.hpp>
#include <league_model/league_animation.hpp>

#include <event_handler.hpp>
#include <event_handler/events.hpp>
#include <platform.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

#include <renderer.hpp>
#include <window.hpp>

Window* g_Window;

ShaderProgram g_ShaderProgram;
//ShaderProgram g_ColourProgram;
Shader g_VertexShader(Shader::Type::Vertex);
Shader g_FragmentShader(Shader::Type::Fragment);
//Shader g_ColourVertexShader(Shader::Type::Vertex);
//Shader g_ColourFragmentShader(Shader::Type::Fragment);

VertexBuffer<glm::vec3>* g_PositionBuffer;
VertexBuffer<glm::vec2>* g_UVBuffer;
VertexBuffer<glm::vec3>* g_NormalBuffer;
VertexBuffer<glm::vec4>* g_BoneIndexBuffer;
VertexBuffer<glm::vec4>* g_BoneWeightBuffer;
IndexBuffer<uint16_t>* g_IndexBuffer;

bool g_ModelDirty = true;
glm::vec3 g_Rotation;
glm::vec3 g_Position;
glm::vec3 g_Scale;
float g_Time = 0;
float g_AnimDuration;

bool g_CameraDirty = true;
glm::vec3 g_CameraPosition = glm::vec3(0, 0, 1000);

glm::mat4 g_ModelMatrix;
glm::mat4 g_ViewMatrix;
glm::mat4 g_ProjectionMatrix;
ShaderVariable<glm::mat4>* g_MVP, *g_MVP2;
ShaderVariable<std::vector<glm::mat4>>* g_Bones;
ShaderVariable<Texture>* g_Texture;

League::Skin* g_Skin;
League::Skeleton* g_Skeleton;
League::Animation* g_Animation;

std::map<uint16_t, std::vector<std::pair<float, glm::mat4>>> g_BoneTransform;

unsigned int g_ShadersLoadedCount = 0;
unsigned int g_ColourShadersLoadedCount = 0;
bool g_SkinLoaded = false;
bool g_SkeletonLoaded = false;
bool g_AnimationLoaded = false;

bool g_MouseIsDown = false;
glm::vec2 g_MousePos, g_LastMousePos;

void UploadBuffers()
{
	if (g_SkinLoaded == false /*|| g_SkeletonLoaded == false || g_AnimationLoaded == false*/ || g_ShadersLoadedCount != 2) return;

	g_PositionBuffer = g_ShaderProgram.GetVertexBuffer<glm::vec3>("v_Positions");
	g_UVBuffer = g_ShaderProgram.GetVertexBuffer<glm::vec2>("v_UVs");
	g_NormalBuffer = g_ShaderProgram.GetVertexBuffer<glm::vec3>("v_Normals");
	g_BoneIndexBuffer = g_ShaderProgram.GetVertexBuffer<glm::vec4>("v_BoneIndices");
	g_BoneWeightBuffer = g_ShaderProgram.GetVertexBuffer<glm::vec4>("v_BoneWeights");
	g_IndexBuffer = &g_ShaderProgram.GetIndexBuffer<uint16_t>();

	if (g_PositionBuffer) g_PositionBuffer->Upload(g_Skin->Positions);
	if (g_UVBuffer) g_UVBuffer->Upload(g_Skin->UVs);
	if (g_NormalBuffer) g_NormalBuffer->Upload(g_Skin->Normals);
	if (g_BoneWeightBuffer) g_BoneWeightBuffer->Upload(g_Skin->Weights);
	if (g_BoneIndexBuffer) g_BoneIndexBuffer->Upload(g_Skin->BoneIndices);
	g_IndexBuffer->Upload(g_Skin->Indices);
}

//void OnColourShaderLoad(Shader* a_Shader, BaseFile::LoadState a_LoadState)
//{
//	if (a_LoadState != BaseFile::LoadState::Loaded)
//	{
//		printf("A shader has %s!\n", a_LoadState == BaseFile::LoadState::FailedToLoad ? "failed to load" : "not been loaded, could not find it");
//		return;
//	}
//
//	g_ColourShadersLoadedCount++;
//	if (g_ColourShadersLoadedCount == 2)
//	{
//		g_ColourProgram.Init({ g_ColourVertexShader, g_ColourFragmentShader });
//		g_MVP2 = g_ColourProgram.GetVariable<glm::mat4>("u_MVP");
//	}
//}

void OnShaderLoad(Shader* a_Shader, BaseFile::LoadState a_LoadState)
{
	if (a_LoadState != BaseFile::LoadState::Loaded)
	{
		printf("A shader has %s!\n", a_LoadState == BaseFile::LoadState::FailedToLoad ? "failed to load" : "not been loaded, could not find it");
		return;
	}

	g_ShadersLoadedCount++;
	if (g_ShadersLoadedCount == 2)
	{
		g_ShaderProgram.Init({ g_VertexShader, g_FragmentShader });
		g_MVP = g_ShaderProgram.GetVariable<glm::mat4>("u_MVP");
		g_Texture = g_ShaderProgram.GetVariable<Texture>("u_Diffuse");
		g_Bones = g_ShaderProgram.GetVariable<std::vector<glm::mat4>>("u_Bones");

		if (g_Texture != nullptr)
		{
			Texture& t_Image = (Texture&)(*g_Texture);
			t_Image.Load(t_TextureFileLocation, [](Texture* a_Texture, BaseFile::LoadState a_LoadState)
			{
				if (a_LoadState == BaseFile::LoadState::Loaded) return;
				printf("Texture has %s!\n", a_LoadState == BaseFile::LoadState::FailedToLoad ? "failed to load" : "not been loaded, could not find it");
			});
		}

		UploadBuffers();
	}
}

void DebugDrawBones()
{
	auto& t_Bones = (std::vector<glm::mat4>&)*g_Bones;

	glBegin(GL_LINES);
	glLineWidth(14.0f);
	for (size_t i = 0; i < t_Bones.size(); i++)
	{
		auto& t_Bone = g_Skeleton->Bones[i];
		const auto& t_SkellyBone = g_Skeleton->Bones[i];
		if (t_SkellyBone.parent == -1) continue;
		if (strcmp(t_SkellyBone.name, t_Bone.name) != 0) throw 0;

		auto t_ParentPos = t_Bones[t_SkellyBone.parent] * glm::vec4(1.0f);
		auto t_Pos = t_Bones[i] * glm::vec4(1.0f);

		glVertex3f(t_Pos.x, t_Pos.y, t_Pos.z);
		glVertex3f(t_ParentPos.x, t_ParentPos.y, t_ParentPos.z);
	}
	glEnd();
}

template<typename T>
T FindNearestTime(const std::vector<std::pair<float, T>>& a_Vector)
{
	auto t_Min = a_Vector[0];
	auto t_Max = a_Vector[a_Vector.size() - 1];

	for (size_t i = 0; i < a_Vector.size(); i++)
	{
		const auto& t_Current = a_Vector[i];

		if (t_Current.first <= g_Time)
		{
			t_Min = t_Current;
			continue;
		}

		if (t_Current.first > g_Time)
		{
			t_Max = t_Current;
			break;
		}

		printf("Yikes! FindNearestTime could not find a time compatible with this animation!\n");
		throw 0; // Should not happen
	}

	float t_Div = t_Max.first - t_Min.first;
	float t_LerpValue = (t_Div == 0) ? 1 : (g_Time - t_Min.first) / t_Div;
	return glm::lerp(t_Min.second, t_Max.second, t_LerpValue);
}

League::Skeleton::Bone* GetSkeletonBone(const League::Animation::Bone& a_AnimBone)
{
	for (size_t i = 0; i < g_Skeleton->Bones.size(); i++)
		if (strcmp(g_Skeleton->Bones[i].name, a_AnimBone.name.c_str()) == 0)
			return &g_Skeleton->Bones[i];

	return nullptr;
}

League::Animation::Bone* GetAnimationBone(const League::Skeleton::Bone& a_SkeletonBone)
{
	for (size_t i = 0; i < g_Animation->Bones.size(); i++)
		if (strcmp(g_Animation->Bones[i].name.c_str(), a_SkeletonBone.name) == 0)
			return &g_Animation->Bones[i];

	return nullptr;
}

void SetupHierarchy(const glm::mat4 a_InverseRoot, std::vector<glm::mat4>& t_Bones, size_t i, const League::Animation::Bone& a_AnimBone, const League::Skeleton::Bone& a_SkeletonBone, const glm::mat4& a_Parent)
{
	glm::vec3 t_Translation = FindNearestTime(a_AnimBone.translation);
	glm::quat t_Rotation = FindNearestTime(a_AnimBone.quaternion);
	glm::vec3 t_Scale = FindNearestTime(a_AnimBone.scale);

	auto t_LocalTransform = glm::translate(t_Translation) * glm::mat4_cast(t_Rotation) * glm::scale(t_Scale);
	auto t_GlobalTransform = a_Parent * t_LocalTransform;

	t_Bones[a_SkeletonBone.id] = a_InverseRoot * t_GlobalTransform * a_SkeletonBone.localMatrix;

	for (auto& t_Child : a_SkeletonBone.Children)
	{
		const auto* t_AnimationBone = GetAnimationBone(a_SkeletonBone);
		if (t_AnimationBone == nullptr) continue;

		SetupHierarchy(a_InverseRoot, t_Bones, t_Child->id, *t_AnimationBone, *t_Child, t_Bones[a_SkeletonBone.id]);
	}
};
			
void CalculateBones()
{
	const bool t_HasAnimation = g_Animation != nullptr && g_Animation->Bones.size() != 0;
	g_AnimDuration = t_HasAnimation ? g_Animation->Bones[0].scale.back().first : 0;
	if (g_Bones == nullptr || g_AnimDuration == 0) return;
	
	while (g_Time > g_AnimDuration) g_Time -= g_AnimDuration;

	auto t_InverseRoot = glm::inverse(g_ModelMatrix);
	auto& t_Bones = (std::vector<glm::mat4>&)*g_Bones;
	t_Bones.resize(g_Skeleton->Bones.size());

	for (size_t i = 0; i < g_Animation->Bones.size(); i++)
	{
		const auto& t_AnimBone = g_Animation->Bones[i];
		const auto* t_SkellyBone = GetSkeletonBone(t_AnimBone);

		if (t_SkellyBone->parent != -1) continue;

		SetupHierarchy(t_InverseRoot, t_Bones, i, t_AnimBone, *t_SkellyBone, glm::identity<glm::mat4>());
	}
}

int main(void)
{
	printf("Loading window\n");
	Window::WindowSettings t_Settings;
	g_Window = new Window(t_Settings);

	glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);

	printf("Loading shaders\n");
	g_VertexShader.Load("data/league_model.vert", OnShaderLoad);
	g_FragmentShader.Load("data/league_model.frag", OnShaderLoad);

	/*g_ColourVertexShader.Load("data/colour.vert", OnColourShaderLoad);
	g_ColourFragmentShader.Load("data/colour.frag", OnColourShaderLoad);*/

	printf("Loading skin (%s)\n", t_SkinFileLocation);
	g_Skin = new League::Skin();
	g_Skeleton = new League::Skeleton(*g_Skin);
	g_Skin->Load(t_SkinFileLocation, [](League::Skin* a_Skin, BaseFile::LoadState a_LoadState)
	{
		if (a_LoadState != BaseFile::LoadState::Loaded)
		{
			printf("Skin has %s!\n", a_LoadState == BaseFile::LoadState::FailedToLoad ? "failed to load" : "not been loaded, could not find it");
			return;
		}

		g_SkinLoaded = true;
		g_Position = -(g_Skin->BoundingMax + g_Skin->BoundingMin) * 0.5f;
		
		/*

		printf("Loading skeleton (%s)\n", t_SkeletonFileLocation);
		g_Skeleton->Load(t_SkeletonFileLocation, [](League::Skeleton* a_Skeleton, BaseFile::LoadState a_LoadState)
		{
			if (a_LoadState != BaseFile::LoadState::Loaded)
			{
				printf("Skeleton has %s!\n", a_LoadState == BaseFile::LoadState::FailedToLoad ? "failed to load" : "not been loaded, could not find it");
				return;
			}					

			g_SkeletonLoaded = true;

			printf("Loading animation (%s)\n", t_AnimationFileLocation);
			g_Animation = new League::Animation();
			g_Animation->Load(t_AnimationFileLocation, *a_Skeleton, [](League::Animation* a_Animation, BaseFile::LoadState a_LoadState)
			{
				if (a_LoadState != BaseFile::LoadState::Loaded) 
				{
					printf("Animation has %s!\n", a_LoadState == BaseFile::LoadState::FailedToLoad ? "failed to load" : "not been loaded, could not find it");
					return;
				}

				g_AnimationLoaded = true;*/
				UploadBuffers();
		/*	});
		});*/
	});

	printf("Initialising event handler\n");
	EventHandler::Init();

	MouseDownEvent::AddListener([](const MouseDownEvent* a_Event, void* a_Data) 
	{ 
		if (a_Event->Button != Mouse::Button::Left) return;
		
		g_MouseIsDown = true;
		g_MousePos.x = g_LastMousePos.x = a_Event->x;
		g_MousePos.y = g_LastMousePos.y = a_Event->y;
	});

	MouseUpEvent::AddListener([](const MouseUpEvent* a_Event, void* a_Data)
	{
		if (a_Event->Button != Mouse::Button::Left) return;

		g_MouseIsDown = false;
	});

	MouseScrollEvent::AddListener([](const MouseScrollEvent* a_Event, void* a_Data)
	{
		g_CameraPosition.z -= a_Event->Change;
		g_CameraPosition.z = glm::clamp(g_CameraPosition.z, 150.0f, 1000.0f);
		g_CameraDirty = true;
	});

	PointerMoveEvent::AddListener([](const PointerMoveEvent* a_Event, void* a_Data)
	{
		if (g_MouseIsDown == false) return;
		g_MousePos.x = a_Event->x;
		g_MousePos.y = a_Event->y;
	});

	printf("Setting up loop\n");
	Platform::SetMainLoop([]()
	{
		EventHandler::EmitEvent<UpdateEvent>(0.0f);
		EventHandler::Run();

		if (g_SkinLoaded == false /*|| g_SkeletonLoaded == false || g_AnimationLoaded == false*/ || g_ShadersLoadedCount != 2)
		{
			static bool t_SkinWasLoaded = false;
			static bool t_SkeletonWasLoaded = false;
			static bool t_AnimationWasLoaded = false;
			static size_t t_ShadersLoadedCountPrev = 0;
			static bool t_FirstRun = true;

			if (t_FirstRun || g_SkinLoaded != t_SkinWasLoaded || g_SkeletonLoaded != t_SkeletonWasLoaded || g_AnimationLoaded != t_AnimationWasLoaded || g_ShadersLoadedCount != t_ShadersLoadedCountPrev)
			{
				printf("Main loop: Waiting for files to load (Skin = %s, Skeleton = %s, Animation = %s, %d shaders loaded)\n", g_SkinLoaded ? "Loaded" : "Not loaded", g_SkeletonLoaded ? "Loaded" : "Not loaded", g_AnimationLoaded ? "Loaded" : "Not loaded", g_ShadersLoadedCount);

				t_FirstRun = false;
				t_SkinWasLoaded = g_SkinLoaded;
				t_SkeletonWasLoaded = g_SkeletonLoaded;
				t_AnimationWasLoaded = g_AnimationLoaded;
				t_ShadersLoadedCountPrev = g_ShadersLoadedCount;
			}
			
			g_Window->SwapBuffers();
			return g_Window->RunFrame();
		}

		if (g_CameraDirty)
		{
			g_ViewMatrix = glm::lookAt(g_CameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
			g_CameraDirty = false;
		}
		
		glm::vec2 t_RotationDelta = g_MousePos - g_LastMousePos;
		if (g_MouseIsDown && (abs(t_RotationDelta.x) > 0.01f || abs(t_RotationDelta.y) > 0.01f))
		{
			g_Rotation += glm::vec3(t_RotationDelta.y, t_RotationDelta.x, 0.0f) * 0.0174532925f;
			g_LastMousePos = g_MousePos;
			g_ModelDirty = true;
		}

		if (g_ModelDirty)
		{
			g_ModelMatrix = glm::mat4_cast(glm::quat(g_Rotation)) * glm::translate(g_Position);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		g_ShaderProgram.Use();

		CalculateBones();

		glViewport(0, 0, g_Window->GetWidth(), g_Window->GetHeight());
		g_ProjectionMatrix = glm::perspective(45.0f, (float)g_Window->GetWidth() / (float)g_Window->GetHeight(), 0.1f, 1500.0f);
		*g_MVP = g_ProjectionMatrix * g_ViewMatrix * g_ModelMatrix;

		g_ShaderProgram.Update();

		if (g_PositionBuffer) g_PositionBuffer->Use();
		if (g_UVBuffer) g_UVBuffer->Use();
		if (g_NormalBuffer) g_NormalBuffer->Use();
		if (g_BoneIndexBuffer) g_BoneIndexBuffer->Use();
		if (g_BoneWeightBuffer) g_BoneWeightBuffer->Use();
		g_IndexBuffer->Draw();

//#if defined(_DEBUG)
//		if (g_Bones != nullptr && g_AnimDuration != 0 && g_ColourShadersLoadedCount == 2)
//		{
//			g_ColourProgram.Use();
//
//			*g_MVP2 = g_ProjectionMatrix * g_ViewMatrix * g_ModelMatrix;
//
//			g_ColourProgram.Update();
//			glClear(GL_DEPTH_BUFFER_BIT);
//			DebugDrawBones();
//		}
//#endif

		g_Window->SwapBuffers();

		g_Time += 0.016;
		return g_Window->RunFrame();
	});

	printf("Setting up post-exit deletes\n");
	std::atexit([]()
	{
		delete g_Skin;
		delete g_Skeleton;
		delete g_Window;
	});

	return 0;
}