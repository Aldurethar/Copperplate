#version 460 core
out vec4 FragColor;

in vec3 fColor;

void main()
{
    FragColor = vec4(fColor, 1.0);
	//FragColor = vec4(1.0, 0.0, 0.0, 1.0);
} 