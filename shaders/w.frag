#version 330
precision mediump float;

in vec3 world;
out vec4 outColor;

float lim = 0.5;
float meter_divisions = 4.;

vec3 world_dim = vec3(6, 3, 6);

void main() {
	// vec3 bars = pow(2.*abs(0.5 - mod(world * meter_divisions, 1.)), vec3(12));
	// float a = max(bars.r, bars.g) * max(bars.g, bars.b) * max(bars.r, bars.b);
	// //if (a < lim) discard;
	// a = smoothstep(lim, 1., a);
	// vec3 color = vec3(0.7, 0.9, 1.);
	// //
	// outColor = vec4(color, 1.) * (a * 0.25);
	
	vec3 c = world / world_dim;

	// c = abs(mod(world * meter_divisions, 1.)-0.5)*2.;
	// c = pow(c, vec3(4.));
	// float a = max(c.r, c.g) * max(c.g, c.b) * max(c.b, c.r);
	// a = smoothstep(0.3, 1., a);
	// c = vec3(a);

	outColor = vec4(0.7, 0.9, 1., 1.) * 0.3;
}