#version 460 core
layout(triangles_adjacency) in;
layout(line_strip, max_vertices = 12) out;

in VS_OUT {
	vec3 norm;
	vec3 worldPos;
} gs_in[];

out vec3 fColor;

uniform vec3 viewDirection;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void drawFaceNormal(vec3 normal){
	//vec3 faceMid = (gs_in[0].worldPos + gs_in[2].worldPos + gs_in[4].worldPos) / 3;
	//gl_Position = projection * view * vec4(faceMid, 1.0);
	vec4 faceMid = (gl_in[0].gl_Position + gl_in[2].gl_Position + gl_in[4].gl_Position) / 3.0;
	gl_Position = faceMid;
	EmitVertex();
	//vec3 tip = faceMid + 0.2 * normal;
	//gl_Position = projection * view * vec4(tip, 1.0);	
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
		//positions[i] = gl_in[i].gl_Position.xyz;
		//positions[i] = gs_in[i].worldPos;
	}

	vec3 faceNorm = normalize(cross(positions[2] - positions[0], positions[4] - positions[0]));
	vec3 side1Norm = normalize(cross(positions[1] - positions[0], positions[2] - positions[0]));
	vec3 side2Norm = normalize(cross(positions[4] - positions[3], positions[2] - positions[3]));
	vec3 side3Norm = normalize(cross(positions[5] - positions[4], positions[0] - positions[4]));
	
	float faceFront = sign(dot(faceNorm, viewDir));
	float side1Front = sign(dot(side1Norm, viewDir));
	float side2Front = sign(dot(side2Norm, viewDir));
	float side3Front = sign(dot(side3Norm, viewDir));
	
	fColor = vec3(0.0);
	if (faceFront > 0){
		if(side1Front < 0){
			gl_Position = gl_in[0].gl_Position;
			EmitVertex();
			gl_Position = gl_in[2].gl_Position;
			EmitVertex();
			EndPrimitive();
		}
		if(side2Front < 0){
			gl_Position = gl_in[2].gl_Position;
			EmitVertex();
			gl_Position = gl_in[4].gl_Position;
			EmitVertex();
			EndPrimitive();
		}
		if(side3Front < 0){
			gl_Position = gl_in[4].gl_Position;
			EmitVertex();
			gl_Position = gl_in[0].gl_Position;
			EmitVertex();
			EndPrimitive();
		}
	}
	/*fColor = vec3(0.0);	
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = gl_in[2].gl_Position;
	EmitVertex();
	gl_Position = gl_in[4].gl_Position;
	EmitVertex();
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();
	EndPrimitive();	*/

	/*if(faceFront > 0) fColor = vec3(1.0, 0.0, 0.0);
	else fColor = vec3(0.0, 0.0, 1.0);
	drawFaceNormal(faceNorm);

	fColor = vec3(1.0, 1.0, 1.0);
	drawVertNormal(0);
	drawVertNormal(2);
	drawVertNormal(4);*/
	

	/*fColor = vec3(1.0, 1.0, 1.0);
	//vec4 faceMid = (gl_in[0].gl_Position + gl_in[2].gl_Position + gl_in[4].gl_Position) / 3;
	vec3 faceMid = (gs_in[0].worldPos + gs_in[2].worldPos + gs_in[4].worldPos) / 3;
	gl_Position = projection * view * vec4(faceMid, 1.0);
	//gl_Position = faceMid;
	EmitVertex();
	vec3 tip = faceMid + 0.2 * faceNorm;
	gl_Position = projection * view * vec4(tip, 1.0);
	//gl_Position = faceMid + 0.2 * vec4(faceNorm, 0.0);
	EmitVertex();
	EndPrimitive();*/

	/*fColor = vec3(1.0, 0.0, 0.0);
	vec4 side1Mid = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) / 3;
	gl_Position = side1Mid;
	EmitVertex();
	gl_Position = side1Mid + 0.2 * vec4(side1Norm, 0.0);
	EmitVertex();
	EndPrimitive();*/

	/*fColor = vec3(1.0, 0.0, 1.0);
	vec4 side2Mid = (gl_in[2].gl_Position + gl_in[3].gl_Position + gl_in[4].gl_Position) / 3;
	gl_Position = side2Mid;
	EmitVertex();
	gl_Position = side2Mid + 0.2 * vec4(side2Norm, 0.0);
	EmitVertex();
	EndPrimitive();*/

	/*fColor = vec3(0.0, 0.0, 1.0);
	vec4 side3Mid = (gl_in[4].gl_Position + gl_in[5].gl_Position + gl_in[0].gl_Position) / 3;
	gl_Position = side3Mid;
	EmitVertex();
	gl_Position = side3Mid + 0.2 * vec4(side3Norm, 0.0);
	EmitVertex();
	EndPrimitive();*/


	//float faceFront = sign(dot(faceNorm, viewDirection));

	//vec3 side1Norm = normalize(gs_in[0].norm + gs_in[1].norm + gs_in[2].norm);
	//float side1Front = sign(dot(side1Norm, viewDirection));

	//vec3 side2Norm = normalize(gs_in[2].norm + gs_in[3].norm + gs_in[4].norm);
	//float side2Front = sign(dot(side2Norm, viewDirection));

	//vec3 side3Norm = normalize(gs_in[4].norm + gs_in[5].norm + gs_in[0].norm);
	//float side3Front = sign(dot(side3Norm, viewDirection));

	/*for (int i = 0; i < 6; i++){
		if(i % 2 == 0) fColor = vec3(0.0);
		else if (i == 1) fColor = vec3(1.0, 0.0, 0.0);
		else if (i == 3) fColor = vec3(0.0, 1.0, 0.0);
		else if (i == 5) fColor = vec3(0.0, 0.0, 1.0);
		gl_Position = gl_in[i].gl_Position;
		EmitVertex();
		gl_Position = projection * view * vec4((gs_in[i].worldPos + 0.2 * gs_in[i].norm), 1.0);
		EmitVertex();
		EndPrimitive();
	}*/


	//fColor = vec3(0.0, 0.0, 0.0);
	//fColor = vec3(dot(viewDirection, faceNorm));
	/*if (faceFront > 0){		
		//fColor = vec3(0.0, 0.0, 0.0);
		fColor = normalize(faceNorm - side1Norm);
		gl_Position = gl_in[0].gl_Position;
		EmitVertex();
		gl_Position = gl_in[2].gl_Position;
		EmitVertex();
		gl_Position = gl_in[4].gl_Position;
		EmitVertex();
		gl_Position = gl_in[0].gl_Position;
		EmitVertex();
		EndPrimitive();	
		/*if(side1Front <= 0){
			fColor = vec3(1.0, 0.0, 0.0);
			gl_Position = gl_in[0].gl_Position;
			EmitVertex();
			gl_Position = gl_in[2].gl_Position;
			EmitVertex();
			EndPrimitive();
		}
	}*/
	
	
	/*if(faceFront != side1Front){
		gl_Position = gl_in[0].gl_Position;
		EmitVertex();
		gl_Position = gl_in[2].gl_Position;
		EmitVertex();
		EndPrimitive();
	}

	if(faceFront != side2Front){
		gl_Position = gl_in[2].gl_Position;
		EmitVertex();
		gl_Position = gl_in[4].gl_Position;
		EmitVertex();
		EndPrimitive();
	}

	if(faceFront != side3Front){
		gl_Position = gl_in[4].gl_Position;
		EmitVertex();
		gl_Position = gl_in[0].gl_Position;
		EmitVertex();
		EndPrimitive();
	}*/

}