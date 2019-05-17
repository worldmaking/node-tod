
const glfw = require("glfw-raub")
const { Window } = glfw;

//const gl = require("webgl-raub");
const { gl } = require("3d-core-raub");


const w1 = new Window({ title: 'GLFW Simple Test 1' });
const w2 = new Window({ title: 'GLFW Simple Test 2' });


// testing events
w1.on('mousemove', e => console.log(`[#1 mousemove] ${e.x}, ${e.y}`));

w2.on('mousemove', e => console.log(`[#2 mousemove] ${e.x}, ${e.y}`));

console.log(w1.version);

w1.makeCurrent();
gl.enable(gl.POINT_SPRITE); // GL_POINT_SPRITE 0x8861
gl.enable(0x8642); // GL_VERTEX_PROGRAM_POINT_SIZE
gl.enable(0x8862); // GL_COORD_REPLACE

w2.makeCurrent();
gl.enable(gl.POINT_SPRITE); // GL_POINT_SPRITE 0x8861
gl.enable(0x8642); // GL_VERTEX_PROGRAM_POINT_SIZE
gl.enable(0x8862); // GL_COORD_REPLACE


const draw = () => {
	
	w1.makeCurrent();
	const wsize1 = w1.framebufferSize;
	glfw.testScene(wsize1.width, wsize1.height);
	w1.swapBuffers();
	
	w2.makeCurrent();
	const wsize2 = w2.framebufferSize;
	glfw.testScene(wsize2.width, wsize2.height);
	w2.swapBuffers();
	
	glfw.pollEvents();
	
};


const animate = () => {
	
	if ( ! (
		w1.shouldClose || w2.shouldClose ||
		w1.getKey(glfw.KEY_ESCAPE) || w2.getKey(glfw.KEY_ESCAPE)
	) ) {
		
		draw();
		setTimeout(animate, 16);
		
	} else {
		// Close OpenGL window and terminate GLFW
		w1.destroy();
		w2.destroy();
		
		glfw.terminate();
		
		process.exit(0);
	}
	
};


animate();