#pragma once

#include <renderer/opengl.hpp>

#include <glm/glm.hpp>

class ShaderProgram;

class BaseIndexBuffer {};

template<typename T>
class IndexBuffer : public BaseIndexBuffer
{
public:
	~IndexBuffer() 
	{ 
		glDeleteBuffers(1, &m_ID); 
	}

	void Upload(const std::vector<T>& a_Data)
	{
		m_ElementCount = a_Data.size();
		m_Type = GetType();

		glGenBuffers(1, &m_ID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_ElementCount * sizeof(T), a_Data.data(), GL_STATIC_DRAW);
	}

	void Upload(const T* a_Data, size_t a_ElementCount)
	{
		m_ElementCount = a_ElementCount;
		m_Type = GetType();

		glGenBuffers(1, &m_ID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_ElementCount * sizeof(T), a_Data, GL_STATIC_DRAW);
	}

	size_t GetType() const { printf("Seems like something went wrong getting the tpye of this index buffer!\n"); throw 0; return GL_UNSIGNED_SHORT; }

	void Draw()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
		glDrawElements(GL_TRIANGLES, m_ElementCount, m_Type, nullptr);
	}

	friend class ShaderProgram;
protected:
	IndexBuffer() { }

private:
	IndexBuffer(const IndexBuffer& a_Copy) = delete;

	unsigned int m_ID;
	size_t m_ElementCount;
	size_t m_Type;
};

template<> inline size_t IndexBuffer<uint8_t>::GetType() const
{
	return GL_UNSIGNED_BYTE;
}

template<> inline size_t IndexBuffer<int8_t>::GetType() const
{
	return GL_BYTE;
}

template<> inline size_t IndexBuffer<uint16_t>::GetType() const
{
	return GL_UNSIGNED_SHORT;
}

template<> inline size_t IndexBuffer<int16_t>::GetType() const
{
	return GL_SHORT;
}

template<> inline size_t IndexBuffer<uint32_t>::GetType() const
{
	return GL_UNSIGNED_INT;
}

template<> inline size_t IndexBuffer<int32_t>::GetType() const
{
	return GL_INT;
}