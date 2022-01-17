#version 460 core
layout(lines) in;
layout(triangle_strip, max_vertices = 6) out;

in vec2 Norm[];

uniform float minLineWidth;
uniform float maxLineWidth;
uniform float minShade;
uniform float maxShade;

uniform sampler2D shading;

vec2 pixelSize = vec2(1.0) / vec2(textureSize(shading, 0));

float width(float shade){
	float s = clamp((shade - minShade) / (maxShade - minShade), 0.0, 1.0);
	return minLineWidth + (s * (maxLineWidth - minLineWidth));
}

void emit(vec2 vertPos){
	gl_Position = vec4(vertPos, 0.0, 1.0);
	EmitVertex();
}

void main(){	
	vec2 pos1 = gl_in[0].gl_Position.xy;
	vec2 pos2 = gl_in[1].gl_Position.xy;
	vec2 samplePos1 = pos1 * 0.5 + 0.5;
	vec2 samplePos2 = pos2 * 0.5 + 0.5;
	float shade1 = 1.0 - length(texture(shading, samplePos1).x);
	float shade2 = 1.0 - length(texture(shading, samplePos2).x);
	float offset1 = width(shade1) * 0.5;
	float offset2 = width(shade2) * 0.5;

	vec2 vert0Pos = pos1 + (offset1 * (Norm[0] * pixelSize));
	vec2 vert1Pos = pos1 - (offset1 * (Norm[0] * pixelSize));
	vec2 vert2Pos = pos2 + (offset2 * (Norm[1] * pixelSize));
	vec2 vert3Pos = pos2 - (offset2 * (Norm[1] * pixelSize));
		
	if(shade1 > minShade || shade2 > minShade){
		emit(vert0Pos);
		emit(vert2Pos);
		emit(vert1Pos);
		EndPrimitive();

		emit(vert3Pos);
		emit(vert1Pos);
		emit(vert2Pos);
		EndPrimitive();
	}
}