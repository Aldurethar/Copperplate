#version 460 core
layout(triangles_adjacency) in;
layout(line_strip, max_vertices = 12) out;

in VS_OUT {
	vec3 norm;
	vec3 worldPos;
} gs_in[];

const float eps = 1e-4;

out vec2 screenPos;

uniform vec3 viewDirection;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D depthBuffer;

void drawFaceNormal(vec3 normal){
	vec4 faceMid = (gl_in[0].gl_Position + gl_in[2].gl_Position + gl_in[4].gl_Position) / 3.0;
	gl_Position = faceMid;
	EmitVertex();
	vec4 tip = faceMid + 0.2 * vec4(normal, 0.0);
	gl_Position = tip;
	EmitVertex();
	EndPrimitive();
}

void drawVertNormal(int vert){	
	gl_Position = gl_in[vert].gl_Position;
	EmitVertex();
	vec3 tip = gs_in[vert].worldPos + 0.2 * gs_in[vert].norm;
	gl_Position = projection * view * vec4(tip, 1.0);	
	EmitVertex();
	EndPrimitive();
}

void main(){

	vec3 viewDir = vec3(0.0, 0.0, -1.0);
	vec3 positions[6];
	for (int i = 0; i < 6; i++){
		positions[i] = gl_in[i].gl_Position.xyz / gl_in[i].gl_Position.w;
	}

	vec3 faceNorm = normalize(cross(positions[2] - positions[0], positions[4] - positions[0]));
	vec3 side1Norm = normalize(cross(positions[1] - positions[0], positions[2] - positions[0]));
	vec3 side2Norm = normalize(cross(positions[4] - positions[3], positions[2] - positions[3]));
	vec3 side3Norm = normalize(cross(positions[5] - positions[4], positions[0] - positions[4]));
	
	float faceFront = sign(dot(faceNorm, viewDir));
	float side1Front = sign(dot(side1Norm, viewDir));
	float side2Front = sign(dot(side2Norm, viewDir));
	float side3Front = sign(dot(side3Norm, viewDir));

	float v0Depth = gl_in[0].gl_Position.z * 0.5 + 0.5;
	float v2Depth = gl_in[2].gl_Position.z * 0.5 + 0.5;
	float v4Depth = gl_in[4].gl_Position.z * 0.5 + 0.5;

	vec2 v0ScreenPos = positions[0].xy * 0.5 + 0.5;
	vec2 v2ScreenPos = positions[2].xy * 0.5 + 0.5;
	vec2 v4ScreenPos = positions[4].xy * 0.5 + 0.5;

	bool v0Visible = (v0Depth <= (texture(depthBuffer, v0ScreenPos).r + eps) );
	bool v2Visible = (v2Depth <= (texture(depthBuffer, v2ScreenPos).r + eps) );
	bool v4Visible = (v4Depth <= (texture(depthBuffer, v4ScreenPos).r + eps) );
	
	if (faceFront > 0){
		if(side1Front < 0 && (v0Visible || v2Visible)){
			screenPos = v0ScreenPos;
			gl_Position = gl_in[0].gl_Position;
			EmitVertex();
			screenPos = v2ScreenPos;
			gl_Position = gl_in[2].gl_Position;
			EmitVertex();
			EndPrimitive();
		}
		if(side2Front < 0 && (v2Visible || v4Visible)){
			screenPos = v2ScreenPos;
			gl_Position = gl_in[2].gl_Position;
			EmitVertex();
			screenPos = v4ScreenPos;
			gl_Position = gl_in[4].gl_Position;
			EmitVertex();
			EndPrimitive();
		}
		if(side3Front < 0 && (v4Visible || v0Visible)){
			screenPos = v4ScreenPos;
			gl_Position = gl_in[4].gl_Position;
			EmitVertex();
			screenPos = v0ScreenPos;
			gl_Position = gl_in[0].gl_Position;
			EmitVertex();
			EndPrimitive();
		}
	}
	
}