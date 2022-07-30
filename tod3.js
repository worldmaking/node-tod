const assert = require("assert"),
	fs = require("fs"),
	os = require("os"),
	path = require("path")

const { Worker } = require('worker_threads')
const { vec2, vec3, vec4, quat, mat2, mat2d, mat3, mat4} = require("gl-matrix")

const glespath = path.join("..", "node-gles3");
const gl = require(path.join(glespath, '../node-gles3/gles3.js')),
	glfw = require(path.join(glespath, '../node-gles3/glfw3.js')),
	vr = require(path.join(glespath, '../node-gles3/openvr.js')),
	glutils = require(path.join(glespath, '../node-gles3/glutils.js'))


const tod = require('bindings')('tod.node');

let sb = new SharedArrayBuffer(1024);
let sbf = new Float32Array(sb)
sbf[1] = 10

console.log(tod.test(sb, sbf))

console.log(sbf)

// CONFIG
let usevr = 0 //(os.platform == "win32");

process.argv.forEach(s=>{
	let match
	// -vr or vr=true or vr=1 etc.
	if (match = s.match(/^vr(=(1|true|yes|y))?$/) ) {
		usevr = true;
		console.log("vr", usevr)
	} else if (match = s.match(/^vr(=(0|false|no|n))?$/) ) {
		usevr = false;
		console.log("vr", usevr)
	}
})

const WORLD_DIM = [6, 3, 6]
const NUM_PARTICLES = 20000;
const NUM_GHOSTPOINTS = 320000;
const NUM_SNAKE_SEGMENTS = 136;
const NUM_BEETLES = 2048;

// TODO could we risk increasing this resolution?
const NUM_VOXELS = 32 * 16 * 32;


// derive from header
let beetleBufferByteStride = 16*4
let snakeBufferByteStride = 16*4
let pointsBufferByteStride = 8*4
let ghostBufferByteStride = 4*4
let isoBufferStride = 6*4


let sab = new SharedArrayBuffer(10*8)
let sab2 = new SharedArrayBuffer(10*8)
let wshared = new Float32Array(sab)
let wshared2 = new Float32Array(sab2)
wshared[0] = 10
wshared2[0] = 2
const worker = new Worker('./tod3_worker.js', { workerData: [wshared, wshared2] });
worker.on('message', msg => console.log(msg));
worker.on('error', err => console.error(err));
worker.on('exit', code => {
	if (code !== 0) console.error(`Worker stopped with exit code ${code}`)
	else console.log("worker done")
});


let shared = tod.setup(sab)

// TODO: derive this from struct header?
let byteoffset = 0;
console.log("snakes", byteoffset)
let snakeInstanceData = new Float32Array(shared, byteoffset, NUM_SNAKE_SEGMENTS * 16)
byteoffset += snakeInstanceData.byteLength
console.log("beetles", byteoffset)
let beetleInstanceData = new Float32Array(shared, byteoffset, NUM_BEETLES * 16)
byteoffset += beetleInstanceData.byteLength
console.log("particles", byteoffset)
let particleData = new Float32Array(shared, byteoffset, NUM_PARTICLES * 8)
byteoffset += particleData.byteLength

console.log("ghosts", byteoffset)
let ghostData = new Float32Array(shared, byteoffset, NUM_GHOSTPOINTS * 4);
byteoffset += ghostData.byteLength;

//glm::vec3 isovertices[NUM_VOXELS * 5];
//	uint32_t isoindices[NUM_VOXELS * 15]
let isovertices = new Float32Array(shared, byteoffset, NUM_VOXELS * 5 * 6)
byteoffset += isovertices.byteLength
let isoindices = new Uint32Array(shared, byteoffset, NUM_VOXELS * 15);
byteoffset += isoindices.byteLength;
let isoGeom = {
	indices: isoindices,
}
// console.log(isoGeom.indices[isoGeom.indices.length-1])
// console.log(isovertices[isovertices.length-1])
// console.log(isovertices)

let counts = new Uint32Array(shared, byteoffset, 4)
byteoffset += counts.byteLength;
// console.log(counts)
// console.log(isoGeom.indices[counts[3]-1])
// console.log(isovertices[counts[2]-1])

const world = {
	width: 6,
	depth: 6,
	height: 3,

}
world.dim = vec3.fromValues(world.width, world.height, world.depth);

// load resources:
let geomWing = glutils.geomFromOBJ(fs.readFileSync("wingset01.obj", "utf-8"))
let geomBody = glutils.geomFromOBJ(fs.readFileSync("spc_highr_end03.obj", "utf-8"))
let geomsnake = glutils.geomFromOBJ(fs.readFileSync("snake_fat_adjust1.obj", "utf-8"))


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

let beetleBufferFields = [
	{ name:"a_orientation", components:4, type:gl.FLOAT, bytesize: 16, byteoffset:0*4 },
	{ name:"a_color", components:4, type:gl.FLOAT, bytesize: 16, byteoffset:4*4 },
	{ name:"a_location", components:3, type:gl.FLOAT, bytesize: 12, byteoffset:8*4 },
	{ name:"a_age", components:1, type:gl.FLOAT, bytesize: 4, byteoffset:11*4 },
	{ name:"a_scale", components:3, type:gl.FLOAT, bytesize: 12, byteoffset:12*4 },
]
let snakeBufferFields = [
	{ name:"a_orientation", components:4, type:gl.FLOAT, bytesize: 16, byteoffset:0*4 },
	{ name:"a_color", components:4, type:gl.FLOAT, bytesize: 16, byteoffset:4*4 },
	{ name:"a_location", components:3, type:gl.FLOAT, bytesize: 12, byteoffset:8*4 },
	{ name:"a_phase", components:1, type:gl.FLOAT, bytesize: 4, byteoffset:11*4 },
	{ name:"a_scale", components:3, type:gl.FLOAT, bytesize: 12, byteoffset:12*4 },
]
let particleBufferFields = [
	{ name:"a_location", components:3, type:gl.FLOAT, bytesize: 12, byteoffset:0*4 },
	{ name:"a_color", components:4, type:gl.FLOAT, bytesize: 16, byteoffset:4*4 }
]
let ghostBufferFields = [
	{ name:"a_location", components:3, type:gl.FLOAT, bytesize: 12, byteoffset:0*4 },
]
let isoBufferFields = [
	{ name:"a_position", components:3, type:gl.FLOAT, bytesize: 12, byteoffset:0*3 },
	{ name:"a_normal", components:3, type:gl.FLOAT, bytesize: 12, byteoffset:4*3 }
]

function projection(projMatrix, 
	pa,	// bottom-left screen coordinate
	pb,	// bottom-right screen coordinate
	pc,	// top-left screen coordinate
	pe,	// eye coordinate
	n, f) {	// near, far clip

	// compute orthonormal basis for the screen
	let vr = vec3.create();
	vec3.sub(vr, pb, pa);
	vec3.normalize(vr, vr);	// right vector
	let vu = vec3.create()
	vec3.sub(vu, pc, pa);
	vec3.normalize(vu, vu); // upvector
	let vn = vec3.create()
	vec3.cross(vn, vr, vu)
	vec3.normalize(vn, vn); // normal(forward) vector (out from screen)

	// compute vectors from eye to screen corners:
	let va = vec3.create()
	vec3.sub(va, pa, pe);
	let vb = vec3.create()
	vec3.sub(vb, pb, pe)
	let vc = vec3.create();
	vec3.sub(vc, pc, pe);

	// distance from eye to screen-plane
	// = component of va along vector vn, i.e. normal to screen
	let d = -vec3.dot(va, vn);

	// find extent of perpendicular projection
	let nbyd = n/d;
	let l = vec3.dot(vr, va) * nbyd;
	let r = vec3.dot(vr, vb) * nbyd;
	let b = vec3.dot(vu, va) * nbyd;	// not vd?
	let t = vec3.dot(vu, vc) * nbyd;
	mat4.frustum(projMatrix, l, r, b, t, n, f);
}

function start() {
	if (!glfw.init()) {
		console.log("Failed to initialize GLFW");
		process.exit(-1);
	}
	let version = glfw.getVersion();
	console.log('glfw ' + version.major + '.' + version.minor + '.' + version.rev);
	console.log('glfw version-string: ' + glfw.getVersionString());

	// Open OpenGL window
	glfw.defaultWindowHints();
	glfw.windowHint(glfw.CONTEXT_VERSION_MAJOR, 3);
	glfw.windowHint(glfw.CONTEXT_VERSION_MINOR, 3);
	glfw.windowHint(glfw.OPENGL_FORWARD_COMPAT, 1);
	glfw.windowHint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE);

	let window = glfw.createWindow(720, 480, "Test");
	if (!window) {
		console.log("Failed to open GLFW window");
		glfw.terminate();
		process.exit(-1);
	}
	glfw.makeContextCurrent(window);
	console.log(gl.glewInit());

	//can only be called after window creation!
	console.log('GL ' + glfw.getWindowAttrib(window, glfw.CONTEXT_VERSION_MAJOR) + '.' + glfw.getWindowAttrib(window, glfw.CONTEXT_VERSION_MINOR) + '.' + glfw.getWindowAttrib(window, glfw.CONTEXT_REVISION) + " Profile: " + glfw.getWindowAttrib(window, glfw.OPENGL_PROFILE));

	// gl.enable(gl.POINT_SPRITE); // GL_POINT_SPRITE 0x8861
	// gl.enable(0x8642); // GL_VERTEX_PROGRAM_POINT_SIZE
	// gl.enable(0x8862); // GL_COORD_REPLACE


	// Enable vertical sync (on cards that support it)
	glfw.swapInterval(0); // 0 for vsync off

	let updating = true


	// key is the (ascii) keycode, scan is the scancode
	// down=1 for keydown, down=0 for keyup, down=2 for key repeat
	// mod is a bitfield in which shift=1, ctrl=2, alt/option=4, mac/win=8
	glfw.setKeyCallback(window, (win, key, scan, down, mod) => {
		if (down==1 && key == 32) updating = !updating;
		let shift = !!(mod % 2);
		let ctrl = !!(mod % 4);
		switch (key) {
			case 257:
			case 335: // enter, return
			reload();
			break;
			default: console.log("key", key, down, shift, ctrl)
		}
		
		//console.log(key, down, mod);
	})

	let beetleProgram = glutils.makeProgram(gl, 
		fs.readFileSync("shaders/b.vert", "utf-8"),
		fs.readFileSync("shaders/b.frag", "utf-8")
	);
	let beetleWings = glutils.createVao(gl, geomWing, beetleProgram.id);
	let beetleBody = glutils.createVao(gl, geomBody, beetleProgram.id);

	let beetleInstanceBuffer = gl.createBuffer()
	gl.bindBuffer(gl.ARRAY_BUFFER, beetleInstanceBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, beetleInstanceData, gl.DYNAMIC_DRAW);
	beetleBody.bind().setAttributes(beetleInstanceBuffer, beetleBufferByteStride, beetleBufferFields, true).unbind();
	beetleWings.bind().setAttributes(beetleInstanceBuffer, beetleBufferByteStride, beetleBufferFields, true).unbind();

	let snakeprogram = glutils.makeProgram(gl, 
		fs.readFileSync("shaders/s.vert", "utf-8"),
		fs.readFileSync("shaders/s.frag", "utf-8")
	);
	let snake = glutils.createVao(gl, geomsnake, snakeprogram.id);

	let snakeInstanceBuffer = gl.createBuffer()
	gl.bindBuffer(gl.ARRAY_BUFFER, snakeInstanceBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, snakeInstanceData, gl.DYNAMIC_DRAW);
	snake.bind().setAttributes(snakeInstanceBuffer, snakeBufferByteStride, snakeBufferFields, true).unbind();

	let pointprogram = glutils.makeProgram(gl,
		fs.readFileSync("shaders/p.vert", "utf-8"),
		fs.readFileSync("shaders/p.frag", "utf-8")
	);
	let pointsVao = glutils.createVao(gl, null, pointprogram.id);
	let pointsBuffer = gl.createBuffer()
	gl.bindBuffer(gl.ARRAY_BUFFER, pointsBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, particleData, gl.DYNAMIC_DRAW);
	pointsVao.bind()
		.setAttributes(pointsBuffer, pointsBufferByteStride, particleBufferFields, false)
		.unbind();

	let ghostprogram = glutils.makeProgram(gl,
		fs.readFileSync("shaders/g.vert", "utf-8"),
		fs.readFileSync("shaders/g.frag", "utf-8")
	);
	let ghostVao = glutils.createVao(gl, null, ghostprogram.id);
	let ghostBuffer = gl.createBuffer()
	gl.bindBuffer(gl.ARRAY_BUFFER, ghostBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, ghostData, gl.DYNAMIC_DRAW);
	ghostVao.bind()
		.setAttributes(ghostBuffer, ghostBufferByteStride, ghostBufferFields, false)
		.unbind();

	let isoprogram = glutils.makeProgram(gl,
		fs.readFileSync("shaders/i.vert", "utf-8"),
		fs.readFileSync("shaders/i.frag", "utf-8")
	);
	let isoVao = glutils.createVao(gl, isoGeom, isoprogram.id);
	let isoBuffer = gl.createBuffer()
	gl.bindBuffer(gl.ARRAY_BUFFER, isoBuffer);
	gl.bufferData(gl.ARRAY_BUFFER, isovertices, gl.DYNAMIC_DRAW);
	isoVao.bind()
		.setAttributes(isoBuffer, isoBufferStride, isoBufferFields, false)
		.unbind()

	let wallprogram = glutils.makeProgram(gl,
		fs.readFileSync("shaders/w.vert", "utf-8"),
		fs.readFileSync("shaders/w.frag", "utf-8")
	);
	//let wall = glutils.createVao(gl, glutils.makeCube(0), wallprogram.id);
	let wall = glutils.createVao(gl, geomWalls, wallprogram.id);

	let quadprogram = glutils.makeProgram(gl,
	`#version 330
	in vec4 a_position;
	in vec2 a_texCoord;
	uniform vec2 u_scale;
	out vec2 v_texCoord;
	void main() {
		gl_Position = a_position;
		vec2 adj = vec2(1, -1);
		gl_Position.xy = (gl_Position.xy + adj)*u_scale.xy - adj;
		v_texCoord = a_texCoord;
	}`,
	`#version 330
	precision mediump float;
	uniform sampler2D u_tex;
	in vec2 v_texCoord;
	out vec4 outColor;

	void main() {
		outColor = vec4(v_texCoord, 0., 1.);
		outColor = texture(u_tex, v_texCoord);
		//outColor.g = 1.;
	}
	`);
	let quad = glutils.createVao(gl, glutils.makeQuad(), quadprogram.id);
	let fbodim = glfw.getFramebufferSize(window)

	if (usevr) {
		if (!vr.connect(true)) {
			console.error("vr failed to connect");
			usevr = false;
		} else {
			vr.update()

			let models = vr.getModelNames()
			console.log(models)

			fbodim[0] = vr.getTextureWidth()
			fbodim[1] = vr.getTextureHeight()
		}
	}

	console.log("fbo", fbodim)
	let fbo = glutils.makeFboWithDepth(gl, fbodim[0], fbodim[1])

	let t = glfw.getTime();
	let framecount = 0;
	let fps = 60;
	let dt = 1/fps;


	function renderEye(settings) {
		let dim = glfw.getFramebufferSize(window);
		let aspect = fbodim[0] / fbodim[1]

		// Compute the matrixs
		let lightposition = vec3.fromValues(world.width/2, world.height*2, world.depth/2)
		let viewmatrix = mat4.create();
		let projmatrix = mat4.create();
		let projmatrix_walls = mat4.create();

		// sets our camera height above floor
		let eye_height = 1.6;//55 + 0.*Math.cos(t);
		// shifts the image plane vertically
		// should be set such that the horizon line in real world matches the eyeheight variable above
		let strafey = 0.45 + 0.*Math.sin(t);
		// this basically determines our field of view, kind of vertigo effect
		let ha0 = 0.45 + 0.0*Math.sin(t);
		// sets focal range of world
		let near = 1 + 0.25*(Math.sin(t*0.1)+1);
		let zoom = near;

		let strafex = 0.;
		{
			let parallax_rate = 0.03
			let parallax_range = 0.03
			strafex = parallax_range * Math.sin(t * parallax_rate * 10.);
		}
		let wa0 = ha0 * aspect; 
		let nearclip_walls = WORLD_DIM[2]*near + 0.01;
		let farclip_walls = nearclip_walls + WORLD_DIM[2] - 0.02;
		let farclip = farclip_walls + WORLD_DIM[2];
		let nearclip = nearclip_walls - WORLD_DIM[2];
		let numeyes = 1
		let particlesize = 18

		{
			numeyes = 1
			particlesize *= 0.2
			
			mat4.lookAt(viewmatrix, 
				[0, 0, 1], 
				[0, 0, 0], 
				[0, 1, 0]
			);
			//mat4.translate(viewmatrix, viewmatrix, vec3.fromValues(0, 0, -world.depth/2));
			mat4.rotate(viewmatrix, viewmatrix, t*0.1, vec3.fromValues(0, 1, 0))
			mat4.translate(viewmatrix, viewmatrix, vec3.fromValues(-world.width/2, -world.height/2, -world.depth/2));
			//mat4.translate(viewmatrix, viewmatrix, center[0], center[1], center[2]);
			mat4.perspective(projmatrix, Math.PI/2, dim[0]/(dim[1]*2),nearclip, farclip);
			mat4.perspective(projmatrix_walls, Math.PI/2, dim[0]/(dim[1]*2), nearclip_walls, farclip_walls)
			projmatrix_walls = projmatrix;
		}
		// 	projmatrix: projmatrix,
		// 	viewmatrix: viewmatrix,
		// 	modelmatrix: modelmatrix,
		// 	fbo: fbo,
		viewmatrix = settings.viewmatrix
		projmatrix = settings.projmatrix
		projmatrix_walls = settings.projmatrix

		gl.depthMask(false)
		gl.lineWidth(16)
		wallprogram.begin();
		wallprogram.uniform("u_world_dim", world.width, world.height, world.depth);
		wallprogram.uniform("u_viewmatrix", viewmatrix);
		wallprogram.uniform("u_projmatrix", projmatrix_walls);
		wall.bind().drawLines().unbind();
		wallprogram.end();
		
		gl.depthMask(true)


		snakeprogram.begin();
		snakeprogram.uniform("u_viewmatrix", viewmatrix);
		snakeprogram.uniform("u_projmatrix", projmatrix);
		snakeprogram.uniform("u_lightposition", lightposition[0], lightposition[1], lightposition[2]);
		snake.bind().drawInstanced(NUM_SNAKE_SEGMENTS).unbind();
		snakeprogram.end();

		let live_beetles = counts[0]
		beetleProgram.begin();
		beetleProgram.uniform("u_viewmatrix", viewmatrix);
		beetleProgram.uniform("u_projmatrix", projmatrix);
		beetleProgram.uniform("u_lightposition", lightposition[0], lightposition[1], lightposition[2]);
		beetleBody.bind().drawInstanced(live_beetles).unbind();
		beetleWings.bind().drawInstanced(live_beetles).unbind();
		beetleProgram.end();

		gl.enable(gl.BLEND);
		gl.blendFunc(gl.SRC_ALPHA, gl.ONE);
		gl.depthMask(false)

		if (1) {
			// TODO: link mGooTex
			isoprogram.begin();
			isoprogram.uniform("u_viewmatrix", viewmatrix);
			isoprogram.uniform("u_projmatrix", projmatrix);
			isoprogram.uniform("u_world_dim", world.dim[0], world.dim[1], world.dim[2]);
			isoprogram.uniform("u_now", t);
			isoprogram.uniform("u_alpha", 0.1);
			isoVao.bind()
		
			gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, isoVao.indexBuffer);
			gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, isoindices, gl.DYNAMIC_DRAW);
				//.drawPoints(counts[2])
				//.draw(counts[2])
				//.drawLines(counts[2])
			gl.drawElements(gl.TRIANGLES, 
				counts[3], 
				gl.UNSIGNED_INT, 0);
			//console.log(counts[3], counts[2])
			//console.log(isovertices);
			// console.log(isoGeom.indices[counts[3]-1])
			// console.log(isovertices[counts[2]-6], isovertices[counts[2]-5], isovertices[counts[2]-4])

			isoVao.unbind();
			isoprogram.end();
		}
		if (0) {
			ghostprogram.begin();
			ghostprogram.uniform("u_pixelSize", particlesize * 2);
			ghostprogram.uniform("u_viewmatrix", viewmatrix);
			ghostprogram.uniform("u_projmatrix", projmatrix);
			ghostVao.bind()
			ghostVao.drawPoints(counts[1])
			ghostVao.unbind();
			ghostprogram.end();
		}
		if (1) {
			pointprogram.begin();
			pointprogram.uniform("u_pixelSize", particlesize * 2);
			pointprogram.uniform("u_viewmatrix", viewmatrix);
			pointprogram.uniform("u_projmatrix", projmatrix);
			pointsVao.bind()
			pointsVao.drawPoints(NUM_PARTICLES)
			pointsVao.unbind();
			pointprogram.end();
		}

		gl.disable(gl.BLEND);
		gl.enable(gl.DEPTH_TEST)
		gl.depthMask(true);
	}

	function animate() {
		if(glfw.windowShouldClose(window) || glfw.getKey(window, glfw.KEY_ESCAPE)) {
			shutdown();
		} else {
			setImmediate(animate)
		}

		let t1 = glfw.getTime();
		dt = t1-t;
		if (Math.floor(t1)>Math.floor(t)) {
			//console.log("tick")
			
		}
		fps += 0.1*((1/dt)-fps);
		t = t1;
		glfw.setWindowTitle(window, `fps ${fps}`);
		glfw.makeContextCurrent(window);

		// simulation:
		tod.update(t, dt);
		
		worker.postMessage({msg:"tick", t:t, dt:dt, shared:[wshared, wshared2]})
		//console.log(wshared[0], wshared2[0])

		gl.bindBuffer(gl.ARRAY_BUFFER, snakeInstanceBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, snakeInstanceData, gl.DYNAMIC_DRAW);
		gl.bindBuffer(gl.ARRAY_BUFFER, beetleInstanceBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, beetleInstanceData, gl.DYNAMIC_DRAW);
		gl.bindBuffer(gl.ARRAY_BUFFER, pointsBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, particleData, gl.DYNAMIC_DRAW);
		gl.bindBuffer(gl.ARRAY_BUFFER, ghostBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, ghostData, gl.DYNAMIC_DRAW);

		gl.bindBuffer(gl.ARRAY_BUFFER, isoBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, isovertices, gl.DYNAMIC_DRAW);

		//if(wsize) console.log("FB size: "+wsize.width+', '+wsize.height);
		if (usevr) {
			vr.update();
			let inputs = vr.inputSources()
			let hmd, left_hand, right_hand;
			for (let input of inputs) {
				if (input.targetRayMode == "gaze") {
					hmd = input;
				} else if (input.handedness == "left") {
					left_hand = input;
				} else if (input.handedness == "right") {
					right_hand = input;
				}
			}
		}
		
		// render to our targetTexture by binding the framebuffer
		gl.bindFramebuffer(gl.FRAMEBUFFER, fbo.id);
		{
			gl.viewport(0, 0, fbo.width, fbo.height);
			gl.enable(gl.DEPTH_TEST)
			gl.depthMask(true)
			gl.clearColor(0, 0, 0, 1);
			gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

			if (usevr) {

				for (let i=0; i<2; i++) {
					gl.viewport(i * fbo.width/2, 0, fbo.width/2, fbo.height);

					// Compute the matrix
					let viewmatrix = mat4.create();
					//mat4.lookAt(viewmatrix, [0, 0, 3], [0, 0, 0], [0, 1, 0]);
					vr.getView(i, viewmatrix);
					mat4.translate(viewmatrix, viewmatrix, [-3, 0, -3])

					let projmatrix = mat4.create();
					//mat4.perspective(projmatrix, Math.PI/2, fbo.width/fbo.height, 0.01, 10);
					vr.getProjection(i, projmatrix);

					let modelmatrix = mat4.create();
					let axis = vec3.fromValues(Math.sin(t), 1., 0.);
					vec3.normalize(axis, axis);
					//mat4.rotate(modelmatrix, modelmatrix, t, axis)

					renderEye({
						projmatrix: projmatrix,
						viewmatrix: viewmatrix,
						modelmatrix: modelmatrix,
						fbo: fbo,
					})
				}
			} else {

				// Compute the matrix
				let viewmatrix = mat4.create();
				mat4.lookAt(viewmatrix, [3, 1.5, 3], [3, 1.5, 4], [0, 1, 0]);

				let projmatrix = mat4.create();
				mat4.perspective(projmatrix, Math.PI/2, fbo.width/fbo.height, 0.01, 10);

				let modelmatrix = mat4.create();

				renderEye({
					projmatrix: projmatrix,
					viewmatrix: viewmatrix,
					modelmatrix: modelmatrix,
					fbo: fbo,
				})
			}
		}
		gl.bindFramebuffer(gl.FRAMEBUFFER, null);

		if (usevr) vr.submit(fbo.colorTexture)

		// Get window size (may be different than the requested size)
		let dim = glfw.getFramebufferSize(window);
		gl.viewport(0, 0, dim[0], dim[1]);
		gl.enable(gl.DEPTH_TEST)
		gl.depthMask(true)
		gl.clearColor(0.2, 0.2, 0.2, 1);
		gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

		// render the cube with the texture we just rendered to
		gl.bindTexture(gl.TEXTURE_2D, fbo.colorTexture);
		quadprogram.begin();
		quadprogram.uniform("u_scale", 1, 1);
		quad.bind().draw().unbind();
		quadprogram.end();

		// Swap buffers
		glfw.swapBuffers(window);
		glfw.pollEvents();
		framecount++;

		//console.log(counts)
	}

	function shutdown() {
		//saveSimState()
		if (usevr) vr.connect(false);
		// Close OpenGL window and terminate GLFW
		glfw.destroyWindow(window);
		glfw.terminate();
		process.exit(0);
	}

	animate();
}

start()