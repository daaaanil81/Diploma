var size_camera = 0;
var row = 0;

function clickedAdd() {
    // let ip = prompt('Enter plese Ip camera', '');
    var input_ip = document.getElementById('ip_addres');
    var re = /((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)(\.|$)){4}/;
    var OK = re.exec(input_ip.value);  
    if (input_ip.value.length >= 16 || !OK)
    {
        document.getElementById('url').innerHTML = "Bad IP";
    }
    else
    {
        var td = document.getElementById('td_add_camera');
        let div_test = document.createElement('div');
        div_test.className = "div_form_camera";
        size_camera += 1;
        div_test.id = input_ip.value.substr(7);
        div_test.onclick = function () {
            document.location="/stream.html?ip="+'10.168.' + this.id;
        };
        if(size_camera == 7)
        {
            size_camera = 1;
            row += 1;
        }
        div_test.style.top = (row*190+95).toString() + "%";
        div_test.style.left = (size_camera*10 + 30) + "%";
        div_test.innerHTML = "<p align='center'><img src=images/video-camera.png></img></p><p align='center'>" + input_ip.value + "</p>";
        td.append(div_test);
    }
}
function clickedAdd_mobile() {
}
function createUrl() {
    var input1 = document.getElementById('ip_addres');
    var input2 = document.getElementById('ip_addres_mobile');
    if (input1.value.length >= 16 || input2.value.length >= 16)
    {
        document.getElementById('url').innerHTML = "Bad IP";
        document.getElementById('url_mobile').innerHTML = "Bad IP";
    }
    else
    {        
        document.getElementById('url').innerHTML = "rtsp://" + input1.value + "/axis-media/media.amp";
        document.getElementById('url_mobile').innerHTML = "rtsp://" + input2.value + "/axis-media/media.amp";
    }
}
