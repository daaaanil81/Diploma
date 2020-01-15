
#ifndef _H264_
#define _H264_
#include "dtls.h"
#include "base64.h" 
#include "stun.h"


/** Connection on stream
*
*
*
*/
int connect_camera(struct sockaddr_in& saddr, int& camerafd, char* host);
/** Option into camera
*
*
*
*/
int option_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host);
/* Describe into camera
*
*
*
*/
int describe_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* ans);
/* Setup into camera
*
*
*
*/
int setup_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, unsigned int port, char* session, unsigned int& port_udp);
/* Play into camera
*
*
*
*/
int play_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* session);
/* Teardown into camera
*
*
*
*/
int teardown_to_camera(int sockfd, char* host, char* session);
/** Create server's ice candidate for browser
*
*
*
*/
int create_ice(struct pthread_arguments* p_a);
/** Parsing sdp from camera and create sdp for browser
*
*
*
*/
int sdpParse(struct pthread_arguments* p_a);
/** Parsing ice candidate from browser
*
*
*
*/
void iceParse(struct pthread_arguments* p_a);
void pwdParse(struct pthread_arguments* p_a);
int generationSTUN(struct pthread_arguments* p_a);
void free_all(struct pthread_arguments* p_a);
unsigned int rtp_parse(char* rtp, unsigned char* rtp_sps, unsigned int* sequnce, unsigned int* sequnce_origin, struct pthread_arguments* p_a);
unsigned int rtp_sps_parse(char* rtp, unsigned char* sps, unsigned int sequnce, struct pthread_arguments* p_a);


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
static char ice_candidate_second[] = " typ host generation 0 ufrag sEMT network-cost 999";
static char type_sdp[] = "SDP";
static char type_ice[] = "ICE";
//static char ip_server_program[16];
static unsigned int port_ice_start = 53532;
static unsigned int port_camera_start = 43700;
static bool list[5] = {0};
static pthread_t tchilds[5]; /// Identificator threads
static bool busy = false;
static bool other_user = false;
static bool ice_step = false;
static bool sdp_step = false;
static bool flag_sps_send = false;
#endif
