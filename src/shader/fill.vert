#version 400
#extension GL_ARB_separate_shader_objects  : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform buffer{
  vec2 viewSize;
};

layout (location = 0) in vec2 vertex;
layout (location = 1) in vec2 tcoord;
layout (location = 0) out vec2 ftcoord;
layout (location = 1) out vec2 fpos;
void main(void) {
	ftcoord = tcoord;
	fpos = vertex;
	gl_Position = vec4(2.0*vertex.x/viewSize.x - 1.0, 2.0*vertex.y/viewSize.y - 1.0, 0, 1);
}