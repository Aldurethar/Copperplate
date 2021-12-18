#version 460 core
out vec2 Gradient;

in vec2 texcoord; // current pixel coord

#define F 3.0 // used to define the window size (sigma*F)
#define RESOL 2.0 // multisampling (increase resolution)
#define PI 3.14159265359
#define TWOPI 6.28318531

uniform sampler2D shading;
uniform float sigma;

float halfsize = ceil(F*sigma); // kernel halfsize (in pixels)
const float stepSize = 1.0/RESOL; // step size for sampling (in pixels)
const float eps = 1e-8;

vec2 pixelSize = 1./vec2(textureSize(shading,0)); // pixel size 

float weight(vec2 offset) {
	float dist = length(offset);
	return exp(-(dist * dist) / (2 * sigma * sigma));
}

void main() {
	vec2 pos = texcoord;
	float gx = 0.0;
	float gy = 0.0;
	for(float i = -halfsize; i <= halfsize; i += stepSize){
		for(float j = -halfsize; j <= halfsize; j += stepSize){
			vec2 offset = vec2(i,j);
			float w = weight(offset);
			vec2 currPos = pos + (offset * pixelSize);
			float brightness = length(texture(shading, currPos).xyz);

			float s = abs(i) > eps ? sign(i) : 0.0;
			gx += (w * s * brightness);

			s = abs(j) > eps ? sign(j) : 0.0;
			gy += (w * s * brightness);
		}
	}
	Gradient = normalize(vec2(gx, gy));
}