
const remoteVideo = document.getElementById('remoteVideo');
const configuration = {iceServers: [{urls: 'stun:stun.l.google.com:19302'}]};

var localConnection;
var localDescription;
var remoteDescription;
var localIce;
var remoteIce;
var remoteStream;
var sizeIce = 0;
var flagSDP = true;
var candidate_result = null;
var options = {
    offerToReceiveAudio: false,
    offerToReceiveVideo: true
};
var flag_ICE = true;
var flag_Connection = false;
//#####################################################################################################################
const host = window.location.href.split("?")[1].split("=")[1];
document.getElementById("IdText").innerText = host;
console.log("Hello");
//#####################################################################################################################
var connection = new WebSocket('wss://10.168.0.235:9999', 'lws-minimal'); // tcp server on c/c++
console.log("Connection...");
// console.log("Socket connection timeout", connection.readyState);
// if(!connection.readyState)
// {
//     console.log("Error with connection");
//     connection = new WebSocket('wss://petrov.in.ua:9999', 'lws-minimal'); // tcp server on c/c++
// } 
// if(!flag_Connection)
// {
//     console.log("Error with connection");
//     connection = new WebSocket('wss://petrov.in.ua:9999', 'lws-minimal'); // tcp server on c/c++
//     flag_ICE = false;
//     flag_Connection = true;
// }
// if(!flag_Connection)
// {
//     console.log("Error with connection");
//     document.getElementById("status").innerText = "Error with connection";
// }

connection.onopen = function () {
    console.log("Send");
    flag_Connection = true;
    clearTimeout(timerId);
};

connection.onclose = function (event) {
    if(localConnection != null)
    { 
        localConnection.close();
    }
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
        localConnection = new RTCPeerConnection(configuration);
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
        setTimeout(sendConnect, 3000);
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
        document.getElementById("status").innerText = event.data;
    }
};
function sendIceCandidate(event) {
    if (event.candidate) {
        console.log("Candidate: " + event.candidate.candidate);
        var array = event.candidate.candidate.split(' ');
        var ip_address = array[4];
        if(sizeIce <= 0)
        {
            if(ip_address.substr(0, 6) == '10.168' && flag_ICE)
            {
                candidate_result = event.candidate.candidate;
                connection.send('ICE' + event.candidate.candidate);
                sizeIce++;  
            }
            if(ip_address.substr(0, 6) != '10.168' && !flag_ICE)
            {
                candidate_result = event.candidate.candidate;
                connection.send('ICE' + event.candidate.candidate);
                sizeIce++;
            }
        }
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