#pragma once

#include <renderer/opengl.hpp>
#include <file_system.hpp>

class Shader
{
public:
	using OnLoadFunction = void(*)(Shader* a_Shader, void* a_Argument);
	enum Type
	{
		Vertex = GL_VERTEX_SHADER,
		Fragment = GL_FRAGMENT_SHADER
	};

	Shader(Type a_Type);
	~Shader();

	void Load(std::string a_FilePath, OnLoadFunction a_OnLoadFunction = nullptr, void* a_Argument = nullptr);

	operator GLuint() const;

	void Delete();
	
	File::LoadState GetLoadState() const { return m_State; }

private:
	File::LoadState m_State = File::LoadState::NotLoaded;

	Type m_Type;
	GLuint m_ID = 0;
};