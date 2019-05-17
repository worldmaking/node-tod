#version 330
precision mediump float;

in vec3 world;
out vec4 outColor;

float lim = 0.5;
float meter_divisions = 5.;

vec3 world_dim = vec3(6, 3, 6);

void main() {
	vec3 bars = pow(2.*abs(0.5 - mod(world * meter_divisions, 1.)), vec3(8));
	float a = max(bars.r, bars.g) * max(bars.g, bars.b) * max(bars.r, bars.b);
	if (a < lim) discard;
	vec3 color = vec3(0.7, 0.9, 1.);
	//outColor = world / world_dim;
	outColor = vec4(color, 1.) * (a * 0.25);
}