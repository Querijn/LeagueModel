#include <application.hpp>
#include <platform.hpp>
#include <event_handler.hpp>
#include <event_handler/events.hpp>

#include <league/skin.hpp>
#include <league/skeleton.hpp>
#include <league/animation.hpp>

#include <algorithm>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

Application* Application::Instance = nullptr;
double g_LastTime = 0;

Application::Application() :
	m_Window(Window::WindowSettings()),

	m_VertexShader(Shader::Type::Vertex),
	m_FragmentShader(Shader::Type::Fragment)
{
	Instance = this;
}

Application::~Application()
{
	for (auto t_Animation : m_Animations)
	{
		delete t_Animation;
		t_Animation = nullptr;
	}

	m_Animations.clear();
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

	LoadMesh("data/141001/141001.skn", "data/141001/141001.skl", [](StringView a_SkinPath, StringView a_SkeletonPath, Application::Mesh* a_Mesh, void* a_UserData)
	{
		if (a_Mesh == nullptr) return;

		a_Mesh->SubMeshes[0].SetTexture("data/141001/main_texture.dds");

		Instance->LoadAnimation(*a_Mesh, "data/141001/Dance.anm", [](League::Animation& a_Animation, void* a_UserData)
		{
			if (a_Animation.GetLoadState() != File::LoadState::Loaded)
				return;

			auto* t_Mesh = (Application::Mesh*)a_UserData;
			t_Mesh->ApplyAnimation(a_Animation);
		}, a_Mesh);
	});

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

void Application::LoadMesh(StringView a_SkinPath, StringView a_SkeletonPath, OnMeshLoadFunction a_OnLoadFunction, void* a_UserData)
{
	struct LoadData
	{
		LoadData(StringView a_SkinPath, StringView a_SkeletonPath, OnMeshLoadFunction a_Function, void* a_Argument) :
			SkinPath(a_SkinPath), SkeletonPath(a_SkeletonPath), OnLoadFunction(a_Function), Argument(a_Argument)
		{}

		StringView SkinPath;
		StringView SkeletonPath;
		League::Skin* SkinTarget = nullptr;
		OnMeshLoadFunction OnLoadFunction;
		void* Argument;
	};
	auto* t_LoadData = new LoadData(a_SkinPath, a_SkeletonPath, a_OnLoadFunction, a_UserData);

	auto* t_Skin = new League::Skin();
	t_Skin->Load(a_SkinPath, [](League::Skin& a_Skin, void* a_Argument)
	{
		auto& t_LoadData = *(LoadData*)a_Argument;
		if (a_Skin.GetLoadState() != File::LoadState::Loaded)
		{
			if (t_LoadData.OnLoadFunction) t_LoadData.OnLoadFunction(t_LoadData.SkinPath, t_LoadData.SkeletonPath, nullptr, t_LoadData.Argument);
			delete &a_Skin;
			delete &t_LoadData;
			return;
		}

		t_LoadData.SkinTarget = &a_Skin;
		auto* t_Skeleton = new League::Skeleton(a_Skin);
		t_Skeleton->Load(t_LoadData.SkeletonPath, [](League::Skeleton& a_Skeleton, void* a_Argument)
		{
			auto& t_LoadData = *(LoadData*)a_Argument;
			auto& t_Meshes = Instance->m_Meshes;
			auto& t_ShaderProgram = Instance->m_ShaderProgram;
			auto& t_Skin = *t_LoadData.SkinTarget;

			// We can only start loading the mesh AFTER we load (or fail to load the skeleton), because the skeleton potentially modifies the bone indices
			Mesh t_Mesh;
			if (a_Skeleton.GetLoadState() == File::LoadState::Loaded)
				t_Mesh.Skeleton = std::shared_ptr<League::Skeleton>(&a_Skeleton); // Now the mesh owns this pointer
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

				t_Submesh.Position -= (t_BoundingBox.Min + t_BoundingBox.Max) * 0.5f;

				t_Submesh.IndexBuffer = &t_ShaderProgram.GetIndexBuffer<uint16_t>();
				auto& t_SkinMesh = t_SkinMeshes[i];
				t_Submesh.IndexBuffer->Upload(t_SkinMesh.Indices, t_SkinMesh.IndexCount);

				t_Mesh.SubMeshes.push_back(t_Submesh);
			}

			t_Meshes.push_back(t_Mesh);

			auto& t_MeshReference = t_Meshes.back();
			if (t_LoadData.OnLoadFunction) t_LoadData.OnLoadFunction(t_LoadData.SkinPath, t_LoadData.SkeletonPath, &t_MeshReference, t_LoadData.Argument);

			delete &t_Skin;
			delete &t_LoadData;
		}, &t_LoadData);

	}, t_LoadData);
}

void Application::LoadAnimation(Application::Mesh & a_Mesh, StringView a_AnimationPath, League::Animation::OnLoadFunction a_OnLoadFunction, void * a_UserData)
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
		LoadData(StringView a_AnimationPath, League::Animation::OnLoadFunction a_Function, void* a_Argument) :
			AnimationPath(a_AnimationPath), OnLoadFunction(a_Function), Argument(a_Argument)
		{}

		StringView AnimationPath;
		StringView SkeletonPath;
		League::Animation::OnLoadFunction OnLoadFunction;
		void* Argument;
	};
	auto* t_LoadData = new LoadData(a_AnimationPath, a_OnLoadFunction, a_UserData);

	auto* t_Animation = new League::Animation(*a_Mesh.Skeleton);
	t_Animation->Load(a_AnimationPath, [](League::Animation& a_Animation, void* a_Argument)
	{
		auto& t_LoadData = *(LoadData*)a_Argument;
		if (a_Animation.GetLoadState() != File::LoadState::Loaded)
		{
			if (t_LoadData.OnLoadFunction) t_LoadData.OnLoadFunction(a_Animation, t_LoadData.Argument);
			delete &t_LoadData;
			delete &a_Animation;
			return;
		}

		Instance->m_Animations.push_back(&a_Animation);
		if (t_LoadData.OnLoadFunction) t_LoadData.OnLoadFunction(a_Animation, t_LoadData.Argument);
		delete &t_LoadData;
	}, t_LoadData);
}

const Texture & Application::GetDefaultTexture() const
{
	return m_DefaultTexture;
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
	auto& t_DrawMesh = m_Meshes.back();

	if (t_DrawMesh.SubMeshes.size() == 0)
	{
		m_Window.SwapBuffers();
		return m_Window.RunFrame();
	}
	*m_MVPUniform = m_ProjectionMatrix * m_ViewMatrix * t_DrawMesh.SubMeshes[0].GetTransformMatrix();
	t_DrawMesh.Draw(0, m_Time += a_DT, m_ShaderProgram, *m_MVPUniform, m_TextureUniform ? &m_TextureUniform->Get() : nullptr, m_BoneArrayUniform ? &m_BoneArrayUniform->Get() : nullptr);

	m_Window.SwapBuffers();
	return m_Window.RunFrame();
}

int main()
{
	printf("LeagueModel Application built on %s at %s, calling new\n", __DATE__, __TIME__);
	auto* t_Application = new Application();

	printf("Initialising\n");
	Application::Instance->Init();

	printf("End of main()\n");
}