#version 460 core
out vec4 FragColor;

in vec3 Norm;
in float Depth;

void main()
{	
    FragColor = vec4(Norm, Depth);
} 