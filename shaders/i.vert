#version 330

uniform mat4 u_viewmatrix;
uniform mat4 u_projmatrix;
uniform vec3 u_world_dim;
uniform sampler3D u_gradient;
uniform float u_now;

in vec3 a_position;
in vec3 a_normal;
out vec3 vertex;
out vec3 normal;
out vec3 color;
out vec4 view;

void main() {
	vertex = a_position.xyz;
	normal = a_normal;

	// wobbly offset according to position:
	vec3 t = u_now * vec3(3., 4., 5.);
	float scale = 3.141592653589793 * .9;
	float range = .1 * 3./32.;
	vertex += range * (sin(t + vertex * scale));

	vec3 grad = texture(u_gradient, vertex/u_world_dim).xyz;
	color = 0.5 + grad;

	view = u_viewmatrix * vec4(vertex, 1.);

	gl_Position = u_projmatrix * view;
	float pixelSize = 6.;
	if (gl_Position.w > 0.0) {
		gl_PointSize = pixelSize / gl_Position.w;
	} else {
		gl_PointSize = 0.0;
	}
}
