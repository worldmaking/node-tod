#version 330
in vec4 a_position;
in vec2 a_texCoord;
out vec2 v_texCoord;

void main() {
	v_texCoord = a_texCoord;
	gl_Position = vec4(v_texCoord*2.-1., 0., 1.);
}