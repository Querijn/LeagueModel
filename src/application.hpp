#pragma once

#include <renderer.hpp>
#include <window.hpp>
#include <event_handler/events.hpp>
#include <league_model/skeleton.hpp>
#include <league_model/animation.hpp>
#include <application_mesh.hpp>

#include <glm/glm.hpp>

#include <memory>

struct MeshLoadData;
class Application
{
public:
	using Mesh = ApplicationMesh;
	using OnMeshLoadFunction = void(*)(const std::string& a_SkinPath, const std::string& a_SkeletonPath, Application::Mesh* a_Mesh, League::Skin& a_Skin, void* a_UserData);
	static Application* Instance;

	Application(const char* a_Root);
	~Application();
	
	void Init();

	void LoadMesh(const std::string& a_SknPath, const std::string& a_SkeletonPath);
	void LoadSkin(const std::string& a_BinPath, const std::string& a_AnimationBinPath);

	void LoadAnimation(Application::Mesh& a_Mesh, const std::string& a_AnimationPath, League::Animation::OnLoadFunction a_OnLoadFunction = nullptr, void* a_UserData = nullptr);
	void AddAnimationReference(const std::string& a_AnimationName);

	const Texture& GetDefaultTexture() const;

	Mesh* GetMeshUnsafe(const std::string& a_Name);

	const Mesh* GetMesh(const std::string& a_Name) const;
	const std::vector<std::string>& GetAnimations() const;

	std::vector<std::string> GetSkinFiles() const;

	void SetAssetRoot(const std::string& a_AssetRoot);
	const std::string& GetAssetRoot() const;

private:
	void OnMouseDownEvent(const MouseDownEvent* a_Event);
	void OnMouseUp(const MouseUpEvent* a_Event);
	void OnPointerDownEvent(const PointerDownEvent* a_Event);
	void OnPointerUpEvent(const PointerUpEvent* a_Event);
	void OnMouseScrollEvent(const MouseScrollEvent* a_Event);
	void OnPointerMoveEvent(const PointerMoveEvent* a_Event);

	void LoadDefaultTexture();
	void LoadShaders();
	void LoadShaderVariables();

	void UpdateViewMatrix();

	bool Update(double a_DT);

	void LoadMesh(const std::string& a_SkinPath, const std::string& a_SkeletonPath, OnMeshLoadFunction a_OnLoadFunction, void* a_UserData = nullptr);
	void OnMeshLoad(MeshLoadData& a_LoadData);

	Window m_Window;

	Texture m_DefaultTexture;

	ShaderProgram m_ShaderProgram;
	Shader m_VertexShader;
	Shader m_FragmentShader;

	ShaderVariable<glm::mat4>* m_MVPUniform = nullptr;
	ShaderVariable<std::vector<glm::mat4>>* m_BoneArrayUniform = nullptr;
	ShaderVariable<Texture>* m_TextureUniform = nullptr;

	glm::mat4 m_ModelMatrix;
	glm::mat4 m_ViewMatrix;
	glm::mat4 m_ProjectionMatrix;

	glm::vec3 m_CameraPosition = glm::vec3(0, 0, 1);
	float m_CameraDistance = 1000;

	std::map<std::string, Mesh> m_Meshes;
	std::vector<std::string> m_AvailableAnimations;

	bool m_MouseIsDown = false;
	glm::vec2 m_MousePosition;
	std::string m_Root;

	float m_Time = 0.0f;
};