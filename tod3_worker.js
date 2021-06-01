

const { workerData, parentPort } = require('worker_threads')

console.log("hello from worker", workerData)
parentPort.postMessage({ msg: "hello "+workerData })
parentPort.on("message", msg => {
	let shared = msg.shared
	//console.log("received", msg)
	shared[0][0]++
	shared[1][0]++
	//console.log(shared[0][0], shared[1][0])
})