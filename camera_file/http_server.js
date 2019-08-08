const net = require('net');
var StaticServer = require('static-server');
var server = new StaticServer({
  rootPath: '.',            // required, the root of the server file tree
  port: 8888,               // required, the port to listen
  host: '10.168.75.94',       // optional, defaults to any interface
  templates: {
    index: 'index.html'      // optional, defaults to 'index.html'
  }
});

server.start(function () {
  console.log('Server listening to ', server.host, ":",server.port);
});
