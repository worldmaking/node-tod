

let events = {
	sets: {},

	get(name) {
		if (!this.sets[name]) this.sets[name] = new Set()
		return this.sets[name]
	},

	add(name, obj) {
		this.get(name).add(obj)
		return this
	},

	remove(name, obj) {
		this.get(name).delete(obj)
		return this
	},
}

module.exports = events