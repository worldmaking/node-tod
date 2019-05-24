#version 330
uniform mat4 u_viewmatrix;
uniform mat4 u_projmatrix;
uniform vec3 u_lightposition;
in vec3 a_position;
in vec3 a_normal;
in vec2 a_texCoord;
in vec4 a_orientation;
in vec4 a_color;
in vec3 a_location;
in float a_age;
in vec3 a_scale;
out vec4 v_color;
out vec3 v_normal;
out vec3 v_lightdir;
out vec3 v_vertex;

const float pi = 3.141592653589793;

//	q must be a normalized quaternion
vec3 quat_rotate(vec4 q, vec3 v) {
	vec4 p = vec4(
				q.w*v.x + q.y*v.z - q.z*v.y,	// x
				q.w*v.y + q.z*v.x - q.x*v.z,	// y
				q.w*v.z + q.x*v.y - q.y*v.x,	// z
				-q.x*v.x - q.y*v.y - q.z*v.z	// w
				);
	return vec3(
				p.x*q.w - p.w*q.x + p.z*q.y - p.y*q.z,	// x
				p.y*q.w - p.w*q.y + p.x*q.z - p.z*q.x,	// y
				p.z*q.w - p.w*q.z + p.y*q.x - p.x*q.y	// z
				);
}

vec3 beetle_animate(vec3 vertex, float phase, float amp) {
	// distort the geometry:
	float x = vertex.x;
	float y = vertex.y;
	float z = vertex.z;
	
	// yshift is proportional to z
	// zshift moves toward origin if yshift has same sign as y
	// and zshift is proportional to yshift
	float s = sin(phase * 2. * pi);
	float c = cos(phase * 2. * pi);
	
	float expansion = (1. + 0.06 * s);
	float wingpush = (abs(x) * 0.4 * (0.5+sin((z + phase*2.) * pi)));
	float wingopen = (y * abs(x) * 0.4 * (0.5*c));
	
	// wing component: greater as x increases:
	float side = sign(x);
	
	float yshift = (abs(z) * 0.15 * s) + (-0.04*s) + wingopen;
	float zshift = amp * ( (-yshift * z * y) + wingpush);
	
	return vec3(
				expansion*x,
				expansion*y + yshift,
				expansion*z + zshift
				);
}

void main() {
	vec3 vertex = a_position.xyz;
	vertex = beetle_animate(vertex, a_age * 5., 1.);
	v_vertex = vertex;
	vertex = quat_rotate(normalize(a_orientation), (vertex * a_scale)) + a_location.xyz;
	
	gl_Position = u_projmatrix * u_viewmatrix * vec4(vertex, 1);

	v_color = a_color;
	v_normal = quat_rotate(a_orientation, a_normal);
	v_lightdir = vertex - u_lightposition;

}