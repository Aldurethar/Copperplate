#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

const float radius = 0.5;
const float PI = 3.141592;
const float HALFPI = 1.570796;
const vec2 viewPortCorrection = vec2(1.77778, 1.0); // 16:9

void main()
{	
	//vec3 center = vec3(0.0);
	vec2 screenPos = TexCoords * 2 - 1;
	screenPos = screenPos * viewPortCorrection;

	float distToCenter = length(screenPos) / radius;
	float z = cos(distToCenter * HALFPI) * radius;
	vec3 pos = vec3(screenPos, z);
	vec3 norm = normalize(pos);
	float depth = -z * 0.5 + 0.5;

	if(distToCenter > 1.0) {
		depth = 1.0;
		norm = vec3(0.0);
	}

    FragColor = vec4(norm, depth);
	//FragColor = vec4(Norm, gl_FragCoord.z);
} 