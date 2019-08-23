var connection = new WebSocket('ws://10.168.75.95:8555'); // tcp server on c/c++
connection.onopen = function () {
  connection.send('10.168.0.85'); // Send the message 'Ping' to the server
  console.log("Send");
};
connection.onclose = function(event) {
};
connection.onmessage = function(event) {
  console.log(event.data);
};
