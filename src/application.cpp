#include <application.hpp>
#include <platform.hpp>
#include <event_handler.hpp>
#include <event_handler/events.hpp>

#include <league/bin_valuestorage.hpp>
#include <league/bin.hpp>
#include <league/skin.hpp>
#include <league/skeleton.hpp>
#include <league/animation.hpp>

#include <algorithm>

#include <profiling/memory.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

#include <fstream>

Application* Application::Instance = nullptr;
double g_LastTime = 0;

Application::Application(const char* a_Root) :
	m_Window(Window::WindowSettings()),

	m_VertexShader(Shader::Type::Vertex),
	m_FragmentShader(Shader::Type::Fragment),

	m_Root(a_Root)
{
	Instance = this;

	if (m_Root[m_Root.size() - 1] != '/')
		m_Root += "/";
}

Application::~Application()
{
	for (auto t_Animation : m_Animations)
	{
		Delete(t_Animation);
		t_Animation = nullptr;
	}

	m_Animations.clear();
	m_Meshes.clear();
}

void Application::Init()
{
	EventHandler::Init();

	ADD_MEMBER_FUNCTION_EVENT_LISTENER(MouseDownEvent, Application, OnMouseDownEvent);
	ADD_MEMBER_FUNCTION_EVENT_LISTENER(MouseUpEvent, Application, OnMouseUp);
	ADD_MEMBER_FUNCTION_EVENT_LISTENER(PointerUpEvent, Application, OnPointerUpEvent);
	ADD_MEMBER_FUNCTION_EVENT_LISTENER(MouseScrollEvent, Application, OnMouseScrollEvent);
	ADD_MEMBER_FUNCTION_EVENT_LISTENER(PointerMoveEvent, Application, OnPointerMoveEvent);

	glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);

	LoadDefaultTexture();
	LoadShaders();

	LoadSkin("data/output/data/characters/neeko/skins/skin0.bin", "data/output/data/characters/neeko/animations/skin0.bin");
	UpdateViewMatrix();

	Platform::SetMainLoop([]() 
	{
		double dt = Platform::GetTimeSinceStart() - g_LastTime;
		g_LastTime = Platform::GetTimeSinceStart();

		EventHandler::EmitEvent<UpdateEvent>(dt);
		EventHandler::Run();

		return Instance->Update(dt); 
	});
}

void Application::LoadSkin(std::string a_BinPath, std::string a_AnimationBinPath)
{
	struct LoadData
	{
		LoadData(std::string a_AnimationBinPath) : AnimationBinPath(a_AnimationBinPath) {}

		std::string Texture;
		std::string AnimationBinPath;
		std::string AnimationName;
		bool FirstAnimationApplied = false;
		size_t References = 0;
		Application::Mesh* Target;
		League::Bin SkinBin;
		League::Bin AnimationBin;
	};
	auto* t_LoadData = New(LoadData(a_AnimationBinPath));

	// Load in the BIN containing most of the information about the base mesh
	t_LoadData->SkinBin.Load(a_BinPath, [](League::Bin& a_Bin, void* a_UserData)
	{
		if (a_Bin.GetLoadState() != File::LoadState::Loaded)
		{
			printf("Skin information bin %s!\n", a_Bin.GetLoadState() != File::LoadState::FailedToLoad ? "was not found" : "failed to load");
			return;
		}
		printf("Skin information is loaded!\n");

		auto* t_LoadData = (LoadData*)a_UserData;
		auto t_MeshProperties = a_Bin.Get("skinMeshProperties");
		if (!t_MeshProperties)
		{
			printf("Mesh properties are missing..\n");
			Delete(t_LoadData);
			return;
		}

		printf("Mesh properties are valid!\n");

		const auto& t_Root = Application::Instance->GetAssetRoot();

		// Get the skeleton file
		auto t_SkeletonValue = (const League::StringValueStorage*)t_MeshProperties->GetChild("skeleton");
		if (!t_SkeletonValue) { Delete(t_LoadData); return; }
		std::string t_Skeleton = t_Root + t_SkeletonValue->Get();

		// Get the skin file
		auto t_SkinValue = (const League::StringValueStorage*)t_MeshProperties->GetChild("simpleSkin");
		if (!t_SkinValue) { Delete(t_LoadData); return; }
		std::string t_Skin = t_Root + t_SkinValue->Get();

		// Get the texture file
		auto t_TextureValue = (const League::StringValueStorage*)t_MeshProperties->GetChild("texture");
		if (!t_TextureValue) { Delete(t_LoadData); return; }
		t_LoadData->Texture = t_Root + t_TextureValue->Get();

		printf("Starting to load the mesh (%s and %s)..\n", t_Skin.c_str(), t_Skeleton.c_str());
		
		// Load the mesh (Skeleton + Skin)
		Application::Instance->LoadMesh(t_Skin, t_Skeleton, [](std::string a_SkinPath, std::string a_SkeletonPath, Application::Mesh* a_Mesh, void* a_UserData)
		{
			auto* t_LoadData = (LoadData*)a_UserData;
			if (a_Mesh == nullptr) return;

			t_LoadData->Target = a_Mesh;

			printf("Mesh loaded, loading texture %s..\n", t_LoadData->Texture.c_str());
			// Set the texture (async)
			for (int i = 0; i < a_Mesh->SubMeshes.size(); i++)
				a_Mesh->SubMeshes[i].SetTexture(t_LoadData->Texture);

			if (a_Mesh->Skeleton == nullptr)
			{
				if (t_LoadData->References == 0)
					Delete(t_LoadData);
				return;
			}

			// Load all the animations
			t_LoadData->AnimationBin.Load(t_LoadData->AnimationBinPath, [](League::Bin& a_Bin, void* a_UserData)
			{
				auto* t_LoadData = (LoadData*)a_UserData;
				if (a_Bin.GetLoadState() != File::LoadState::Loaded) return;

				auto t_AnimationNames = a_Bin.Find([](const League::Bin::ValueStorage& a_ValueStorage, void* a_UserData)
				{
					if (a_ValueStorage.GetType() != League::Bin::ValueStorage::Type::String)
						return false;

					return a_ValueStorage.Is("mAnimationFilePath");
				});
				printf("Found all of the animations! We have %lu animations.\n", t_AnimationNames.size());

				const auto& t_Root = Application::Instance->GetAssetRoot();

				t_LoadData->References++;
				for (auto t_AnimationNameStorage : t_AnimationNames)
				{
					auto t_StringStorage = (const League::StringValueStorage*)t_AnimationNameStorage;
					t_LoadData->AnimationName = t_Root + t_StringStorage->Get();

					Instance->AddAnimationReference(*t_LoadData->Target, t_LoadData->AnimationName);

					if (t_LoadData->FirstAnimationApplied) continue;
					t_LoadData->References++;
					Instance->LoadAnimation(*t_LoadData->Target, t_LoadData->AnimationName, [](League::Animation& a_Animation, void* a_UserData)
					{
						auto* t_LoadData = (LoadData*)a_UserData;
						t_LoadData->References--;

						if (a_Animation.GetLoadState() != File::LoadState::Loaded)
						{
							if (t_LoadData->References == 0)
								Delete(t_LoadData);
							return;
						}

						t_LoadData->Target->AddAnimationReference(t_LoadData->AnimationName, a_Animation);
						t_LoadData->Target->ApplyAnimation(t_LoadData->AnimationName);
						       
						if (t_LoadData->References == 0)
							Delete(t_LoadData);
					}, t_LoadData);
					t_LoadData->FirstAnimationApplied = true;
				}

				// On Windows it's all synchronous, so try to delete here
				t_LoadData->References--;
				if (t_LoadData->References == 0)
					Delete(t_LoadData);
			}, t_LoadData);

		}, t_LoadData);
	}, t_LoadData);
}

void Application::LoadMesh(std::string a_SkinPath, std::string a_SkeletonPath, OnMeshLoadFunction a_OnLoadFunction, void* a_UserData)
{
	m_Meshes.clear();
	m_Animations.clear();
	m_AvailableAnimations.clear();

	struct LoadData
	{
		LoadData(std::string a_SkinPath, std::string a_SkeletonPath, OnMeshLoadFunction a_Function, void* a_Argument) :
			SkinPath(a_SkinPath), SkeletonPath(a_SkeletonPath), OnLoadFunction(a_Function), Argument(a_Argument)
		{}

		std::string SkinPath;
		std::string SkeletonPath;
		League::Skin SkinTarget;
		OnMeshLoadFunction OnLoadFunction;
		void* Argument;
	};
	auto* t_LoadData = New(LoadData(a_SkinPath, a_SkeletonPath, a_OnLoadFunction, a_UserData));

	t_LoadData->SkinTarget.Load(a_SkinPath, [](League::Skin& a_Skin, void* a_Argument)
	{
		auto& t_LoadData = *(LoadData*)a_Argument;
		if (a_Skin.GetLoadState() != File::LoadState::Loaded)
		{
			if (t_LoadData.OnLoadFunction) t_LoadData.OnLoadFunction(t_LoadData.SkinPath, t_LoadData.SkeletonPath, nullptr, t_LoadData.Argument);
			Delete(&t_LoadData);
			return;
		}

		auto* t_Skeleton = new League::Skeleton(a_Skin);
		t_Skeleton->Load(t_LoadData.SkeletonPath, [](League::Skeleton& a_Skeleton, void* a_Argument)
		{
			auto& t_LoadData = *(LoadData*)a_Argument;
			auto& t_Meshes = Instance->m_Meshes;
			auto& t_ShaderProgram = Instance->m_ShaderProgram;
			auto& t_Skin = t_LoadData.SkinTarget;

			// We can only start loading the mesh AFTER we load (or fail to load the skeleton), because the skeleton potentially modifies the bone indices
			Mesh t_Mesh;
			if (a_Skeleton.GetLoadState() == File::LoadState::Loaded)
				t_Mesh.Skeleton = std::make_shared<League::Skeleton>(a_Skeleton); // Now the mesh owns this pointer
			else
			{
				// Data is worthless to us, delete
				t_Mesh.Skeleton = nullptr;
				delete &a_Skeleton;
			}
			
			t_Mesh.PositionBuffer = t_ShaderProgram.GetVertexBuffer<glm::vec3>("v_Positions");
			t_Mesh.UVBuffer = t_ShaderProgram.GetVertexBuffer<glm::vec2>("v_UVs");
			t_Mesh.NormalBuffer = t_ShaderProgram.GetVertexBuffer<glm::vec3>("v_Normals");
			t_Mesh.BoneIndexBuffer = t_ShaderProgram.GetVertexBuffer<glm::vec4>("v_BoneIndices");
			t_Mesh.BoneWeightBuffer = t_ShaderProgram.GetVertexBuffer<glm::vec4>("v_BoneWeights");

			if (t_Mesh.PositionBuffer) t_Mesh.PositionBuffer->Upload(t_Skin.GetPositions());
			if (t_Mesh.UVBuffer) t_Mesh.UVBuffer->Upload(t_Skin.GetUVs());
			if (t_Mesh.NormalBuffer) t_Mesh.NormalBuffer->Upload(t_Skin.GetNormals());
			if (t_Mesh.BoneWeightBuffer) t_Mesh.BoneWeightBuffer->Upload(t_Skin.GetWeights());
			if (t_Mesh.BoneIndexBuffer) t_Mesh.BoneIndexBuffer->Upload(t_Skin.GetBoneIndices());

			auto& t_BoundingBox = t_Skin.GetBoundingBox();
			auto& t_SkinMeshes = t_Skin.GetMeshes();
			for (int i = 0; i < t_SkinMeshes.size(); i++)
			{
				Application::Mesh::SubMesh t_Submesh;

				t_Submesh.IndexBuffer = &t_ShaderProgram.GetIndexBuffer<uint16_t>();
				auto& t_SkinMesh = t_SkinMeshes[i];
				t_Submesh.IndexBuffer->Upload(t_SkinMesh.Indices, t_SkinMesh.IndexCount);

				t_Mesh.SubMeshes.push_back(t_Submesh);
			}

			t_Meshes[t_LoadData.SkinPath] = t_Mesh;
			if (t_LoadData.OnLoadFunction) t_LoadData.OnLoadFunction(t_LoadData.SkinPath, t_LoadData.SkeletonPath, &t_Meshes[t_LoadData.SkinPath], t_LoadData.Argument);

			Delete(&t_LoadData);
		}, &t_LoadData);

	}, t_LoadData);
}

void Application::LoadAnimation(Application::Mesh & a_Mesh, std::string a_AnimationPath, League::Animation::OnLoadFunction a_OnLoadFunction, void * a_UserData)
{
	if (a_Mesh.Skeleton == nullptr)
	{
		League::Skin t_Skin;
		League::Skeleton t_FakeSkeleton(t_Skin);
		League::Animation t_Animation(t_FakeSkeleton);
		if (a_OnLoadFunction) a_OnLoadFunction(t_Animation, a_UserData);
		return;
	}

	struct LoadData
	{
		LoadData(std::string a_AnimationPath, League::Animation::OnLoadFunction a_Function, void* a_Argument) :
			AnimationPath(a_AnimationPath), OnLoadFunction(a_Function), Argument(a_Argument)
		{}

		std::string AnimationPath;
		std::string SkeletonPath;
		League::Animation::OnLoadFunction OnLoadFunction;
		void* Argument;
	};
	auto* t_LoadData = New(LoadData(a_AnimationPath, a_OnLoadFunction, a_UserData));

	auto* t_Animation = New(League::Animation(*a_Mesh.Skeleton));
	t_Animation->Load(a_AnimationPath, [](League::Animation& a_Animation, void* a_Argument)
	{
		auto& t_LoadData = *(LoadData*)a_Argument;
		if (a_Animation.GetLoadState() != File::LoadState::Loaded)
		{
			if (t_LoadData.OnLoadFunction) t_LoadData.OnLoadFunction(a_Animation, t_LoadData.Argument);
			Delete(&t_LoadData);
			Delete(&a_Animation);
			return;
		}

		Instance->m_Animations.push_back(&a_Animation);
		if (t_LoadData.OnLoadFunction) t_LoadData.OnLoadFunction(a_Animation, t_LoadData.Argument);
		Delete(&t_LoadData);
	}, t_LoadData);
}

void Application::AddAnimationReference(Application::Mesh & a_Mesh, const std::string & a_AnimationName)
{
	m_AvailableAnimations[&a_Mesh].push_back(a_AnimationName);
}

const Texture & Application::GetDefaultTexture() const
{
	return m_DefaultTexture;
}

Application::Mesh * Application::GetMeshUnsafe(const std::string & a_Name)
{
	auto t_Index = m_Meshes.find(a_Name);
	if (t_Index == m_Meshes.end())
		return nullptr;

	return &t_Index->second;
}

const Application::Mesh * Application::GetMesh(const std::string & a_Name) const
{
	auto t_Index = m_Meshes.find(a_Name);
	if (t_Index == m_Meshes.end())
		return nullptr;

	return &t_Index->second;
}

std::vector<std::string> Application::GetAnimationsForMesh(const Mesh & a_Mesh) const
{
	auto t_Index = m_AvailableAnimations.find(&a_Mesh);
	if (t_Index == m_AvailableAnimations.end())
		return std::vector<std::string>();

	return t_Index->second;
}

std::vector<std::string> Application::GetAnimationsForMesh(const std::string & a_Name) const
{
	auto t_Mesh = GetMesh(a_Name);
	if (t_Mesh == nullptr)
		return std::vector<std::string>();

	return GetAnimationsForMesh(*t_Mesh);
}

std::vector<std::string> Application::GetSkinFiles() const
{
	std::vector<std::string> t_Results;

	for (auto& t_MeshInfo : m_Meshes)
		t_Results.push_back(t_MeshInfo.first);

	return t_Results;
}

const std::string & Application::GetAssetRoot() const
{
	return m_Root;
}

void Application::OnMouseDownEvent(const MouseDownEvent * a_Event)
{
	if (a_Event->Button != Mouse::Button::Left) return;

	m_MouseIsDown = true;
	m_MousePosition = glm::vec2(a_Event->x, a_Event->y);
}

void Application::OnMouseUp(const MouseUpEvent * a_Event)
{
	if (a_Event->Button != Mouse::Button::Left) return;

	m_MouseIsDown = false;
}

void Application::OnPointerDownEvent(const PointerDownEvent * a_Event)
{
	if (a_Event->index != 0) return;

	m_MouseIsDown = true;
	m_MousePosition = glm::vec2(a_Event->x, a_Event->y);
}

void Application::OnPointerUpEvent(const PointerUpEvent * a_Event)
{
	if (a_Event->index != 0) return;

	m_MouseIsDown = false;
}

void Application::OnMouseScrollEvent(const MouseScrollEvent * a_Event)
{
	m_CameraDistance -= a_Event->Change;
	m_CameraDistance = glm::clamp(m_CameraDistance, 150.0f, 1000.0f);
	UpdateViewMatrix();
}

void Application::OnPointerMoveEvent(const PointerMoveEvent * a_Event)
{
	if (m_MouseIsDown == false) return;

	glm::vec2 t_CurrentMousePosition = glm::vec2(a_Event->x, a_Event->y);
	glm::vec2 t_RotationDelta = t_CurrentMousePosition - m_MousePosition;
	if (abs(t_RotationDelta.x) < 0.01f && abs(t_RotationDelta.y) < 0.01f) return;

	auto t_Rotation = glm::vec3(-t_RotationDelta.y, -t_RotationDelta.x, 0.0f) * 0.0174532925f;
	m_CameraPosition = glm::quat(t_Rotation) * m_CameraPosition;
	UpdateViewMatrix();

	m_MousePosition = t_CurrentMousePosition;
}

void Application::LoadDefaultTexture()
{
	m_DefaultTexture.Load("data/missing_texture.jpg");
}

void Application::LoadShaders()
{
	m_FragmentShader.Load("data/league_model.frag", [](Shader* a, void* b) { Instance->LoadShaderVariables(); });
	m_VertexShader.Load("data/league_model.vert", [](Shader* a, void* b) { Instance->LoadShaderVariables(); });
}

void Application::LoadShaderVariables()
{
	if (m_FragmentShader.GetLoadState() != File::LoadState::Loaded) return;
	if (m_VertexShader.GetLoadState() != File::LoadState::Loaded) return;

	m_ShaderProgram.Init({ m_FragmentShader, m_VertexShader });
	m_ShaderProgram.Use();

	m_MVPUniform = m_ShaderProgram.GetVariable<glm::mat4>("u_MVP");
	m_TextureUniform = m_ShaderProgram.GetVariable<Texture>("u_Diffuse");
	m_BoneArrayUniform = m_ShaderProgram.GetVariable<std::vector<glm::mat4>>("u_Bones");
}

void Application::UpdateViewMatrix()
{
	m_ViewMatrix = glm::lookAt(m_CameraPosition * m_CameraDistance, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}

bool Application::Update(double a_DT)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (m_MVPUniform == nullptr)
	{
		m_Window.SwapBuffers();
		return m_Window.RunFrame();
	}

	glViewport(0, 0, m_Window.GetWidth(), m_Window.GetHeight());
	m_ProjectionMatrix = glm::perspective(45.0f, (float)m_Window.GetWidth() / (float)m_Window.GetHeight(), 0.1f, 1500.0f);
	
	if (m_Meshes.size() == 0)
	{
		m_Window.SwapBuffers();
		return m_Window.RunFrame();
	}

	for (auto& t_MeshInfo : m_Meshes)
	{
		auto& t_DrawMesh = t_MeshInfo.second;
		if (t_DrawMesh.SubMeshes.size() == 0)
		{
			m_Window.SwapBuffers();
			return m_Window.RunFrame();
		}

		*m_MVPUniform = m_ProjectionMatrix * m_ViewMatrix * glm::translate(t_DrawMesh.Center);

		m_Time += a_DT;
		for (int i = 0; i < t_DrawMesh.SubMeshes.size(); i++)
			t_DrawMesh.Draw(i, m_Time, m_ShaderProgram, *m_MVPUniform, m_TextureUniform ? &m_TextureUniform->Get() : nullptr, m_BoneArrayUniform ? &m_BoneArrayUniform->Get() : nullptr);
	}

	m_Window.SwapBuffers();
	return m_Window.RunFrame();
}

int main()
{
	printf("LeagueModel Application built on %s at %s, calling new\n", __DATE__, __TIME__);
	auto* t_Application = new Application("data/output");
	Application::Instance->Init();

	std::atexit([]()
	{
		delete Application::Instance;
		Memory::Expose();
	});
}