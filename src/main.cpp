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
	if (g_SkinLoaded == false || g_ShadersLoadedCount != 2) return;

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