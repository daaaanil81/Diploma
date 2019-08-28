#ifndef H264
#define H264
#include <stdio.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/poll.h>
#include "Base64.h"
#include <openssl/sha.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <libwebsockets.h>
#include <stdbool.h>

#define TCP_PORT 554
#define DESCRIBE_BUFFER_SIZE 1024
#define OPTION_BUFFER_SIZE 300
#define REQUEST 300
#define DEBUG 1

int connect_camera(struct sockaddr_in& saddr, int& camerafd, char* host);
int option_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host);
int describe_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* ans);
static char type_sdp[] = "SDP";
static char option_camera_first[] = "OPTIONS rtsp://";
static char option_camera_second[] = "/axis-media/media.amp RTSP/1.0\r\nCSeq: " "1\r\nUser-Agent: WebRTC\r\n\r\n";
static char describe_camera_first[] = "DESCRIBE rtsp://";
static char describe_camera_second[] = "/axis-media/media.amp "
"RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: WebRTC\r\nAccept: application/sdp\r\n\r\n";
int sdpParse(char* des, char* offer);
#endif
