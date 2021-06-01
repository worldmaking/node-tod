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

let WORLD_DIM = [4,3,4]
let geomWalls = {}
let stepsize = 0.25;
let wallpts = [];
for (let x=0; x <= WORLD_DIM[0]; x+=stepsize) {
	wallpts.push(x, 0, 0, 		x, 0, WORLD_DIM[2]);
	wallpts.push(x, WORLD_DIM[1], 0, 		x, WORLD_DIM[1], WORLD_DIM[2]);
	wallpts.push(x, 0, 0, 		x, WORLD_DIM[1], 0);
	wallpts.push(x, 0, WORLD_DIM[2], 		x, WORLD_DIM[1], WORLD_DIM[2]);
}
for (let y=0; y <= WORLD_DIM[1]; y+=stepsize) {
	wallpts.push(0, y, 0, 		WORLD_DIM[0], y, 0);
	wallpts.push(0, y, WORLD_DIM[2], 		WORLD_DIM[0], y, WORLD_DIM[2]);
	wallpts.push(0, y, 0, 		0, y, WORLD_DIM[2]);
	wallpts.push(WORLD_DIM[0], y, 0, 		WORLD_DIM[0], y, WORLD_DIM[2]);
}
for (let z=0; z <= WORLD_DIM[2]; z+=stepsize) {
	wallpts.push(0, 0, z, 		WORLD_DIM[0], 0, z);
	wallpts.push(0, WORLD_DIM[1], z, 		WORLD_DIM[0], WORLD_DIM[1], z);
	wallpts.push(0, 0, z, 		0, WORLD_DIM[1], z);
	wallpts.push(WORLD_DIM[0], 0, z, 		WORLD_DIM[0], WORLD_DIM[1], z);
}
geomWalls.vertices = new Float32Array(wallpts);
let wallprogram = glutils.makeProgram(gl,
	fs.readFileSync("shaders/w.vert", "utf-8"),
	fs.readFileSync("shaders/w.frag", "utf-8")
);
//let wall = glutils.createVao(gl, glutils.makeCube(0), wallprogram.id);
let wall = glutils.createVao(gl, geomWalls, wallprogram.id);


function animate(dt, t) {
	
}

function draw_opaque(state) {
	gl.lineWidth(16)
	wallprogram.begin();
	wallprogram.uniform("u_world_dim", WORLD_DIM);
	wallprogram.uniform("u_viewmatrix", state.viewmatrix);
	wallprogram.uniform("u_projmatrix", state.projmatrix);
	wall.bind().drawLines().unbind();
	wallprogram.end();
}

events.add("animate", animate)
events.add("draw:opaque", draw_opaque)



module.exports = {
	// insert unloader here:
	dispose() {
		console.log("disposing")
		events.remove("animate", animate)
		events.remove("draw:opaque", draw_opaque)
	},
}