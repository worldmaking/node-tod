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
in float a_phase;
in vec3 a_scale;
out vec4 v_color;
out vec3 v_normal;
out vec3 v_lightdir;

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

vec3 snake_animate(in vec3 vertex, in float phase) {
	// distort the geometry:
	float x = vertex.x;
	float y = vertex.y;
	float z = vertex.z;
	
	float boundary = -1.0;
	float z1 = boundary - z;
	float s = sin((phase + 0.1) * -2. * pi);
	float s1 = sin((-0.4 - phase) * 2. * pi);
	float c = cos(phase * -2. * pi);
	if (z1 > 0.) {
		// leaf
		
		// convert to spherical:
		vec3 cart = vec3(x, y, z1);
		float r = length(cart);
		float t = acos(z1 / r);
		float p = atan(y, x);
		// how far back the wings fold:
		// theta
		t = t + s * 0.7;
		// radius
		r = r + s1*0.1;
		
		x = r * sin(t) * cos(p);
		y = r * sin(t) * sin(p);
		z1 = r * 2. * cos(t);
		z = boundary - z1;
		
		// plant-stage:
		/*
		if (phase < 1110.) {
			float petal = max(0., phase + 1.);
			x *= petal;
			y *= petal;
			z *= petal;
		}
		*/
	} else {
		// stalk
		float breath = 0.5 + 0.3 * (2. + sin((-phase + 0.5) * pi * 2.));
		x = x * breath;
		y = y * breath;
		z = z;
	}
	return vec3(x, y, z);
}

void main() {
	vec3 vertex = a_position.xyz;
	vertex = snake_animate(vertex, a_phase);
	vertex = quat_rotate(normalize(a_orientation), (vertex * a_scale)) + a_location.xyz;
	
	gl_Position = u_projmatrix * u_viewmatrix * vec4(vertex, 1);

	v_color = a_color;

	/*
	// subScatter calculations
	// (none of this makes sense)
	ssVertPos = gl_Position.xyz;
	ssLightVec = lightPosition - ssVertPos;
	ssEyeVec = -ssVertPos;
	ssLightPos = ssVertPos + lightPosition;
	*/
	
	v_normal = quat_rotate(a_orientation, a_normal);
	v_lightdir = vertex - u_lightposition;
}