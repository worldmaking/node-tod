#version 330
precision mediump float;

in vec4 v_color;
in vec3 v_normal;
in vec3 v_lightdir;
in vec3 v_viewdir;
out vec4 outColor;


vec4 color1 = vec4(0.07, 0.07, 0.0, 1); // fvLowTone, ambient(0.06, 0.27, 0.30, 1)
vec4 color2 = vec4(0.00, 0.01, 0.02, 1); // fvSpecular, specular(0.1, 0.50, 0.55, 1)
vec4 color3 = vec4(0.02, 0.80, 0.95, 1); // fvHighTone, diffuse(0.05, 0.30, 0.55, 1)
vec4 hungrycolor1 = vec4(0.02, 0.01, 0.02, 1);
vec4 hungrycolor2 = vec4(0.90, 0.92, 0.92, 1);
vec4 hungrycolor3 = vec4(0.5, 0.5, 0.5, 1);

void main() {
	float energy = clamp(v_color.r, 0., 1.);

	vec4 fvLowTone = mix(hungrycolor1, color1, energy);
	vec4 fvHighTone = mix(hungrycolor2, color2, energy); // diffuse
	vec4 fvSpecular = mix(hungrycolor3, color3, energy);

	vec3 normal = normalize(v_normal);
	vec3 lightdir = normalize(v_lightdir);
	
	vec3 viewdir = normalize(v_viewdir);
	float ndotl = dot(normal, lightdir);
	// TODO this math seems suspect:
	float fSpecularPower = 1.57;
	vec3 reflection = normalize(((15.0 * normal) * ndotl) - lightdir);
	float rdotv = max(0., dot(reflection, viewdir));
	//float fresnel = abs(2./(1.+dot(normal, fvViewDirection)));
	float fresnel = max(0., 2./(1.+dot( normal, viewdir)));

	vec3 ambient = fvLowTone.rgb;
	vec3 diffuse = fvHighTone.rgb * ndotl;
	vec3 specular = fvSpecular.rgb * (pow(rdotv, fSpecularPower));

//	outColor = vec4(ambient + diffuse + specular, 1.);
	
	outColor = vec4(ambient + diffuse + specular, 1.);

	//outColor.rgb = specular;
}