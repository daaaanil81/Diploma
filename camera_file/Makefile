SERVER_PATH=./js
TCP_SERVER_PATH=./tcp_server
TCP_PROJECT=$(TCP_SERVER_PATH)/ws-server
start_http_server:
	node $(SERVER_PATH)/http_server.js $(IP)
compile:
	g++ -O0 -g  $(TCP_SERVER_PATH)/h264_camera.cpp $(TCP_SERVER_PATH)/ws-server.c $(TCP_SERVER_PATH)/Crc32.c  $(TCP_SERVER_PATH)/Base64.cpp  -o $(TCP_PROJECT) -lssl -lcrypto  -pthread -lwebsockets
clean:
	rm -rf $(TCP_SERVER_PATH)/*.dSYM
	rm $(TCP_PROJECT)
