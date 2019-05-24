#version 330
precision mediump float;

out vec4 outColor;
in vec3 world;

float gamma = 1.6;//2.2;
vec3 gamma3 = vec3(1.0 / gamma);

void main() {
	// get normalized -1..1 point coordinate
	vec2 pc = (gl_PointCoord - 0.5) * 2.0;
	// convert to distance:
	float dist = max(0., 1.0 - length(pc));
	// paint
    outColor = vec4(dist) * 0.35;
    if (world.x < 0. || world.x > 6. || world.y < 0. || world.y > 3. || world.z < 0. || world.z > 6.) discard;
    //outColor.rgb *= world;
	//outColor.rgb = pow(outColor.rgb, gamma3);
}