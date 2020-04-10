SERVER_PATH=./js
TCP_SERVER_PATH=./tcp_server
UDP_FILE_PATH=./stream_to_file
BASE64=./Base64
UDP_FILE_PROJECT=$(UDP_FILE_PATH)/stream_to_file
TCP_PROJECT=$(TCP_SERVER_PATH)/ws-server
start_http_server:
	node $(SERVER_PATH)/http_server.js $(IP)
compile:
	gcc -O0 -g $(TCP_SERVER_PATH)/dtls.c $(TCP_SERVER_PATH)/rtp.c $(TCP_SERVER_PATH)/stun.c $(TCP_SERVER_PATH)/rtcp.c $(TCP_SERVER_PATH)/crypto.c $(TCP_SERVER_PATH)/h264_camera.c $(TCP_SERVER_PATH)/pthread_arguments.c $(TCP_SERVER_PATH)/ws-server.c $(TCP_SERVER_PATH)/base64.c -o $(TCP_PROJECT) -lssl -lm -lz -lcrypto  -pthread -lwebsockets 
clean:
	rm -rf $(TCP_SERVER_PATH)/*.dSYM
	rm -rf $(TCP_SERVER_PATH)/*.swp
	rm -rf $(TCP_SERVER_PATH)/*.swo
	rm -rf $(UDP_FILE_PATH)/*.swp
	rm -rf $(UDP_FILE_PATH)/*.swo
	rm $(TCP_PROJECT)
	rm $(UDP_FILE_PROJECT)
open:
	open $(TCP_SERVER_PATH)/h264_camera.cpp
	open $(TCP_SERVER_PATH)/ws-server.c
	open $(TCP_SERVER_PATH)/h264_camera.h
udp:
	gcc $(UDP_FILE_PATH)/stream_to_file.c -o $(UDP_FILE_PROJECT)
