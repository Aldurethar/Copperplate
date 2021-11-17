#version 460 core
out vec2 FragColor;

in vec2 Movement;

void main()
{	
    FragColor = Movement;
	//FragColor = vec4(Norm, gl_FragCoord.z);
} 