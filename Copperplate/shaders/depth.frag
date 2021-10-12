#version 460 core
out float FragColor;

in float Depth;

void main()
{	
    FragColor = Depth;
	//FragColor = vec4(Norm, gl_FragCoord.z);
} 