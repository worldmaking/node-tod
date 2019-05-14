#version 330
precision mediump float;

in vec3 world;
out vec4 outColor;

float lim = 0.5;
float meter_divisions = 5.;

void main() {
	vec3 color = pow(2.*abs(0.5 - mod(world * meter_divisions, 1.)), vec3(8.));
	float a = max(color.r, color.g) * max(color.g, color.b) * max(color.r, color.b);
	if (a < lim) discard;
	outColor = vec4(0.7, 0.9, 1., 1.) * (a * 0.5);
}