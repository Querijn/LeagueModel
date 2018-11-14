#include "shader_program.hpp"

ShaderProgram::ShaderProgram()
{
}

ShaderProgram::~ShaderProgram()
{
	for (auto i = m_Variables.begin(); i != m_Variables.end(); i++)
	{
		const auto t_Variable = (*i).second;
		delete t_Variable;
	}
	m_Variables.clear();

	for (auto i = m_VertexBuffers.begin(); i != m_VertexBuffers.end(); i++)
	{
		const auto* t_VertexBuffer = *i;
		delete t_VertexBuffer;
	}
	m_VertexBuffers.clear();

	if (m_ID != (GLuint)-1)
	{
		glDeleteProgram(m_ID);
		m_ID = -1;
	}
}

void ShaderProgram::Init(const std::initializer_list<Shader>& a_Shaders)
{
	m_ID = GL_RET(glCreateProgram(), GLuint);

	for (auto i = a_Shaders.begin(); i != a_Shaders.end(); i++)
	{
		const auto& t_Shader = *i;
		GL(glAttachShader(m_ID, t_Shader));
	}

	GL(glLinkProgram(m_ID));
}

ShaderProgram::operator GLuint() const
{
	return m_ID;
}

GLuint ShaderProgram::GetID() const
{
	return m_ID;
}

void ShaderProgram::Use()
{
	GL(glUseProgram(m_ID));
}

void ShaderProgram::Update()
{
	for (auto i = m_Variables.begin(); i != m_Variables.end(); i++)
	{
		const auto& t_Variable = (*i).second;
		if (t_Variable->IsDirty() == false) return;

		t_Variable->Update();
	}
}

