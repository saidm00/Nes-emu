#version 330 core
#extension GL_ARB_separate_shader_objects: enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inColor;

uniform mat4 uModel;
uniform mat4 uProjection;

//out vec2 fragUv;
//out vec3 fragColor;
//out vec3 fragPosition;

void main(void) {
	mat4 mp = uProjection * uModel;
	vec3 pos = vec3(mp * vec4(inPosition, 1.0));

    gl_Position = vec4(pos, 1.0);

//	fragUv = inUv;
//	fragColor = inColor;
//	fragPosition = inPosition;
}