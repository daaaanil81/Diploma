'use strict';
var localVideo = document.getElementById("localVideo");
var remoteVideo = document.getElementById("remoteVideo");
//const configuration = {iceServers: [{urls: 'stun:stun.l.google.com:19302'}]};
var t = 1;
var localStream;
var remoteStream;
var localConnection;
var remoteConnection;
var server = null;
var status;

var localDescription;
var remoteDescription;

var localICeCandidate;
var remoteIceCandidate;
var get_Offer = false;

const hdConstraints = {
  video: {
    width: {
      min: 1280
    },
    height: {
      min: 720
    }
  }
}

var room = 'VideoRoom';
var socket = io.connect('https://10.168.75.94:7000');

if (room !== '') {
  socket.emit('create or join', room);
  console.log('Attempted to create or  join room', room);
}

socket.on('create', function(room) {
  console.log('Created room ' + room);
  status = "Camera";
  console.log("StartCamera");
  navigator.mediaDevices.getUserMedia(hdConstraints).then(setLocalStream).catch(localStreamError);
});

socket.on('full', function(room) {
  console.log('Room ' + room + ' is full');
});

socket.on('join', function (room){
  console.log('Client joined.');
  status = "Client";
});

socket.on('message', function(message){
  if (message.type == "candidate"){
    console.log("Get remote candidate");
    console.log(message.candidate);
    var candidate = new RTCIceCandidate({sdpMLineIndex: message.candidate.sdpMLineIndex, candidate: message.candidate.candidate});
    localICeCandidate = candidate;
    localConnection.addIceCandidate(candidate)
    .catch(function() {"Error in candidate other user"});
  }
  if (message.type == "offer"){
      console.log("Offer other user \n" + message.description);
      localConnection.setRemoteDescription(new RTCSessionDescription(message.description));
      remoteDescription = message.description;
      console.log("Send answer");
      localConnection.createAnswer(sRemoteDescription,onError);
  }
  if (message.type == "answer") {
	  if(status == "Client")
	  {
		  console.log("Answer client bad!");
	  }
	  else
	  {

    console.log("Answer other user \n" + message.description);
    localConnection.setRemoteDescription(new RTCSessionDescription(message.description));
  	}
 }
});

socket.on('ready',function(room){
  start();
});

function setLocalStream(mediaStream){
  localStream = mediaStream;
  localVideo.srcObject = mediaStream;
}
function localStreamError(error) {
  console.log('navigator.getUserMedia error: ', error);
}
//##############################################################################
function start() {
  //localConnection = new RTCPeerConnection(configuration);
  localConnection = new RTCPeerConnection(null);
  localConnection.onicecandidate = sendIceCandidate; //ice candidates
  if (status == "Camera") {
    console.log("StartClient");
    localConnection.addStream(localStream);
  } else {
      localConnection.onaddstream = setRemoteStream;
  }
  if (status == "Camera") {
    localConnection.createOffer(sLocalDescription, onError);
  }
}

function sendIceCandidate(event){
  if (event.candidate) {
    var mes = { type: "candidate", candidate: event.candidate };
    remoteIceCandidate = event.candidate;
    console.log(mes);
    console.log("Send local ice candidate");
    socket.emit('message' , mes);
  }
}

function sLocalDescription(description){
  localConnection.setLocalDescription(description);
  console.log("Offer description: \n" + description.sdp);
  localDescription = description;
  var mes = { type: "offer", description: description };
  socket.emit('message', mes);
}
function sRemoteDescription(description){
  localConnection.setLocalDescription(description);
  console.log("Answer description: \n" + description.sdp);
  localDescription = description;
  var mes = { type: "answer", description: description };
  socket.emit('message', mes);
}

function onError(){
  console.log("Error with description");
}
window.onunload = function(){
    localConnection.close();
    localConnection = null;
}


function setRemoteStream(event){
  console.log("Set remote stream");
  remoteStream = event.stream;
  remoteVideo.srcObject = event.stream;
}
