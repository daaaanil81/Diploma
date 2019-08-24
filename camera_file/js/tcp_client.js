const remoteVideo = document.getElementById("remoteVideo");
const localConnection = new RTCPeerConnection(null);
var sizeIce = 0;
var localICe;
//#####################################################################################################################
const host = window.location.href.split("?")[1].split("=")[1];
document.getElementById("IdText").innerText = host;
//#####################################################################################################################
const connection = new WebSocket('ws://10.168.75.95:8555'); // tcp server on c/c++
connection.onopen = function () {
    console.log("Send");
    connection.send(host); // Send the message 'Ping' to the server
};
connection.onclose = function (event) {
    console.log("Close");
};
connection.onmessage = function (event) {
    var mes;
    if (event.data === 'OK') {
        console.log(event.data);
        //localConnection.onicecandidate = sendIceCandidate; //ice candidates
    } else {
        console.log(event.data);
    }
    // var Type = event.data.substr(0, 3);
    // if(Type === '111')
    // {
    //     console.log("SDP");
    //     mes = event.data.substr(3, event.data.length - 3);
    //     console.log(mes);
    // }
};

function sendIceCandidate(event) {
    console.log("send ICE");
    if (event.candidate && sizeIce <= 0) {
        localICe = event.candidate;
        console.log("Send local ice candidate");
        console.log(localIce);
        connection.send(localICe);
        sizeIce++;
    }
}
