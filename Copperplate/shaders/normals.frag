#version 460 core
out vec4 FragColor;

in vec3 Norm;

void main()
{
	//vec3 color = (Norm * 0.5) + vec3(0.5, 0.5, 0.5);
    FragColor = vec4(Norm, 1.0);
	//FragColor = vec4(1.0, 0.0, 0.0, 1.0);
} 