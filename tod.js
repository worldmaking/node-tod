
const glfw = require("glfw-raub")
const { Window } = glfw;
//const glfw = require("node-glfw")
//const { Screen, loop, gl, Points } = require('../index');
//const { gl } = require("3d-core-raub")
//const gl = require('webgl-raub');
const { vec2, vec3, vec4, quat, mat2, mat2d, mat3, mat4} = require("gl-matrix")
const gl = require('../node-gles3/index.js') 
const glutils = require('../node-gles3/glutils.js');
const fs = require("fs")
const assert = require("assert")
const EventEmitter = require('events');

const tod = require('bindings')('tod.node');


if (!glfw.init()) {
	console.log("Failed to initialize glfw");
	process.exit(-1);
}
let version = glfw.getVersion();
console.log('glfw ' + version.major + '.' + version.minor + '.' + version.rev);
console.log('glfw version-string: ' + glfw.getVersionString());

let monitors = glfw.getMonitors();
//console.log(monitors)

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

const WORLD_DIM = [6, 3, 6]
const NUM_PARTICLES = 20000;
const NUM_SNAKE_SEGMENTS = 136;
const NUM_BEETLES = 2048;
const NUM_VOXELS = 32 * 16 * 32;

// derive from header
let beetleBufferByteStride = 16*4
let snakeBufferByteStride = 16*4
let pointsBufferByteStride = 8*4
let isoBufferStride = 6*4

let shared = tod.setup()

// TODO: derive this from struct header?
let byteoffset = 0;
let snakeInstanceData = new Float32Array(shared, byteoffset, NUM_SNAKE_SEGMENTS * 16)
byteoffset += snakeInstanceData.byteLength
let beetleInstanceData = new Float32Array(shared, byteoffset, NUM_BEETLES * 16)
byteoffset += beetleInstanceData.byteLength
let particleData = new Float32Array(shared, byteoffset, NUM_PARTICLES * 8)
byteoffset += particleData.byteLength

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

let beetleBufferFields = [
	{ name:"a_orientation", components:4, type:gl.FLOAT, byteoffset:0*4 },
	{ name:"a_color", components:4, type:gl.FLOAT, byteoffset:4*4 },
	{ name:"a_location", components:3, type:gl.FLOAT, byteoffset:8*4 },
	{ name:"a_age", components:1, type:gl.FLOAT, byteoffset:11*4 },
	{ name:"a_scale", components:3, type:gl.FLOAT, byteoffset:12*4 },
]
let snakeBufferFields = [
	{ name:"a_orientation", components:4, type:gl.FLOAT, byteoffset:0*4 },
	{ name:"a_color", components:4, type:gl.FLOAT, byteoffset:4*4 },
	{ name:"a_location", components:3, type:gl.FLOAT, byteoffset:8*4 },
	{ name:"a_phase", components:1, type:gl.FLOAT, byteoffset:11*4 },
	{ name:"a_scale", components:3, type:gl.FLOAT, byteoffset:12*4 },
]
let particleBufferFields = [
	{ name:"a_location", components:3, type:gl.FLOAT, byteoffset:0*4 },
	{ name:"a_color", components:4, type:gl.FLOAT, byteoffset:4*4 }
]
let isoBufferFields = [
	{ name:"a_position", components:3, type:gl.FLOAT, byteoffset:0*3 },
	{ name:"a_normal", components:3, type:gl.FLOAT, byteoffset:4*3 }
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



function Renderer(config) {

	// Open OpenGL window
	glfw.defaultWindowHints();
	glfw.windowHint(glfw.CONTEXT_VERSION_MAJOR, 3);
	glfw.windowHint(glfw.CONTEXT_VERSION_MINOR, 3);
	glfw.windowHint(glfw.OPENGL_FORWARD_COMPAT, 1);
	glfw.windowHint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE);

	// glfw.windowHint(glfw.RESIZABLE, 1);
	// glfw.windowHint(glfw.VISIBLE, 1);
	// // glfw.windowHint(glfw.AUTO_ICONIFY, this._autoIconify ? glfw.TRUE : glfw.FALSE);
	// // glfw.windowHint(glfw.SAMPLES, this._msaa);
	// glfw.windowHint(glfw.REFRESH_RATE, 0);

	// glfw.windowHint(glfw.RED_BITS, 8);
	// glfw.windowHint(glfw.GREEN_BITS, 8);
	// glfw.windowHint(glfw.BLUE_BITS, 8);
	// glfw.windowHint(glfw.DEPTH_BITS, 24);
	// glfw.windowHint(glfw.DOUBLEBUFFER, glfw.TRUE);

	if (0) {
		this.window = new Window({ 
			title: config.title,
			width: config.dim[0],
			height: config.dim[1],
			vsync: config.sync,
			//mode: config.mode || 'windowed', // 'fullscreen'
			autoIconify: false,
			display: config.monitor || 0,
		});
		this.window_handle = this.window.handle
	} else {
	
		let emitter = new EventEmitter();

		if (config.mode == "fullscreen") {
			glfw.windowHint(glfw.DECORATED, glfw.FALSE);
			this.window_handle = glfw.createWindow(config.dim[0], config.dim[1], emitter, config.title); //, config.display)
		} else if (config.mode == "borderless") {
			glfw.windowHint(glfw.DECORATED, glfw.FALSE);
			const monitor = monitors[config.monitor];
			this.window_handle = glfw.createWindow(config.dim[0], config.dim[1], emitter, config.title) //, config.display)
			glfw.setWindowPos(this.window_handle, monitor.pos_x, monitor.pos_y);
			glfw.setWindowSize(this.window_handle, monitor.width, monitor.height);
		} else {
			glfw.windowHint(glfw.DECORATED, glfw.TRUE);
			this.window_handle = glfw.createWindow(config.dim[0], config.dim[1], emitter, config.title)
			glfw.setWindowPos(this.window_handle, config.pos[0], config.pos[1]);
		}
	}
	//this.window = glfw.CreateWindow(config.dim[0], config.dim[1], config.title || ""); //, monitors.length-1);
	if (!this.window_handle) {
		console.log("Failed to open glfw window");
		glfw.terminate();
		process.exit(-1);
	}
	//this.window.makeCurrent()
	glfw.makeContextCurrent(this.window_handle);
	glfw.swapInterval(config.vsync ? 1 : 0);
	console.log(gl.glewInit()); // need to do this for GLES3 symbols


	//glfw.SwapInterval(config.sync ? 1 : 0); 

	console.log('GL ' + glfw.getWindowAttrib(this.window_handle, glfw.CONTEXT_VERSION_MAJOR) + '.' + glfw.getWindowAttrib(this.window_handle, glfw.CONTEXT_VERSION_MINOR) + '.' + glfw.getWindowAttrib(this.window_handle, glfw.CONTEXT_REVISION) + " Profile: " + glfw.getWindowAttrib(this.window_handle, glfw.OPENGL_PROFILE));

	// TODO
	/*
	gl::Texture3d::Format format3d;
	// format3d.maxAnisotropy( ? )
	format3d.internalFormat(GL_RGB32F) // GL_RGBA8_SNORM GL_RGBA32F
	.wrap(GL_CLAMP_TO_BORDER)	// maybe GL_CLAMP here?
	.mipmap(true)		// ?
	.minFilter(GL_LINEAR)
	.magFilter(GL_LINEAR)
	.label("goo");
	format3d.setDataType(GL_FLOAT);
	
	mGooTex = gl::Texture3d::create(DIM, DIM, DIM, format3d);
	*/



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

	// TODO: Ghost

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
	let wall = glutils.createVao(gl, glutils.makeCube(0), wallprogram.id);


	let quadprogram = glutils.makeProgram(gl,
		fs.readFileSync("shaders/q.vert", "utf-8"),
		fs.readFileSync("shaders/q.frag", "utf-8")
	);
	let quad = glutils.createVao(gl, glutils.makeQuad(), quadprogram.id);

	let fbo = glutils.makeFboWithDepth(gl, 1920, 1080)

	this.draw = function(t, dt, framecount) {

		glfw.setWindowTitle(this.window_handle, `fps ${fps}`);
		glfw.makeContextCurrent(this.window_handle);
		let dim = glfw.getFramebufferSize(this.window_handle);
		let aspect = dim.width / dim.height

		// Compute the matrixs
		let lightposition = vec3.fromValues(world.width/2, world.height, world.depth/2)
		let viewmatrix = mat4.create();
		let projmatrix = mat4.create();
		let projmatrix_walls = mat4.create();
		let center = [world.width/2, world.height/2, world.depth/2];
		let eye_height = 1.55

		let ha0 = 0.5;
		let wa0 = ha0 * aspect; // * 2.; // x2 because we render to a 2 screen panorama
		let strafex = 0.;
		{
			let parallax_rate = 0.01
			let parallax_range = 0.01
			strafex = parallax_range * Math.sin(t * parallax_rate * 10.);
		}
		let strafey = 0.5;
		let nearclip_walls = WORLD_DIM[2] + 0.01;
		let farclip_walls = nearclip_walls + WORLD_DIM[2] - 0.02;
		let farclip = farclip_walls + WORLD_DIM[2];
		let nearclip = nearclip_walls - WORLD_DIM[2];

		if (config.id == 0) {
			// screen 0
			projection(projmatrix,
				[0, 0, 0],		// bottom-left screen coordinate
				[wa0, 0, 0],		// bottom-right screen coordinate
				[0, ha0, 0],		// top-left screen coordinate
				[wa0 - strafex, ha0 * (strafey), 1],	// eye coordinate
				nearclip, farclip
			);
			projection(projmatrix_walls,
				[0, 0, 0],		// bottom-left screen coordinate
				[wa0, 0, 0],		// bottom-right screen coordinate
				[0, ha0, 0],		// top-left screen coordinate
				[wa0 - strafex, ha0 * (strafey), 1],	// eye coordinate
				nearclip_walls, farclip_walls
			);
			mat4.lookAt(viewmatrix, 
				// [strafex*WORLD_DIM[0], eye_height, -WORLD_DIM[2]],
				// [strafex*WORLD_DIM[0], eye_height, WORLD_DIM[2]],
				[WORLD_DIM[0]-strafex*WORLD_DIM[0], eye_height, WORLD_DIM[2]*2],
				[WORLD_DIM[0]-strafex*WORLD_DIM[0], eye_height, WORLD_DIM[2]],
				[0, 1, 0]
			);
		} else if (config.id == 1) {
			
			// screen 1
			projection(projmatrix,
				[0, 0, 0],		// bottom-left screen coordinate
				[wa0, 0, 0],		// bottom-right screen coordinate
				[0, ha0, 0],		// top-left screen coordinate
				[-strafex, ha0 * (strafey), 1],	// eye coordinate
				nearclip, farclip
			);
			projection(projmatrix_walls,
				[0, 0, 0],		// bottom-left screen coordinate
				[wa0, 0, 0],		// bottom-right screen coordinate
				[0, ha0, 0],		// top-left screen coordinate
				[-strafex, ha0 * (strafey), 1],	// eye coordinate
				nearclip_walls, farclip_walls
			);
			mat4.lookAt(viewmatrix, 
				[-WORLD_DIM[2], eye_height, -strafex*WORLD_DIM[0]],
				[WORLD_DIM[2], eye_height, -strafex*WORLD_DIM[0]],
				[0, 1, 0]
			);
		} else {

			mat4.lookAt(viewmatrix, 
				[0, 0, 1], 
				[0, 0, 0], 
				[0, 1, 0]
			);
			//mat4.translate(viewmatrix, viewmatrix, vec3.fromValues(0, 0, -world.depth/2));
			mat4.rotate(viewmatrix, viewmatrix, Math.PI*config.id + t*0.1, vec3.fromValues(0, 1, 0))
			mat4.translate(viewmatrix, viewmatrix, vec3.fromValues(-world.width/2, -world.height/2, -world.depth/2));
			//mat4.translate(viewmatrix, viewmatrix, center[0], center[1], center[2]);
			mat4.perspective(projmatrix, Math.PI/2, dim.width/dim.height,nearclip, farclip);
			mat4.perspective(projmatrix_walls, Math.PI/2, dim.width/dim.height, nearclip_walls, farclip_walls)
			projmatrix_walls = projmatrix;
		}

		// upload gpu data

		// TODO: does this need to be per screen, or could it be once only?
		gl.bindBuffer(gl.ARRAY_BUFFER, snakeInstanceBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, snakeInstanceData, gl.DYNAMIC_DRAW);
		gl.bindBuffer(gl.ARRAY_BUFFER, beetleInstanceBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, beetleInstanceData, gl.DYNAMIC_DRAW);
		gl.bindBuffer(gl.ARRAY_BUFFER, pointsBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, particleData, gl.DYNAMIC_DRAW);


		gl.bindBuffer(gl.ARRAY_BUFFER, isoBuffer);
		gl.bufferData(gl.ARRAY_BUFFER, isovertices, gl.DYNAMIC_DRAW);
	
		//fbo.begin()
		// render to our targetTexture by binding the framebuffer
		gl.bindFramebuffer(gl.FRAMEBUFFER, fbo.id);
		{
			gl.viewport(0, 0, fbo.width, fbo.height);
			gl.enable(gl.DEPTH_TEST)
			gl.depthMask(true)
			gl.clearColor(0, 0, 0, 1);
			gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

			wallprogram.begin();
			wallprogram.uniform("u_world_dim", world.width, world.height, world.depth);
			wallprogram.uniform("u_viewmatrix", viewmatrix);
			wallprogram.uniform("u_projmatrix", projmatrix_walls);
			wall.bind().draw().unbind();
			wallprogram.end();

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

			// TODO: link mGooTex
			isoprogram.begin();
			isoprogram.uniform("u_viewmatrix", viewmatrix);
			isoprogram.uniform("u_projmatrix", projmatrix);
			isoprogram.uniform("u_world_dim", world.dim[0], world.dim[1], world.dim[2]);
			isoprogram.uniform("u_now", t);
			isoprogram.uniform("u_alpha", 0.2);
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

			pointprogram.begin();
			pointprogram.uniform("u_pixelSize", dim.height * 0.02);
			pointprogram.uniform("u_viewmatrix", viewmatrix);
			pointprogram.uniform("u_projmatrix", projmatrix);
			pointsVao.bind()
			pointsVao.drawPoints(NUM_PARTICLES)
			pointsVao.unbind();
			pointprogram.end();
			
			gl.disable(gl.BLEND);
			gl.depthMask(true);
		}
		//fbo.end();
		gl.bindFramebuffer(gl.FRAMEBUFFER, null);

		gl.viewport(0, 0, dim.width, dim.height);
		gl.enable(gl.DEPTH_TEST)
		gl.depthMask(true)
		gl.clearColor(0, 0, 0, 1);
		gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

		// render the cube with the texture we just rendered to
		gl.bindTexture(gl.TEXTURE_2D, fbo.colorTexture);
		quadprogram.begin();
		quadprogram.uniform("bl_fade", config.bl_fade[0], config.bl_fade[1]);
		quadprogram.uniform("tr_fade", config.tr_fade[0], config.tr_fade[1]);
		quadprogram.uniform("tl", config.tl[0], config.tl[1]);
		quadprogram.uniform("tr", config.tr[0], config.tr[1]);
		quadprogram.uniform("bl", config.bl[0], config.bl[1]);
		quadprogram.uniform("br", config.br[0], config.br[1]);
		
		quad.bind().draw().unbind();
		quadprogram.end();
	}
}

// Only sync one at most one window (per monitor?), otherwise the frame-rate will be halved... 
let renders = [
	new Renderer({ 
		dim: [1920/3, 1200/3], pos: [40, 40], 
		monitor: (0 % monitors.length),
		sync: false, id: 0,
		tl: [-0.1, 0.1], tr: [0.1, 0.1],
		bl: [-0.1,-0.1], br: [0.1,-0.1],
		bl_fade: [0.001, 0.001], tr_fade: [0.001, 0.001],
	}),
	new Renderer({ 
		dim: [1920/3, 1200/3], pos: [1920/3, 40], 
		monitor: (1 % monitors.length), 
		//mode: 'borderless',
		sync: false, id: 1,
		tl: [-0.1, 0.1], tr: [0.1, 0.1],
		bl: [-0.1,-0.1], br: [0.1,-0.1],
		bl_fade: [0.001, 0.001], tr_fade: [0.001, 0.001], 
	}),
	new Renderer({ 
		dim: [1920/3, 1200/3], pos: [40, 100 + 1200/3], 
		monitor: (2 % monitors.length),
		sync: false, id: 2,
		tl: [-0.1, 0.1], tr: [0.1, 0.1],
		bl: [-0.1,-0.1], br: [0.1,-0.1],
		bl_fade: [0.001, 0.001], tr_fade: [0.001, 0.001],
	}),
]

let t = glfw.getTime();
let framecount = 0;
let fps = 60;

function update() {
	glfw.pollEvents();
	for (let render of renders) {
		let win = render.window_handle
		if (glfw.getKey(win, glfw.KEY_ESCAPE) || glfw.windowShouldClose(win)) {
			return;
		}
	}

	let t1 = glfw.getTime();
	let dt = t1-t;
	fps += 0.1*((1/dt)-fps);
	t = t1;

	glutils.requestAnimationFrame(update, 1);

	// simulation:
	tod.update(t, dt);

	//process.exit();

	/*
	for (let i=0; i<snakeInstanceData.length; i+=16) {
		// quat:
		let instance = snakeInstanceData.subarray(i, i+16);
		let phase = instance.subarray(11, 12);
		phase[0] = (phase[0] + dt) % 1.; 
		//if (i==0) console.log(phase[0])
	}
	for (let i=0; i<beetleInstanceData.length; i+=16) {
		// quat:
		let instance = beetleInstanceData.subarray(i, i+16);
		let phase = instance.subarray(11, 12);
		phase[0] = (phase[0] + (dt*3.)) % 1.; 
		//if (i==0) console.log(phase[0])
	}	*/

	for (let render of renders) {
		render.draw(t, dt, framecount)
		
		// Swap buffers
		glfw.swapBuffers(render.window_handle);
	}


	//console.log(particleData)
	framecount++;
}

update();

