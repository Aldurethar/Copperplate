#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

#define F 3.0 // used to define the window size (sigma*F)
#define RESOL 2.0 // resolution for multisampling (steps per pixel)
#define PI 3.14159265359
#define TWOPI 6.28318531

uniform sampler2D normalMap; // input normals in camera space (stored in rgb) + depth (stored in a)
uniform float sigma; // the scale at which you want to compute curvature (keep it low or it will be slow) 

float halfsize = ceil(F*sigma); // kernel halfsize (in pixels)
const float stepSize = 1.0/RESOL; // step size for sampling (in pixels)
const float eps = 1e-15;

vec2 pixelSize = 1./vec2(textureSize(normalMap,0)); // pixel size 

/*  
* Computes the gradient at a given pixel.
*/
vec2 grad(in vec4 norm) {
	const float f = 0.5; // forshortening in [0,1]. 1=a lot (true gradient), 0 = no forshortening. 
	float d = -1.0 / mix(1.0, max(norm.z,eps), f);
	return vec2(norm.x,norm.y)*d;
}

/*
* Computes a weight based on the difference in depth of two pixels.
* Bigger depth difference means a smaller weight, based on a gaussian function.
*/
float weight(in vec4 currNorm,in vec4 otherNorm) {
	const float sd = 0.1; // sigma for checking differences in depth
	float d = otherNorm.w - currNorm.w;
	return min(length(currNorm.xyz), exp(-(d * d) / (2.0 * sd * sd)));
}

/*
* Interpolates between two gradients.
*/
vec2 mixedGrad(in vec2 gCenter,in vec2 gCurrent,in float w) {
	// takes weight into account to compute current value
	return mix(gCenter,gCurrent,w);
}

/*
* Computes the Hessian Matrix at a given pixel.
* The Hessian Matrix contains the partial derivatives of the gradient in each of the directions.
* For our 2D case, the Matrix would have the form: (gxx  gxy 
*													gyx  gyy)
* However, gxy = gyx, so we can store it as a vec3(gxx, gyy, gxy).
*/
vec3 hessianMatrix(in vec4 pixel) {
	// precompute gaussian data (factor/denom)
	float sigma2 = sigma*sigma;
	float sigma4 = sigma2*sigma2;
	float gaussFac = -1./(TWOPI*sigma4);
	float gaussDenom = 2.0*sigma2;

	vec3 H = vec3(0.);
	vec2 centerGrad = grad(pixel); // gradient of the current pixel
	float pixelWeight = weight(pixel,pixel);	// weight of the current pixel, 
												//should always be the length of the normal (1.0 unless something went wrong)

	for(float i=-halfsize;i<=halfsize;i+=stepSize) {	//iterate over neighborhood 3*sigma pixels in each direction
		for(float j=-halfsize;j<=halfsize;j+=stepSize) {

			vec2 offset = vec2(i,j);
			vec4 currNorm = texture(normalMap,TexCoords+offset*pixelSize);
			vec2 currGrad = grad(currNorm);
			float currWeight = weight(currNorm, pixel);
	
			//mix the current gradient with the center gradient based on depth difference and weight it based on distance
			currGrad = mixedGrad(centerGrad, currGrad, currWeight); 
			float distWeight = gaussFac * exp(-(offset.x * offset.x + offset.y * offset.y) / gaussDenom);
			currGrad = currGrad * distWeight;

			// update Hessian
			H.x = H.x + offset.x * currGrad.x; // gxx
			H.y = H.y + offset.y * currGrad.y; // gyy
			H.z = H.z + 0.5 * (offset.x * currGrad.y + offset.y * currGrad.x); // gxy (=gyx)
		}
	}

	return H * pixelWeight;
}

vec4 eigenValues(in vec3 m) {
	float tmp = max(sqrt(m.x*m.x + 4.0*m.z*m.z - 2.0*m.x*m.y + m.y*m.y), 0.0);
	float k1  = 0.5*(m.x+m.y+tmp);
	float k2  = 0.5*(m.x+m.y-tmp);
	vec2  d1  = vec2(m.z,k1-m.x);
  	vec2  d2  = vec2(k1-m.x,-m.z);
	
	d1 = length(d1)<eps ? vec2(0.) : normalize(d1);
	d2 = length(d2)<eps ? vec2(0.) : normalize(d2);

	// return max dir, max eigen-val, min eigen-val
	return k1>k2 ? vec4(d1.x,d1.y,k1,k2) : vec4(d2.x,d2.y,k2,k1);
}


void main() {
	vec4 pix = texture(normalMap,TexCoords);
	vec3 H = hessianMatrix(pix);
	vec4 ee = eigenValues(H);
	float mc = .5*(ee.z+ee.w);
	vec4 firstGaussianDeriv = vec4(ee.xy,mc,length(pix.xyz));
	FragColor = vec4(ee.xy, 0.0, 1.0);
	//FragColor = vec4(-ee.xy, 0.0, 1.0);
	//FragColor = pix;
}