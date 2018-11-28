#pragma once

#include <renderer/opengl.hpp>
#include <renderer/texture.hpp>

#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

class ShaderProgram;
class BaseShaderVariable 
{
public:
	enum Type
	{
		Undefined,
		Uniform,
		Attribute
	};

	virtual ~BaseShaderVariable() {}

	bool IsDirty() const { return m_Dirty; }

	friend class ShaderProgram;
protected: 
	Type m_Type;

	bool m_Dirty = true;
	virtual void Update() = 0; 
};

template<typename T>
class ShaderVariable : public BaseShaderVariable
{
public:
	~ShaderVariable() {}

	T& Get() { m_Dirty = true; return m_Value; }
	operator T&() { m_Dirty = true; return m_Value; }
	operator T() const { return m_Value; }
	T* operator->() { m_Dirty = true; return &m_Value; }

	bool operator==(const T a_Value) const { return m_Value == a_Value; }
	bool operator==(const ShaderVariable<T>& a_Variable) const { return m_Value == a_Variable.m_Value; }

	bool operator!=(const T a_Value) const { return m_Value != a_Value; }
	bool operator!=(const ShaderVariable<T>& a_Variable) const { return m_Value != a_Variable.m_Value; }

	ShaderVariable<T>& operator=(const T& a_Value) { m_Value = a_Value; m_Dirty = true; return *this; }
		
#define ARITHMETIC template<typename = std::enable_if<std::is_arithmetic<char*>::value>>

	ARITHMETIC T& operator+(const T& a_Value) { return m_Value + a_Value; }
	ARITHMETIC T& operator-(const T& a_Value) { return m_Value - a_Value; }
	ARITHMETIC T& operator*(const T& a_Value) { return m_Value * a_Value; }
	ARITHMETIC T& operator/(const T& a_Value) { return m_Value / a_Value; }

	ARITHMETIC ShaderVariable<T>& operator+=(const T& a_Value) { m_Value += a_Value; m_Dirty = true; return *this; }
	ARITHMETIC ShaderVariable<T>& operator-=(const T& a_Value) { m_Value -= a_Value; m_Dirty = true; return *this; }
	ARITHMETIC ShaderVariable<T>& operator*=(const T& a_Value) { m_Value *= a_Value; m_Dirty = true; return *this; }
	ARITHMETIC ShaderVariable<T>& operator/=(const T& a_Value) { m_Value /= a_Value; m_Dirty = true; return *this; }
			
	friend class ShaderProgram;
protected:
	ShaderVariable(const int a_Location, const T& a_Value = T()) :
		m_Location(a_Location), m_Value(a_Value)
	{ }

	virtual void Update() override { assert(false); }

private:
	int m_Location = -1;
	T m_Value;
};

template<> inline void ShaderVariable<float>::Update()
{
	GL(glUniform1f(m_Location, m_Value));
}

template<> inline void ShaderVariable<std::vector<float>>::Update()
{
	GL(glUniform1fv(m_Location, m_Value.size(), m_Value.data()));
}

template<> inline void ShaderVariable<int>::Update()
{
	GL(glUniform1i(m_Location, m_Value));
}

template<> inline void ShaderVariable<std::vector<int>>::Update()
{
	GL(glUniform1iv(m_Location, m_Value.size(), m_Value.data()));
}

#if defined(NATIVE) // TODO: Add check for using webgl2?
template<> inline void ShaderVariable<unsigned int>::Update()
{
	GL(glUniform1ui(m_Location, m_Value));
}
#endif

template<> inline void ShaderVariable<glm::vec2>::Update()
{
	GL(glUniform2fv(m_Location, 1, glm::value_ptr(m_Value)));
}

template<> inline void ShaderVariable<glm::vec3>::Update()
{
	GL(glUniform3fv(m_Location, 1, glm::value_ptr(m_Value)));
}

template<> inline void ShaderVariable<glm::vec4>::Update()
{
	GL(glUniform4fv(m_Location, 1, glm::value_ptr(m_Value)));
}

template<> inline void ShaderVariable<glm::mat3>::Update()
{
	GL(glUniformMatrix3fv(m_Location, 1, GL_FALSE, glm::value_ptr(m_Value)));
}

template<> inline void ShaderVariable<glm::mat4>::Update()
{
	GL(glUniformMatrix4fv(m_Location, 1, GL_FALSE, glm::value_ptr(m_Value)));
}

template<> inline void ShaderVariable<std::vector<glm::vec2>>::Update()
{
	if (m_Value.size() == 0) return;
	GL(glUniform2fv(m_Location, m_Value.size(), (float*)&m_Value[0]));
}

template<> inline void ShaderVariable<std::vector<glm::vec3>>::Update()
{
	if (m_Value.size() == 0) return;
	GL(glUniform3fv(m_Location, m_Value.size(), (float*)&m_Value[0]));
}

template<> inline void ShaderVariable<std::vector<glm::vec4>>::Update()
{
	if (m_Value.size() == 0) return;
	GL(glUniform4fv(m_Location, m_Value.size(), (float*)&m_Value[0]));
}

template<> inline void ShaderVariable<std::vector<glm::mat3>>::Update()
{
	if (m_Value.size() == 0) return;
	GL(glUniformMatrix3fv(m_Location, m_Value.size(), GL_FALSE, (float*)&m_Value[0]));
}

template<> inline void ShaderVariable<std::vector<glm::mat4>>::Update()
{
	if (m_Value.size() == 0) return;
	GL(glUniformMatrix4fv(m_Location, m_Value.size(), GL_FALSE, (float*)&m_Value[0]));
}

template<> inline void ShaderVariable<Texture>::Update()
{
	auto t_Position = m_Value.GetPosition();

	GL(glActiveTexture(GL_TEXTURE0 + t_Position));
	GL(glBindTexture(GL_TEXTURE_2D, m_Value));
	GL(glUniform1i(m_Location, t_Position));
}

// TODO: Fix char* in here, needs to be T?
#define ARITHMETIC_GLOBAL template<typename T, typename = std::enable_if<std::is_arithmetic<char*>::value>>

ARITHMETIC_GLOBAL T operator+(const T& a_Value, const ShaderVariable<T>& a_ShaderVariable) { return a_Value + (T)a_ShaderVariable; }
ARITHMETIC_GLOBAL T operator-(const T& a_Value, const ShaderVariable<T>& a_ShaderVariable) { return a_Value - (T)a_ShaderVariable; }
ARITHMETIC_GLOBAL T operator*(const T& a_Value, const ShaderVariable<T>& a_ShaderVariable) { return a_Value * (T)a_ShaderVariable; }
ARITHMETIC_GLOBAL T operator/(const T& a_Value, const ShaderVariable<T>& a_ShaderVariable) { return a_Value / (T)a_ShaderVariable; }

ARITHMETIC_GLOBAL T operator+=(T& a_Value, const ShaderVariable<T>& a_ShaderVariable) { a_Value += (T)a_ShaderVariable; return a_Value; }
ARITHMETIC_GLOBAL T operator-=(T& a_Value, const ShaderVariable<T>& a_ShaderVariable) { a_Value -= (T)a_ShaderVariable; return a_Value; }
ARITHMETIC_GLOBAL T operator*=(T& a_Value, const ShaderVariable<T>& a_ShaderVariable) { a_Value *= (T)a_ShaderVariable; return a_Value; }
ARITHMETIC_GLOBAL T operator/=(T& a_Value, const ShaderVariable<T>& a_ShaderVariable) { a_Value /= (T)a_ShaderVariable; return a_Value; }