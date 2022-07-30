const { performance } = require('perf_hooks');
const { workerData, parentPort } = require('worker_threads')

require("./events.js")

console.log("hello from worker", workerData)
parentPort.postMessage({ msg: "hello "+workerData })
parentPort.on("message", msg => {
	console.log("received", msg)
})

// run worker thread:
let t = performance.now()
let FPS = 60
let avgFPS = FPS
let MSPF = 1000/FPS // ideal ms per step
let dt = 1/FPS

function simulate() {
	let t0 = performance.now()
	let dt = t0-t
	t = t0
	

	let used_ms = performance.now() - t0
	let used_ratio = used_ms / dt
	avgFPS += 0.1*(1000/dt - avgFPS)
	console.log("sim%", used_ms, dt)

	if (1) {
		setTimeout(simulate, 0)
	} else {
		setImmediate(simulate)
	}
}

simulate()