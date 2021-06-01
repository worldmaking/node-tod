
console.log("demo run")

const assert = require("assert"),
	fs = require("fs"),
	os = require("os"),
	path = require("path")

const { vec2, vec3, vec4, quat, mat2, mat2d, mat3, mat4} = require("gl-matrix")

const glespath = path.join("..", "node-gles3");
const gl = require(path.join(glespath, '../node-gles3/gles3.js')),
	glfw = require(path.join(glespath, '../node-gles3/glfw3.js')),
	glutils = require(path.join(glespath, '../node-gles3/glutils.js'))

const events = require("./events.js")

let cubeprogram = glutils.makeProgram(gl,
`#version 330
uniform mat4 u_modelmatrix;
uniform mat4 u_viewmatrix;
uniform mat4 u_projmatrix;
in vec3 a_position;
in vec3 a_normal;
in vec2 a_texCoord;
out vec4 v_color;

void main() {
	// Multiply the position by the matrix.
	vec3 vertex = a_position.xyz;
	gl_Position = u_projmatrix * u_viewmatrix * u_modelmatrix * vec4(vertex, 1);

	v_color = vec4(a_normal*0.25+0.25, 1.);
	v_color += vec4(a_texCoord*0.5, 0., 1.);

	// if using gl.POINTS:
	gl_PointSize = 10.;
}
`,
`#version 330
precision mediump float;

in vec4 v_color;
out vec4 outColor;

void main() {
	outColor = v_color;
}
`);

let cube_geom = glutils.makeCube()
let cube = glutils.createVao(gl, cube_geom, cubeprogram.id);
let modelmatrix = mat4.create();

let angle = 0
let pos = [1*(Math.random()-0.5), 1, -2+(Math.random())]
let scale = [0.2, 0.2, 0.2]
let orient = quat.create()
quat.random(orient)
let rot = quat.create()

function cube_animate(dt, t) {
	angle += dt * 0.0001
	quat.setAxisAngle(rot, [0,1,0], dt)
	quat.multiply(orient, orient, rot)
	//quat.multiply(orient, rot, orient)
	mat4.fromRotationTranslationScale(modelmatrix, orient, pos, scale)
}

function cube_draw(state) {
	cubeprogram.begin();
	cubeprogram.uniform("u_modelmatrix", modelmatrix);
	cubeprogram.uniform("u_viewmatrix", state.viewmatrix);
	cubeprogram.uniform("u_projmatrix", state.projmatrix);
	//cube.bind().drawPoints().unbind();
	cube.bind().draw().unbind();
	cubeprogram.end();
}

events.add("animate", cube_animate)
events.add("draw:opaque", cube_draw)

// will need an unload/displose handler

console.log("loaded demo", new Date())

function dispose() {
	console.log("disposing")
	cube.dispose()
	cubeprogram.dispose()
	events.remove("animate", cube_animate)
	events.remove("draw:opaque", cube_draw)
}

module.exports = {
	msg: "hi",
	// insert unloader here:
	dispose: dispose,
}