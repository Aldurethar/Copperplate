#version 460 core
layout(location = 0) in vec3 aPos;

out float Depth;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main(){
	vec4 viewPos = view * model * vec4(aPos, 1.0);
	vec4 screenPos = projection * viewPos;	
	Depth = screenPos.z * 0.5 + 0.5;
	gl_Position = screenPos;
	
	//gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}