#pragma once

#include <renderer/opengl.hpp>
#include <file_system.hpp>

class Shader
{
public:
	using OnLoadFunction = std::function<void(Shader* a_Shader, BaseFile::LoadState a_LoadState)>;
	enum Type
	{
		Vertex = GL_VERTEX_SHADER,
		Fragment = GL_FRAGMENT_SHADER
	};

	Shader(Type a_Type);
	~Shader();

	void Load(const std::string& a_FilePath, OnLoadFunction a_OnLoadFunction);

	operator GLuint() const;

	void Delete();

private:
	Type m_Type;
	GLuint m_ID = 0;
};