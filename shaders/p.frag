#version 330
precision mediump float;

in vec4 v_color;
out vec4 outColor;

float gamma = 2.2;
vec3 gamma3 = vec3(1.0 / gamma);

void main() {
	// get normalized -1..1 point coordinate
	vec2 pc = (gl_PointCoord - 0.5) * 2.0;
	// convert to distance:
	float dist = max(0., 1.0 - length(pc));
	// paint
	outColor = v_color;
	outColor.a *= dist;

	outColor.rgb = pow(outColor.rgb, gamma3);
}