#version 120

attribute vec3 v_Positions;
attribute vec2 v_UVs;
attribute vec3 v_Normals;
attribute vec4 v_BoneIndices;
attribute vec4 v_BoneWeights;

varying vec2 UV;

const int MAX_BONES = 255;

uniform mat4 u_MVP;
uniform mat4 u_Bones[MAX_BONES];

void main()
{
    mat4 BoneTransform = u_Bones[int(v_BoneIndices[0])] * v_BoneWeights[0];
	BoneTransform += u_Bones[int(v_BoneIndices[1])] * v_BoneWeights[1];
    BoneTransform += u_Bones[int(v_BoneIndices[2])] * v_BoneWeights[2];
    BoneTransform += u_Bones[int(v_BoneIndices[3])] * v_BoneWeights[3];

	gl_Position = u_MVP * BoneTransform * vec4(v_Positions, 1.0);
	UV = v_UVs;
}