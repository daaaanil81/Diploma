import Net from('net');
// The port number and hostname of the server.
const port = 8444;
const host = '10.168.94.75';

// Create a new TCP client.
const client = new Net.Socket();
// Send a connection request to the server.
client.connect(({ port: port, host: host }), function() {
    console.log('TCP connection established with the server.');
});
