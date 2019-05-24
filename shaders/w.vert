#version 330
uniform vec3 u_world_dim;
uniform mat4 u_viewmatrix;
uniform mat4 u_projmatrix;
in vec3 a_position;
in vec3 a_normal;
in vec2 a_texCoord;
out vec3 world;

void main() {
	world = a_position.xyz; // * u_world_dim;
	gl_Position = u_projmatrix * u_viewmatrix * vec4(world, 1);
}