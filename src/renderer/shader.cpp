#include <renderer/shader.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>

Shader::Shader(Type a_Type) :
	m_Type(a_Type)
{
}

void Shader::Load(const std::string & a_FilePath, OnLoadFunction a_OnLoadFunction)
{
	FileSystem t_FileSystem;
	t_FileSystem.OpenFile(a_FilePath, [&](BaseFile* a_File, BaseFile::LoadState a_LoadState)
	{
		if (a_LoadState != BaseFile::LoadState::Loaded)
		{
			if (a_OnLoadFunction) a_OnLoadFunction(nullptr, a_LoadState);
			return;
		}

		auto t_Data = a_File->Data();

		switch (m_Type)
		{
		case Type::Vertex:
			m_ID = GL_RET(glCreateShader(GL_VERTEX_SHADER), GLuint);
			break;
		case Type::Fragment:
			m_ID = GL_RET(glCreateShader(GL_FRAGMENT_SHADER), GLuint);
			break;
		};
		
		char* t_DataPointer = reinterpret_cast<char*>(t_Data.data());
		GLint t_Size = (GLint)t_Data.size();
		GL(glShaderSource(m_ID, 1, &t_DataPointer, &t_Size));
		GL(glCompileShader(m_ID));

		GLint t_HasSucceeded;
		GLint t_LogLength;
		GL(glGetShaderiv(m_ID, GL_INFO_LOG_LENGTH, &t_LogLength));
		GL(glGetShaderiv(m_ID, GL_COMPILE_STATUS, &t_HasSucceeded));

		if (t_LogLength > 0)
		{
			std::vector<GLchar> t_Log(t_LogLength);
			GL(glGetShaderInfoLog(m_ID, t_LogLength, 0, t_Log.data()));

			const char* t_Type = m_Type == Type::Vertex ? "vertex" : "fragment";

			printf("Log for compilation of %s shader:\n%s\n", t_Type, t_Log.data());
		}

		if (a_OnLoadFunction) a_OnLoadFunction(this, t_HasSucceeded ? BaseFile::LoadState::Loaded : BaseFile::LoadState::FailedToLoad);
	});
}

Shader::~Shader()
{
	Delete();
}

void Shader::Delete()
{
	if (m_ID == (GLuint)-1) return;

	GL(glDeleteShader(m_ID));
	m_ID = (GLuint)-1;
}

Shader::operator GLuint() const
{
	return m_ID;
}

