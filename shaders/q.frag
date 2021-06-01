#version 330
precision mediump float;
uniform sampler2D u_tex;
in vec2 v_texCoord;
out vec4 outColor;

// corner pins adjustments:
uniform vec2 tl; 
uniform vec2 tr;
uniform vec2 bl;
uniform vec2 br; 

// fades (0.001 is virtually no fade, 1 is full page)
uniform vec2 bl_fade; 
uniform vec2 tr_fade; 

void main() {
	vec2 v = v_texCoord;
	v += mix(mix(bl, tl, v_texCoord.y), mix(br, tr, v_texCoord.y), v_texCoord.x);
	float luma = 1.; //v.x < 0. || v.y < 0. || v.x > 1. || v.y > 1. ? 0. : 1.;

	// vignette
	luma *= smoothstep(0., bl_fade.x, v.x);
	luma *= smoothstep(0., tr_fade.x, 1.-v.x);
	luma *= smoothstep(0., bl_fade.y, v.y);
	luma *= smoothstep(0., tr_fade.y, 1.-v.y);
	//luma *= smoothstep(0., 0.01, v_texCoord.y);
	//luma *= smoothstep(1., 0.9, v_texCoord.y);

	//luma = max(luma, 0.2);

	outColor = texture(u_tex, v) * luma;
	//outColor = vec4(v, 0.5, 1.) * luma;
	//outColor = vec4(smoothstep(0., left_fade, v.x), smoothstep(0., left_fade, 1.-v.x), 0., 1.);
	outColor = vec4(1.);
}