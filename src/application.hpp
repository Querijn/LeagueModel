#pragma once

#include <renderer.hpp>
#include <window.hpp>
#include <event_handler/events.hpp>
#include <league/skeleton.hpp>
#include <league/animation.hpp>
#include <application_mesh.hpp>

#include <glm/glm.hpp>

#include <memory>

class Application
{
public:
	using Mesh = ApplicationMesh;
	static Application* Instance;

	Application();
	~Application();
	
	void Init();

	void LoadSkin(std::string a_BinPath, std::string a_AnimationBinPath);

	void LoadAnimation(Application::Mesh& a_Mesh, std::string a_AnimationPath, League::Animation::OnLoadFunction a_OnLoadFunction = nullptr, void* a_UserData = nullptr);

	const Texture& GetDefaultTexture() const;

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

	using OnMeshLoadFunction = void(*)(std::string a_SkinPath, std::string a_SkeletonPath, Application::Mesh* a_Mesh, void* a_UserData);
	void LoadMesh(std::string a_SkinPath, std::string a_SkeletonPath, OnMeshLoadFunction a_OnLoadFunction = nullptr, void* a_UserData = nullptr);

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

	std::vector<Mesh> m_Meshes;
	std::vector<League::Animation*> m_Animations;

	bool m_MouseIsDown = false;
	glm::vec2 m_MousePosition;

	float m_Time = 0.0f;
};