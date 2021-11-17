#version 460 core
layout(location = 0) in vec3 aPos;

out vec2 Movement;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 prevModel;
uniform mat4 prevView;

void main(){
	vec4 oldClipPos = projection * prevView * prevModel * vec4(aPos, 1.0);
	vec4 newClipPos = projection * view * model * vec4(aPos, 1.0);

	vec2 oldScreenPos = (oldClipPos.xy / oldClipPos.w) * 0.5 + 0.5;
	vec2 newScreenPos = (newClipPos.xy / newClipPos.w) * 0.5 + 0.5;

	Movement = newScreenPos - oldScreenPos;	
	gl_Position = oldClipPos;
	
}