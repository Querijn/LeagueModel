#version 120

attribute vec3 v_Positions;

uniform mat4 u_MVP;

void main()
{
	gl_Position = u_MVP * vec4(v_Positions, 1);
}