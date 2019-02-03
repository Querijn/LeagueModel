#pragma once

#include <renderer/opengl.hpp>

#include <glm/glm.hpp>

class ShaderProgram;

class BaseVertexBuffer {};

template<typename T>
class VertexBuffer : public BaseVertexBuffer
{
public:
	~VertexBuffer() 
	{ 
		glDeleteBuffers(1, &m_BufferID); 
	}

	void Use() { printf("Yikes! Trying to use VertexBuffer without a specialised type (%s)!\n", typeid(T).name()); throw 0; }
	void Upload(const std::vector<T>& a_Data, bool a_Dynamic)
	{
		GL(glDeleteBuffers(1, &m_BufferID));
		GL(glGenBuffers(1, &m_BufferID));
		GL(glBindBuffer(GL_ARRAY_BUFFER, m_BufferID));
		GL(glBufferData(GL_ARRAY_BUFFER, a_Data.size() * sizeof(T), a_Data.data(), a_Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));
	}

	void Upload(const T* a_Data, size_t a_Count, bool a_Dynamic)
	{
		GL(glDeleteBuffers(1, &m_BufferID));
		GL(glGenBuffers(1, &m_BufferID));
		GL(glBindBuffer(GL_ARRAY_BUFFER, m_BufferID));
		GL(glBufferData(GL_ARRAY_BUFFER, a_Count * sizeof(T), a_Data, a_Dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW));
	}
	
	void Update(const std::vector<T>& a_Data)
	{
		GL(glBindBuffer(GL_ARRAY_BUFFER, m_BufferID));
		GL(glBufferData(GL_ARRAY_BUFFER, a_Data.size() * sizeof(T), a_Data.data(), GL_DYNAMIC_DRAW));
	}

	void Update(const T* a_Data, size_t a_Count)
	{
		GL(glBindBuffer(GL_ARRAY_BUFFER, m_BufferID));
		GL(glBufferData(GL_ARRAY_BUFFER, a_Count * sizeof(T), a_Data, GL_DYNAMIC_DRAW));
	}

	friend class ShaderProgram;
protected:
	VertexBuffer(unsigned int a_AttributeID) : m_AttributeID(a_AttributeID) { }

private:
	VertexBuffer(const VertexBuffer& a_Copy) = delete;

	unsigned int m_BufferID;
	unsigned int m_AttributeID;
};

template<> inline void VertexBuffer<glm::vec2>::Use()
{
	glEnableVertexAttribArray(m_AttributeID);
	glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
	glVertexAttribPointer(m_AttributeID, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
}

template<> inline void VertexBuffer<glm::vec3>::Use()
{
	glEnableVertexAttribArray(m_AttributeID);
	glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
	glVertexAttribPointer(m_AttributeID, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
}

template<> inline void VertexBuffer<glm::vec4>::Use()
{
	glEnableVertexAttribArray(m_AttributeID);
	glBindBuffer(GL_ARRAY_BUFFER, m_BufferID);
	glVertexAttribPointer(m_AttributeID, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
}