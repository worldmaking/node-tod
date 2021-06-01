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

const events = require("./events.js")

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
	// Enable vertical sync (on cards that support it)
	glfw.swapInterval(usevr ? 0 : 1); // 0 for vsync off
	glfw.setWindowPos(window, 40, 40)

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
			//reload();
			break;
			default: console.log("key", key, down, shift, ctrl)
		}
		
		//console.log(key, down, mod);
	})

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

	let t = glfw.getTime();
	let framecount = 0;
	let fps = 60;
	let dt = 1/fps;

	let viewmatrix = mat4.create();
	let projmatrix = mat4.create();

	function renderEye(settings) {
		gl.clearColor(0, 0, 0, 1);
		gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

		let state = {
			projmatrix: projmatrix,
			viewmatrix: viewmatrix,
		}

		// insert scene rendering here
		gl.depthMask(false)
		// backdrop pass:
		events.get("draw:background").forEach(f => f(state))
		gl.depthMask(true)
		gl.enable(gl.DEPTH_TEST)
		// solid objects:
		events.get("draw:opaque").forEach(f => f(state))

		gl.enable(gl.BLEND);
		gl.blendFunc(gl.SRC_ALPHA, gl.ONE);
		gl.depthMask(false)
		// transparent objects:
		events.get("draw:transparent").forEach(f => f(state))

		gl.disable(gl.BLEND);
		gl.enable(gl.DEPTH_TEST)
		gl.depthMask(true);
	}

	function animate() {
		glfw.pollEvents();
		if(glfw.windowShouldClose(window) || glfw.getKey(window, glfw.KEY_ESCAPE)) {
			shutdown();
		}
		let t1 = glfw.getTime();
		dt = t1-t;
		fps += 0.1*((1/dt)-fps);
		t = t1;
		glfw.setWindowTitle(window, `fps ${fps}`);

		// run simulation
		events.get("animate").forEach(f => f(dt, t))
		// worker.postMessage({msg:"tick", t:t, dt:dt, shared:[wshared, wshared2]})

		glfw.makeContextCurrent(window);
		// submit buffers etc. to gpu

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
					vr.getView(i, viewmatrix);
					vr.getProjection(i, projmatrix);
					renderEye()
				}
			} else {
				// Compute the matrix
				mat4.lookAt(viewmatrix, [0, 1.5, 1], [0, 1.5, -1], [0, 1, 0]);
				mat4.perspective(projmatrix, Math.PI/2, fbo.width/fbo.height, 0.01, 10);
				//mat4.translate(viewmatrix, viewmatrix, [0, 0, -3])
				renderEye()
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
		let finaltex = fbo.colorTexture
		// insert postfx rendering here
		gl.bindTexture(gl.TEXTURE_2D, finaltex);
		quadprogram.begin();
		quadprogram.uniform("u_scale", 1, 1);
		quad.bind().draw().unbind();
		quadprogram.end();

		// Swap buffers
		glfw.swapBuffers(window);
		framecount++;

		setImmediate(animate)
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

	console.log("started")
}

start()

////////////////////////////////////////////////////////////////////////////
console.log("init")

// how to hot reload a module:
function nocache(module_path) {
	let fullpath = require.resolve(module_path)
	console.log(fullpath)
	// whenever the module changes,
	fs.watch(fullpath).on("change", () => {
		//fs.statSync(module_path)
		console.log("reloading", module_path)
		// call module's dispose handler:
		let m = require.cache[fullpath].exports
		if (m.dispose) m.dispose()
		delete require.cache[fullpath]
		require(fullpath)
	})
	require(fullpath)
}

//let script = fs.readFileSync("demo.js", "utf-8")
// how can we avoid needing globals so that new Function can work?
//for (let i=0; i<10; i++) 
//eval(script)

nocache("./demo.js")
//require("./demo.js")

// global.foo = "graham"
// let f = new Function("console.log(foo, this)")
// f()
