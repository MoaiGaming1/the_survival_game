#version 330 core

#define MAX_POINT_LIGHTS 32

struct PointLight {
	vec3 position;
	vec3 color;
	float intensity;
	float radius;
};

out vec4 FragColor;

in vec3 fragNormal;
in vec3 fragPos;
in vec4 fragPosLightSpace;

uniform sampler2D shadowMap;

uniform vec4 objColor;

uniform float ambientStrength;

uniform vec3 sunDirection;
uniform float sunIntensity;
uniform vec4 sunColor;

uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights;

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragPos) {
	vec3 lightVec = light.position - fragPos;
	float dist = length(lightVec);

	if (light.radius > 0.0 && dist > light.radius) {
		return vec3(0.0);
	}

	vec3 lightDir = lightVec / dist;
	float diff = max(dot(normal, lightDir), 0.0);

	float effectiveIntensity;
	if (light.radius > 0.0) {
		float falloff = clamp(1.0 - (dist / light.radius), 0.0, 1.0);
		effectiveIntensity = light.radius * light.intensity * falloff;
	} else {
		float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
		effectiveIntensity = light.intensity * attenuation;
	}

	return diff * effectiveIntensity * light.color;
}

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;

	if (projCoords.z > 1.0) return 0.0;

	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;

	float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for(int x = -1; x <= 1; ++x) {
		for(int y = -1; y <= 1; ++y) {
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
		}
	}
	shadow /= 9.0;

	return shadow;
}

void main() {
	vec3 normal = normalize(fragNormal);
	vec3 lightDir = normalize(-sunDirection);

	float diff = max(dot(normal, lightDir), 0.0);
	float shadow = calculateShadow(fragPosLightSpace, normal, lightDir);
	
	vec3 ambient = ambientStrength * sunColor.rgb;
	vec3 diffuse = (1.0 - shadow) * diff * sunIntensity * sunColor.rgb;

	vec3 lighting = ambient + diffuse;

	for (int i = 0; i < numPointLights; i++) {
		vec3 x = calculatePointLight(pointLights[i], normal, fragPos);
		lighting += x;
	}

	FragColor = vec4(objColor.rgb * lighting, objColor.a);
}
