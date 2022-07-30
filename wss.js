const assert = require("assert"),
	os = require("os"),
	path = require("path")
const http = require('http');

const ws = require('ws');
const express = require("express");

function makeServer(handleMessage, public_path = path.join(__dirname, 'public'), PORT = process.env.PORT || 3000) {
	const app = express();
	app.use(express.static(public_path));
	const server = http.createServer(app)
	const wss = new ws.Server({ server: server });
	wss.on('connection', (socket) => {
		console.log('client connected');
		socket.send(JSON.stringify({ cmd:"handshake" }))
		socket.on('close', () => { console.log('client disconnected'); });
		socket.on('message', handleMessage)
	});
	
	server.listen(PORT, () => console.log(`Server listening on port: ${PORT}`));

	function broadcast(msg) {
		wss.clients.forEach(function (client) {
			if (client.readyState == ws.OPEN) {
				client.send( msg );
			}
		});
	}

	return broadcast
}

module.exports = makeServer