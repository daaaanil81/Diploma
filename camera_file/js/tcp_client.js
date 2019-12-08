const remoteVideo = document.getElementById('remoteVideo');
//const configuration = {iceServers: [{urls: 'stun:stun.l.google.com:19302'}]};
var localConnection;
var localDescription;
var remoteDescription;
var localIce;
var remoteIce;
var remoteStream;
var sizeIce = 0;
var flagSDP = true;
var inboundStream = null;
var options = {
    offerToReceiveAudio: false,
    offerToReceiveVideo: true
};
//#####################################################################################################################
const host = window.location.href.split("?")[1].split("=")[1];
document.getElementById("IdText").innerText = host;
//#####################################################################################################################
var connection = new WebSocket('wss://10.168.166.132:8666', 'lws-minimal'); // tcp server on c/c++
connection.onopen = function () {
    console.log("Send");
    //
};
connection.onclose = function (event) {
    localConnection.close();
    localConnection = null;
    console.log("Close");
};
connection.onmessage = function (event) {
    var mes;
    if (event.data === 'Connect') {
        console.log(event.data);
        console.log(host);
        connection.send('host' + host); // Send the message 'Ping' to the server
    }
    if (event.data === 'OK') {
        console.log(event.data);
        localConnection = new RTCPeerConnection(null);
        /*localConnection.onaddstream = function(event) {
            remoteVideo.srcObject = event.stream;
            remoteStream = event.stream;
            console.log("Receive stream");
        };*/
        

        localConnection.onicecandidate = sendIceCandidate; //ice candidate
        localConnection.createOffer(sLocalDescription, onError, options);
        localConnection.addEventListener('track', gotRemoteStream);
    }

    if (event.data === 'Busy') {
        console.log("Busy");
        //setTimeout(sendConnect, 3000);
    }
    var Type = event.data.substr(0, 3);
    if (Type === 'SDP') {
        console.log("SDP");
        mes = event.data.substr(3, event.data.length - 3);
        console.log(mes);
        var description = { type: "answer", sdp: mes };
        localConnection.setRemoteDescription(new RTCSessionDescription(description)).catch(onError_Valid_Description);
        remoteDescription = description;
    }
    if (Type === 'ICE') {
        mes = event.data.substr(3, event.data.length - 3);
        var candidate = new RTCIceCandidate({ sdpMLineIndex: 0, candidate: mes });
        remoteIce = candidate;
        console.log("Receive remote ice candidate");
        localConnection.addIceCandidate(candidate, sucIce, errorIce);
    }
    if (Type === 'Err') {
        console.log(event.data);
    }
};
function sendIceCandidate(event) {
    if (event.candidate) {
        console.log("Candidate: " + event.candidate.candidate);
    }
    if (event.candidate && sizeIce <= 0) {
        localIce = event.candidate;
        console.log("Send local ice candidate");
        connection.send('ICE' + localIce.candidate);
        sizeIce++;
    }
}

function sLocalDescription(description) {
    localConnection.setLocalDescription(description);
    console.log("Offer description: \n" + description.sdp);
    localDescription = description;
    connection.send('SDP' + description.sdp);
}

function gotRemoteStream(e) {
    if (remoteVideo.srcObject !== e.streams[0]) {
      remoteVideo.srcObject = e.streams[0];
      console.log('pc2 received remote stream');
    }
    remoteVideo.play();
    //remoteVideo.autoplay = true;
  }

function onError() {
    console.log("Error with description");
}

function onError_Valid_Description() {
    console.log("onError_Valid_Description");
    flagSDP = false;
    connection.send("Error");
}

function sucIce() {
    console.log("Successful in candidate other user");
}
function errorIce() {
    console.log("Error in candidate other user");
}
function sendConnect() {
    console.log("Send");
    connection.send("Connect");
}
window.onunload = function(){
    localConnection.close();
    localConnection = null;
    console.log("Close pages");
};