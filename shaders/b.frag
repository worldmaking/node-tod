#version 330
precision mediump float;
in vec4 v_color;
in vec3 v_normal;
in vec3 v_lightdir;
in vec3 v_vertex;
out vec4 outColor;

vec4 highTone = vec4(1.0, 0.692, 0.07, 0.); // diffuse 0.19

float halfLambert(in vec3 v1, in vec3 v2) {
	return dot(v1, v2) * 0.5 + 0.5;
}

void main() {
	vec3 normal = normalize(v_normal);
	vec3 lightdir = normalize(v_lightdir);
	float ndotl = dot(normal, lightdir);
	float attenuation = (.5/length(v_vertex));

	vec3 indl = vec3(max(0., dot(normal, lightdir)));
	indl += 8. * halfLambert(v_vertex, -lightdir);
	indl *= v_color.rgb * attenuation;
	vec3 rim = vec3(1.-max(0., dot(normal, v_vertex)));
	rim *= rim;
	rim *= max(0., dot(normal, lightdir)) * highTone.rgb * attenuation;

	vec4 final_color = vec4(0.15*indl + 0.5*rim, 1.);

	outColor = final_color;
	//outColor = vec4(1.);
}