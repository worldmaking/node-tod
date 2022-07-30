
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
let NUM_CUBES = 40000

let cubeprogram = glutils.makeProgram(gl,
`#version 330
uniform mat4 u_viewmatrix;
uniform mat4 u_projmatrix;

// instanced variable:
in vec4 i_pos;
in vec4 i_quat;
in vec3 i_color;

in vec3 a_position;
in vec3 a_normal;
in vec2 a_texCoord;
out vec4 v_color;
out vec3 v_worldpos;
out vec3 v_normal;

// http://www.geeks3d.com/20141201/how-to-rotate-a-vertex-by-a-quaternion-in-glsl/
vec3 quat_rotate( vec4 q, vec3 v ){
	return v + 2.0 * cross( q.xyz, cross( q.xyz, v ) + q.w * v );
}
vec4 quat_rotate( vec4 q, vec4 v ){
	return vec4(v.xyz + 2.0 * cross( q.xyz, cross( q.xyz, v.xyz ) + q.w * v.xyz), v.w );
}

void main() {
	// Multiply the position by the matrix.
	vec4 vertex = vec4(a_position, 1.);
	vertex = quat_rotate(i_quat, vertex);
	vertex.xyz += i_pos.xyz;
	v_worldpos = vertex.xyz;
	gl_Position = u_projmatrix * u_viewmatrix * vertex;

	v_normal = quat_rotate(i_quat, a_normal);

	v_color = vec4(v_normal*0.25+0.25, 1.);
	v_color += vec4(a_texCoord*0.5, 0., 1.);
	v_color = vec4(i_color, 1.);
}
`,
`#version 330
precision mediump float;

in vec4 v_color;
in vec3 v_normal;
in vec3 v_worldpos;
out vec4 outColor;

void main() {
	float a = 0.;
	vec3 N = normalize(v_normal);
	//vec3 light_pos = vec3(cos(a), 1.7, sin(a));
	vec3 light_dir = vec3(0, 1, 0); //normalize(light_pos - v_worldpos);
	// similarity of light vector & normal vector
	// cosTheta of angle between them
    float NdotL = max(dot(N, light_dir), 0.0);  
	outColor = v_color * NdotL;
}
`);
// create a VAO from a basic geometry and shader
let x=0.006, y=0.0008, z=0.010
let cube = glutils.createVao(gl, glutils.makeCube({ 
	min:[-x, -y, -z], 
	max:[x, y, z], 
	div: 1 
}), cubeprogram.id);

// create a VBO & friendly interface for the instances:
// TODO: could perhaps derive the fields from the vertex shader GLSL?
let cubes = glutils.createInstances(gl, [
	{ name:"i_pos", components:4 },
	{ name:"i_quat", components:4 },
	{ name:"i_color", components:3 },
], NUM_CUBES)

// the .instances provides a convenient interface to the underlying arraybuffer
cubes.instances.forEach(obj => {
	// each field is exposed as a corresponding typedarray view
	// making it easy to use other libraries such as gl-matrix
	// this is all writing into one contiguous block of binary memory for all instances (fast)
	vec4.set(obj.i_pos, 
		(Math.random()) * 4,
		(Math.random()) * 3,
		(Math.random()) * 4,
		1
	);
	quat.random(obj.i_quat);

	vec3.set(obj.i_color, Math.random()*0.5+0.5, Math.random(), Math.random())

	obj.speed = 0.1
	obj.phase = Math.random()*1000
})
cubes.bind().submit().unbind();

// attach these instances to an existing VAO:
cubes.attachTo(cube);

function animate(dt, t) {
	for (let obj of cubes.instances) {
		let rot = quat.create()
		//quat.random(rot)
		// change its orientation:
		//quat.slerp(obj.i_quat, obj.i_quat, rot, 0.5 * dt);
		quat.fromEuler(rot, dt*1*(obj.i_color[0]-0.5), dt*100*Math.sin(obj.phase * (obj.i_color[1]-0.5)), dt*1*(obj.phase*(obj.i_color[2]-0.5)))
		//quat.multiply(obj.i_quat, rot, obj.i_quat)
		quat.multiply(obj.i_quat, obj.i_quat, rot)

		let spd = obj.speed * dt
		// forward vector derived from quat:
		let fwd = [0, 0, spd]
		vec3.transformQuat(fwd, fwd, obj.i_quat)

		vec3.add(obj.i_pos, obj.i_pos, fwd)

		for (let i=0; i<3; i++) {
			if (obj.i_pos[i] < 0) obj.i_pos[i] += WORLD_DIM[i]
			else if (obj.i_pos[i] > WORLD_DIM[i]) obj.i_pos[i] -= WORLD_DIM[i]
		}
		obj.phase += dt
	}
}

function update_gpu(state) {
	// submit to GPU:
	cubes.bind().submit().unbind()
}

function draw(state) {
	cubeprogram.begin();
	cubeprogram.uniform("u_viewmatrix", state.viewmatrix);
	cubeprogram.uniform("u_projmatrix", state.projmatrix);
	cube.bind().drawInstanced(cubes.count).unbind()
	cubeprogram.end();
}

events.add("animate", animate)
events.add("update_gpu", update_gpu)
events.add("draw:opaque", draw)

module.exports = {
	// insert unloader here:
	dispose() {
		// console.log("disposing")
		// cube.dispose()
		// cubeprogram.dispose()
		events.remove("animate", animate)
		events.remove("draw:opaque", draw)
		events.remove("update_gpu", update_gpu)
	},
}
