#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
	float alpha = texture(screenTexture, TexCoords).a;
	//alpha = alpha * 20.0;
    FragColor = vec4(alpha, alpha, alpha, 1.0);
	//FragColor = vec4(1.0, 0.0, 0.0, 1.0);
} 