#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;

out VS_OUT {
	vec3 norm;
	vec3 worldPos;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
	vs_out.norm = normalize((model * vec4(aNorm.x, aNorm.y, aNorm.z, 0.0f)).xyz);
	vs_out.worldPos = (model * vec4(aPos, 1.0f)).xyz;
	vec4 testPos = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0f);
	gl_Position = testPos;
	
	//gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}