#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;

out vec3 WSNorm;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 modelInvTrans;
uniform mat4 viewInvTrans;
//uniform mat4 projectionInvTrans;

void main(){
	WSNorm = normalize((modelInvTrans * vec4(aNorm, 0.0)).xyz);
	vec4 viewPos = view * model * vec4(aPos, 1.0);
	vec4 screenPos = projection * viewPos;	
	gl_Position = screenPos;
}