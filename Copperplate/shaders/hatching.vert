#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTangent;

out vec2 Norm;

void main(){	
	Norm = normalize(vec2(-aTangent.y, aTangent.x));
	vec2 pos = (aPos * 2.0) - 1.0;
	gl_Position = vec4(pos, 0.0, 1.0);
	
	//gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}