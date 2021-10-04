#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;

out vec3 Norm;
out float Depth;

const float zMin = 0.1;
const float zMax = 10.0;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 modelInvTrans;
uniform mat4 viewInvTrans;
//uniform mat4 projectionInvTrans;

void main(){
	Norm = normalize((viewInvTrans * modelInvTrans * vec4(aNorm, 0.0)).xyz);
	vec4 viewPos = view * model * vec4(aPos, 1.0);
	float dep = clamp(-viewPos.z, zMin, zMax);
	Depth = (dep - zMin) / (zMax - zMin);	
	gl_Position = projection * viewPos;
	
	//gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}