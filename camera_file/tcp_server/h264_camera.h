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
#define SETUP_BUFFER_SIZE 500
#define OPTION_BUFFER_SIZE 300
#define PLAY_BUFFER_SIZE 300

#define SIZE_CAMERA 3
#define REQUEST 300
#define DEBUG 1

int connect_camera(struct sockaddr_in& saddr, int& camerafd, char* host);
int option_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host);
int describe_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* ans);
int setup_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, unsigned int port, char* session);
int play_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* session);
void create_ice(char* ice_server, unsigned int port_ice);
static char type_sdp[] = "SDP";
static char type_ice[] = "ICE";
struct argumenst_for_camera
{
    unsigned int port_ice;
    unsigned int port_camera;
    char ip[20];
};
static struct argumenst_for_camera afc[SIZE_CAMERA];
static char option_camera_first[] = "OPTIONS rtsp://";
static char option_camera_second[] = "/axis-media/media.amp RTSP/1.0\r\nCSeq: " "1\r\nUser-Agent: WebRTC_Dan\r\n\r\n";
static char describe_camera_first[] = "DESCRIBE rtsp://";
static char describe_camera_second[] = "/axis-media/media.amp "
"RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: WebRTC_Dan\r\nAccept: application/sdp\r\n\r\n";
static char setup_camera_first[] = "SETUP rtsp://";
static char setup_camera_second[] = "/axis-media/media.amp/trackID=1 "
"RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: WebRTC_Dan\r\nTransport: RTP/AVP;unicast;client_port=";
static char play_camera_first[] = "PLAY rtsp://";
static char play_camera_second[] = "/axis-media/media.amp "
    "RTSP/1.0\r\nCSeq: 4\r\nUser-Agent: "
"WebRTC_Dan\r\nSession: ";
static char play_camera_thirt[] = "\r\nRange: npt=0.000-\r\n\r\n";
static char ice_candidate_first[] = "candidate:1968211759 1 udp 2122252543 ";
static char ice_candidate_second[] = " typ host generation 0 ufrag XJ0D network-id 1 network-cost 10";
//10.168.75.95 50532 typ host"
static unsigned int port_ice_start = 53532;
static unsigned int port_camera_start = 43700;

int sdpParse(char* des, char* flag,char* offer);
#endif
