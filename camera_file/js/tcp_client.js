const net = require('net');
const port = 8444;
const host = '10.168.75.94';
const socket = net.connect(port, host);


// Create a new TCP client.
const client = new Net.Socket();
// Send a connection request to the server.
client.connect(({ port: port, host: host }), function() {
    console.log('TCP connection established with the server.');
});
