#version 330
uniform mat4 u_viewmatrix;
uniform mat4 u_projmatrix;
uniform float u_pixelSize;
in vec4 a_color;
in vec3 a_location;
out vec4 v_color;


vec3 deadcolor = vec3(0.3);
vec3 birthcolor = vec3(1., 0.5, 0.1);
vec3 deathcolor = vec3(1., 0., 0.);

void main() {
	// unpack:
	float energy = a_color.x;
	float age = a_color.y;
	float agelimit = a_color.z;
	float dead = a_color.w; // either 0 or 1

	if (dead > 0.5) {
		float fadein = clamp(age * 4., 0., 1.);
		float fadeout = clamp(0.5 * float(agelimit - age), 0., 1.);
		float fade = (fadein * fadeout);
		v_color = vec4(deadcolor, fade);
	} else {
		float phase = age/agelimit;
		float fadein = clamp(age * 4., 0., 1.);
		float fadeout = clamp(2. * float(agelimit - age), 0., 1.);
		float fade = (fadein * fadeout);
		//float fade = clamp(32.*abs(0.5f-mod(energy+0.5, 1.)), 0., 1.);
		vec3 c = mix(birthcolor, deathcolor, phase);
		v_color = vec4(1, .1+.5*(1.-energy), 0.1, fade * 1.2);
		//v_color = vec4(c, fade);
	}

	// Multiply the position by the matrix.
	gl_Position = u_projmatrix * u_viewmatrix * vec4(a_location.xyz, 1);

	if (gl_Position.w > 0.0) {
		gl_PointSize = u_pixelSize / gl_Position.w;
	} else {
		gl_PointSize = 0.0;
	}
}