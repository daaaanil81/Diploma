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
#include <fcntl.h>

#define TCP_PORT 554
#define DESCRIBE_BUFFER_SIZE 1024
#define SETUP_BUFFER_SIZE 500
#define OPTION_BUFFER_SIZE 300
#define PLAY_BUFFER_SIZE 300
#define STUN_HEADER 20
#define STUN_HEADER_ATTR 7
#define SIZE_CAMERA 3
#define REQUEST 300
#define DEBUG 1
#define USERNAME 0x0006
#define ICE_CONTROLLING 0x802A
#define ICE_CONTROLLING_LENGTH 0x0008
#define ICE_CONTROLLED 0x8029
#define USE_CANDIDATE 0x0025
#define PROPRITY_VALUE 0x6E7F00FF
#define PRIORITY 0x0024
#define PRIORITY_LENGTH 0x0004
int connect_camera(struct sockaddr_in& saddr, int& camerafd, char* host);
int option_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host);
int describe_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* ans);
int setup_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, unsigned int port, char* session);
int play_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* session);
void create_ice(char* ice_server, unsigned int port_ice,char* ip_user);
int sdpParse(char* des, char* flag, char* answer,char*ice);
void iceParse(char* ice, char* ip, char* ip_user, unsigned int& port, char* uflag_browser);
void* udp_stream(void* arg);
int generationSTUN(char* ip_server, char*ip_browser, unsigned int ice_port_browser, unsigned int ice_port_server, char* name);
struct argumenst_for_camera
{
    unsigned int port_ice;
    unsigned int port_camera;
    char ip[20];
};
struct stun_message {
    uint16_t msg_type;
    uint16_t data_len;
    uint32_t magick;
    uint32_t id[3];
    unsigned char data[256];
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
static char ice_candidate_second[] = " typ host generation 0 ufrag SHgX network-cost 999";

static char type_sdp[] = "SDP";
static char type_ice[] = "ICE";

static unsigned int port_ice_start = 53532;
static unsigned int port_camera_start = 43700;

static bool busy = false;
static bool other_user = false;
static bool ice_step = false;
static bool sdp_step = false;
#endif
