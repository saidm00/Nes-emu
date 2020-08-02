#version 330 core
#extension GL_ARB_separate_shader_objects: enable

//in vec2 fragUv;
//in vec3 fragColor;
//in vec3 fragPosition;

//uniform sampler2D inFontAtlas;

uniform vec4 uColor;

void main(void)
{
//	float alpha = texture(inFontAtlas, fragUv).r;

//	if (alpha <= 0.001) discard;

//	gl_FragColor = vec4(fragColor, alpha);

	gl_FragColor = uColor;
}