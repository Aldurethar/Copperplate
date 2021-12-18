#version 460 core
out vec4 FragColor;

in vec3 WSNorm;

uniform vec3 lightDirection;

void main()
{	
	float ambient = 0.0;
	float diffuse = dot(-normalize(lightDirection), normalize(WSNorm));
	//diffuse = clamp(diffuse, 0.0, 1.0);
	diffuse = (diffuse * 0.5) + 0.5;

	float brightness = ambient + diffuse;
    FragColor = vec4(vec3(brightness), 1.0f);
} 