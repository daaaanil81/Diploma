var static = require('node-static');
var fs = require('fs');
var file = new static.Server();
var app = require('http').createServer(handler);
var io = require('socket.io')(app);
var clientsInRoom = null;
var numClients = null;
app.listen(7000);

function handler (req, res) {
  file.serve(req, res);
}


io.on('connection', function (socket) {
  var ipClient = socket.handshake.address;
  console.log("IP of client: " + ipClient );
  socket.on('message', function(message) {
    socket.broadcast.emit('message', message);
  });
  socket.on('create or join', function(room) {
    clientsInRoom = io.sockets.adapter.rooms[room];
    numClients = clientsInRoom ? Object.keys(clientsInRoom.sockets).length : 0;
    if (numClients == 0) {
      socket.join(room);
      socket.emit('create', room);
      console.log("First client");
    } else if (numClients == 1) {
      socket.join(room);
      socket.emit('join', room);
      console.log("Second client");
      io.sockets.emit('ready', room);
    } else {
      socket.emit('full', room);
    }
  });
  socket.on('start', function (time) {
      socket.broadcast.emit('start', time);
  });
});
