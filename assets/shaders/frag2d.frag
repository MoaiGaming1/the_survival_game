#version 330 core

out vec4 FragColor;

in vec3 TexCoord;

uniform vec4 objColor;

void main() {
	FragColor = objColor; // TODO: implement texcoord and textures
}