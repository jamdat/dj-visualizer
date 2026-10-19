#version 330 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inBarycentric;

uniform mat4 u_MVP;
uniform mat4 u_Model;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec3 vBarycentric;

void main() {
    vec4 worldPos = u_Model * vec4(inPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(mat3(u_Model) * inNormal);
    vUV = inTexCoord;
    vBarycentric = inBarycentric;

    gl_Position = u_MVP * vec4(inPosition, 1.0);
}
