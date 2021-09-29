#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;

out vec3 Norm;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 modelInvTrans;
uniform mat4 viewInvTrans;
//uniform mat4 projectionInvTrans;

void main(){
	Norm = normalize((viewInvTrans * modelInvTrans * vec4(aNorm, 0.0)).xyz);
	//Norm = (viewInvTrans * modelInvTrans * vec4(aNorm, 0.0)).xyz;
	vec4 testPos = projection * view * model * vec4(aPos, 1.0f);
	gl_Position = testPos;
	
	//gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}