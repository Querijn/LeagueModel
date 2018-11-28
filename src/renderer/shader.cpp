#include <renderer/shader.hpp>
#include <string>

#include <cstdlib>
#include <iostream>
#include <vector>

Shader::Shader(Type a_Type) :
	m_Type(a_Type)
{
}

Shader::~Shader()
{
	Delete();
}

void Shader::Load(std::string a_FilePath, Shader::OnLoadFunction a_OnLoadFunction, void * a_Argument)
{
	auto* t_File = FileSystem::GetFile(a_FilePath);
	struct LoadData
	{
		LoadData(Shader* a_Target, OnLoadFunction a_Function, void* a_Argument) :
			Target(a_Target), OnLoadFunction(a_Function), Argument(a_Argument)
		{}

		Shader* Target;
		OnLoadFunction OnLoadFunction;
		void* Argument;
	};
	auto* t_LoadData = new LoadData(this, a_OnLoadFunction, a_Argument);

	t_File->Load([](File* a_File, File::LoadState a_LoadState, void* a_Arg)
	{
		auto* t_LoadData = (LoadData*)a_Arg;
		auto* t_Shader = (Shader*)t_LoadData->Target;

		if (a_LoadState != File::LoadState::Loaded)
		{
			t_Shader->m_State = a_LoadState;
			if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(t_Shader, t_LoadData->Argument);
			return;
		}

		switch (t_Shader->m_Type)
		{
		case Type::Vertex:
			t_Shader->m_ID = GL_RET(glCreateShader(GL_VERTEX_SHADER), GLuint);
			break;
		case Type::Fragment:
			t_Shader->m_ID = GL_RET(glCreateShader(GL_FRAGMENT_SHADER), GLuint);
			break;
		};

		auto t_Data = a_File->GetData();
		char* t_DataPointer = reinterpret_cast<char*>(t_Data.data());
		
		GLint t_Size = (GLint)t_Data.size();
		GL(glShaderSource(t_Shader->m_ID, 1, &t_DataPointer, &t_Size));
		GL(glCompileShader(t_Shader->m_ID));

		GLint t_HasSucceeded;
		GLint t_LogLength;
		GL(glGetShaderiv(t_Shader->m_ID, GL_INFO_LOG_LENGTH, &t_LogLength));
		GL(glGetShaderiv(t_Shader->m_ID, GL_COMPILE_STATUS, &t_HasSucceeded));

		if (t_LogLength > 1) // 1 because for some reason WebGL likes to output a newline for a log
		{
			std::vector<GLchar> t_Log(t_LogLength);
			GL(glGetShaderInfoLog(t_Shader->m_ID, t_LogLength, 0, t_Log.data()));

			const char* t_Type = t_Shader->m_Type == Type::Vertex ? "vertex" : "fragment";

			auto t_Name = a_File->GetName();
			printf("Compilation log of %s shader '%s' (%s):\n%s\n", t_Type, t_Name.c_str(), t_HasSucceeded ? "loaded successfully" : "failed to load", t_Log.data());
		}

		t_Shader->m_State = t_HasSucceeded ? File::LoadState::Loaded : File::LoadState::FailedToLoad;
		if (t_LoadData->OnLoadFunction) t_LoadData->OnLoadFunction(t_Shader, t_LoadData->Argument);
		FileSystem::CloseFile(*a_File);
		delete t_LoadData;
	}, t_LoadData);
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

