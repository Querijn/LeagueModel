@include types.glsl
@vs AnimatedMeshVS

const int MAX_BONES = 255;

uniform AnimatedMeshParametersVS
{
    mat4 mvp;
    mat4 bones[MAX_BONES];
};

in vec3 pos;
in vec2 uvs;
in vec3 normals;
in vec4 boneIndices;
in vec4 boneWeights;

out vec2 outUv;

void main()
{
    mat4 boneTransform = bones[int(boneIndices[0])] * boneWeights[0];
    boneTransform += bones[int(boneIndices[1])] * boneWeights[1];
    boneTransform += bones[int(boneIndices[2])] * boneWeights[2];
    boneTransform += bones[int(boneIndices[3])] * boneWeights[3];

    gl_Position = mvp * boneTransform * vec4(pos, 1.0);
    outUv = uvs;
}

@end

@fs AnimatedMeshFS

in vec2 outUv;

out vec4 frag_color;

uniform texture2D diffuseTexture;
uniform sampler diffuseSampler;

void main()
{
    frag_color = texture(sampler2D(diffuseTexture, diffuseSampler), outUv);
}

@end

@program AnimatedMesh AnimatedMeshVS AnimatedMeshFS

