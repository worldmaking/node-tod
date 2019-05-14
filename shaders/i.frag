#version 330
precision mediump float;

uniform float u_alpha;

out vec4 outColor;
in vec3 vertex;
in vec3 normal;
in vec3 color;
in vec4 view;

vec3 light0dir = vec3(1, 0, 1);
vec3 light2dir = vec3(-1, 0, -1);
vec3 light1dir = vec3(0, -1, 1);
vec3 light3dir = vec3(1, 1, -1);

//Haru
vec3 light0color = vec3(0.4, 0.2, 0.6); //(0.9, 0.5, 0.6);
vec3 light1color = vec3(0.55, 0.55, 0.95); //(0.0, 0.55, 0.75);
vec3 light2color = vec3(0.03, 0.65, 0.65);//(0.13, 0.35, 0.65)(0.0, 0.55, 0.65);
vec3 light3color = vec3(0.05, 0.4, 0.7);//(0.1, 0.4, 0.7)(0.0, 0.5, 0.7)

float gamma = 2.2;
vec3 gamma3 = vec3(1.0 / gamma);

void main() {

	vec3 nn = normalize(normal);

	vec3 lighting
		= abs(dot(light0dir, nn)) * light0color
		+ abs(dot(light1dir, nn)) * light1color
		+ abs(dot(light2dir, nn)) * light2color
		+ abs(dot(light3dir, nn)) * light3color
	;
	
	float glancing = 1.1 - abs(dot(normalize(view.xyz), nn));
	glancing = 3. * pow(glancing, 3.);
	
	lighting *= glancing*2.;
	

	outColor = vec4(lighting, 1.);
	
	outColor *= u_alpha;
	
	// gamma correct
	outColor.rgb = pow(outColor.rgb, gamma3);

	//outColor = vec4(nn*0.5 + 0.5, 1.); //vec4(mod(vertex, 1.), 1.);
}