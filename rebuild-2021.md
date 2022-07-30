
# how much can be made generic?

E.g. move out window/vr etc handling to helper file?
- actually most of that is now wrapped up in a single "start()" function, so this seems viable
- it just needs the events registration to connect things together

# how much can we move from C++ to JS worker threads?

See ToD:
- main thread:
	- updates things that need to be smooth per-frame, like basic motion, recomputing isosurface, etc.
- sim thread:
	- runs all creature simulations: collision handling & interactions, cognition, etc., updating velocities and angular velocities accordingly
- audio thread:
	- mostly read sim state & write audio chirps

Can share binary data between main/worker using SharedArrayBuffer
Can share binary data between JS and C using external_arraybuffer
Can't do both yet, but can use C to load the *same* external_arraybuffer in both threads
BUT: any thread can pass in arraybuffer ptrs to C functions (sees it as a regular arraybuffer)
So, do allocations in Node.js if you can (using SharedArrayBuffer to share between threads); else allocate as a static ptr in library (visible to all threads) and expose as external_arraybuffer

Either way, when the shared memory layout has to change, then *everything* that uses it has to be restarted. We can however fragment the shared memory into parts, e.g.:
workerData = {
	buf1: new Float32Array(sharedArrayBuffer1),
	buf2: new Float32Array(sharedArrayBuffer2),
}
So, all snake data in one, all beetle data in another, etc.
Moreover, we *can* also pass this kind of structure in postMessage between threads, so there's virtually no overhead. That means we should be fine with granular structures.

## How to hotload with worker thread?

- idea 1:
	- worker thread has its own `events` system, completely independent of main thread; this is the message-passing graph
	- each user-module must register events in main thread but also worker thread; to do the latter, code must be evaluated and events registered in the worker thread (including the unloading)
	- project modules are also hotloaded in worker thread, and use `isMainThread` to install different code appropriately?
- it is a bit like separation between cpu and gpu really; both have to reload in sync with each other
	- tricky part: must wait for thread depenedents to unload before unloading main

# scene-graph-ify

Each distinct kind of thing in the world should be defined in its own file(s), so that we can add & remove items, or even hot reload them, without affecting all others
It's a bit tricky because the relevant concerns are distributed to different kinds:
- shaders (reload when context changes) (IO layout strongly tied to buffer attribute structure etc.)
	- might also refer to textures
- geometry (recreate when context changes, resubmit when attributes changed) (strongly tied to shaders)
- motion (JS/C main thread update, will likely update geometry buffer attributes)
- simulation (JS/C sim thread update, will read from & update parameters used in motion)
	- likely references other data structures e.g. fields, collisions, etc.

Example: ToD beetles
- some binary data that was essentially ONLY for rendering purposes, and was sent to shaders as an instanced array of attributes. vec4 etc. for position, orientation, scale, colour, age. Made as compact as possible for performance. Strict types and layout for shader.
- some binary data was used for motion, 
	for each beetle (if active): update orientation, position, update voxel collision; then update rendering binary data & count, and update audio data & count
	minimal reference to global state (world dim, hashspace)
- some binary data was used for simulation,
	for each beetle (if active): handle death-by-energy, fluid flow, sensing, decision-making, colour change, growth, eating nearest particle, change accel, accumulate forces to vel, change turn, reproduce?, etc.
	maybe rebirth
	actually none of this needs to be binary
	refers to external fields: fluid, collision-space


# making it more modular:

- things that happen at particular moments, use a notify() system
	- simplest is to loop over a list/set
		object.on("draw:transparent", fun) adds fun to the appropriate set

# If it was a Max patcher...

- a data-document (the patcher representation) is what defines the world, incl. assets, scene graph, relationships, control & data flows, events, references, etc.
	- 

