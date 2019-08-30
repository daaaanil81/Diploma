const remoteVideo = document.getElementById("remoteVideo");
const localConnection = new RTCPeerConnection(null);
var localDescription;
var remoteDescription;
var localIce;
var remoteIce;
var remoteStream;
var sizeIce = 0;
var flagSDP = true;

//#####################################################################################################################
const host = window.location.href.split("?")[1].split("=")[1];
document.getElementById("IdText").innerText = host;
//#####################################################################################################################
const connection = new WebSocket('wss://10.168.75.95:8666', 'lws-minimal'); // tcp server on c/c++
connection.onopen = function () {
    console.log("Send");
    //
};
connection.onclose = function (event) {
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
        localConnection.onicecandidate = sendIceCandidate; //ice candidate
        localConnection.onaddstream = setRemoteStream;

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
        var description = {type: "offer", sdp: mes};
        localConnection.setRemoteDescription(new RTCSessionDescription(description)).catch(onError_Valid_Description);
        remoteDescription = description;
    }
    if(Type === 'ICE')
    {
        mes = event.data.substr(3, event.data.length - 3);
        var candidate = new RTCIceCandidate({sdpMLineIndex: 0, candidate: mes});
        remoteIce = candidate;
        console.log("Receive remote ice candidate");
        localConnection.addIceCandidate(candidate, sucIce, errorIce);
    }
    if(Type === 'Err')
    {
        console.log(event.data);
    }
};
function answer()
{
    localConnection.createAnswer(sRemoteDescription, onError);
}

function sucIce()
{
    console.log("Successful in candidate other user");
}
function errorIce()
{
    console.log("Error in candidate other user");
}
function sendConnect()
{
    console.log("Send");
    connection.send("Connect");
}
function sendIceCandidate(event) {
    console.log("send ICE");
    if (event.candidate && sizeIce <= 0) {
        localIce = event.candidate;
        console.log("Send local ice candidate");
        connection.send('ICE' + localIce.candidate);
        sizeIce++;
    }
    if (flagSDP) {
        console.log("Send answer");
        answer();
        //setTimeout(answer,10000);
    }
}

function sRemoteDescription(description) {
    localConnection.setLocalDescription(description);
    console.log("Answer description: \n" + description.sdp);
    localDescription = description;
    connection.send('SDP' + description.sdp);
}

function onError() {
    console.log("Error with description");
}

function onError_Valid_Description() {
    console.log("onError_Valid_Description");
    flagSDP = false;
    connection.send("Error");
}
function setRemoteStream(event){
    console.log("Set remote stream");
    remoteStream = event.stream;
    remoteVideo.srcObject = event.stream;
}