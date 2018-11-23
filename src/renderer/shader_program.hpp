#pragma once

#include <renderer/shader.hpp>
#include <renderer/shader_variable.hpp>
#include <renderer/vertex_buffer.hpp>
#include <renderer/index_buffer.hpp>

#include <initializer_list>
#include <map>
#include <iostream>

class ShaderProgram
{
public:
	ShaderProgram();
	~ShaderProgram();

	operator GLuint() const;
	GLuint GetID() const;

	void Init(const std::initializer_list<Shader>& a_Shaders);
	void Use();
	void Update();

	template<typename T>
	ShaderVariable<T>* GetVariable(String a_Name, const T& a_Value = T())
	{
		if (m_Variables.find(a_Name) == m_Variables.end())
		{
			auto t_Name = a_Name.Get();
			GLint t_Reference = GL_RET(glGetUniformLocation(m_ID, t_Name), GLint);

			if (t_Reference < 0)
				return nullptr;

			m_Variables[a_Name] = new ShaderVariable<T>(t_Reference, a_Value);
		}

		return (ShaderVariable<T>*)(m_Variables[a_Name]);
	}

	template<typename T>
	VertexBuffer<T>* GetVertexBuffer(String a_Name)
	{
		auto t_String = a_Name.Get();
		GLint t_Reference = GL_RET(glGetAttribLocation(m_ID, t_String), GLint);
		if (t_Reference < 0) return nullptr;

		auto* t_Buffer = new VertexBuffer<T>(t_Reference);
		m_VertexBuffers.push_back((BaseVertexBuffer*)t_Buffer);
		return t_Buffer;
	}

	template<typename T>
	IndexBuffer<T>& GetIndexBuffer()
	{
		auto* t_Buffer = new IndexBuffer<T>();
		m_IndexBuffers.push_back((BaseIndexBuffer*)t_Buffer);
		return *t_Buffer;
	}

private:
	GLuint m_ID = 0;
	std::map<String, BaseShaderVariable*, StringCompare> m_Variables;
	std::vector<BaseVertexBuffer*> m_VertexBuffers;
	std::vector<BaseIndexBuffer*> m_IndexBuffers;
};