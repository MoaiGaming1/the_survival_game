#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 mvp;
uniform mat4 model;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceMatrix;

out vec4 fragPosLightSpace;
out vec3 fragNormal;
out vec3 fragPos;

void main() {
	fragNormal = normalize(normalMatrix * aNormal);
	fragPos = vec3(model * vec4(aPos, 1.0));
	fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
	gl_Position = mvp * vec4(aPos, 1.0);
}
