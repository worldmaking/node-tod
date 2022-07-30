const assert = require("assert"),
	fs = require("fs")

function hotload(module_path) {
	let fullpath = require.resolve(module_path)
	function run() {
		try {
			// call module's dispose handler:
			let m = require.cache[fullpath]
			if (m) {
				if (m.exports.dispose) m.exports.dispose()
				delete require.cache[fullpath]
			}
			console.log("loading", fullpath, new Date())
			require(fullpath)
			console.log("loaded", fullpath, new Date())
		} catch(e) {
			console.error(e)
		}
	}
	// whenever the module changes,
	fs.watch(fullpath).on("change", () => run())
	run()
}

module.exports = hotload