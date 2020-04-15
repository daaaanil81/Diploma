
#ifndef _H264_
#define _H264_
#include "dtls.h"
#include "base64.h" 
#include "stun.h"
#include "rtp.h"
#include "rtcp.h"


/** Connection on stream
*
*
*
*/
int connect_camera(struct pthread_arguments*);
/** Option into camera
*
*
*
*/
int option_to_camera(struct pthread_arguments*);
/* Describe into camera
*
*
*
*/
int describe_to_camera(struct pthread_arguments*);
/* Setup into camera
*
*
*
*/
int setup_to_camera(struct pthread_arguments*);
/* Play into camera
*
*
*
*/
int play_to_camera(struct pthread_arguments* );
/* Teardown into camera
*
*
*
*/
int teardown_to_camera(struct pthread_arguments* );
/** Create server's ice candidate for browser
*
*
*
*/
int create_ice(struct pthread_arguments* );
/** Parsing sdp from camera and create sdp for browser
*
*
*
*/
int sdpParse(struct pthread_arguments* );
/** Parsing ice candidate from browser
*
*
*
*/
void iceParse(struct pthread_arguments*);
int pwdParse(struct pthread_arguments*);
void free_all(struct pthread_arguments*);
void setSockaddr(struct sockaddr_in*, unsigned char* , unsigned int );
int createSockaddr(struct sockaddr_in*, unsigned char*, unsigned int , int* );
int parameters_to_camera(struct pthread_arguments* p_a);
void sendSocketMessage(struct lws *, char*, int);
void init_ports(unsigned int* , unsigned int* , int);
void MD5_encoder(unsigned char*, unsigned char*,unsigned int);
void *udp_stream(void *arg);
int send_Stun_Sdp_Ice(struct pthread_arguments* arg_pthread, int index_list, struct lws* wsi);

static char option_camera_first[] = "OPTIONS rtsp://";
static char option_camera_second[] = "/axis-media/media.amp RTSP/1.0\r\nCSeq: ";
static char option_camera_three[] = "\r\nUser-Agent: WebRTC_Dan\r\n\r\n";// 1

static char describe_camera_first[] = "DESCRIBE rtsp://";
static char describe_camera_second[] = "/axis-media/media.amp RTSP/1.0\r\nCSeq: ";
static char describe_camera_three[] = "\r\nUser-Agent: WebRTC_Dan\r\nAccept: application/sdp\r\n\r\n"; //2 

static char setup_camera_first[] = "SETUP rtsp://";
static char setup_camera_second[] = "/axis-media/media.amp/trackID=1 RTSP/1.0\r\nCSeq: ";
static char setup_camera_three[] = "\r\nUser-Agent: WebRTC_Dan\r\nTransport: RTP/AVP;unicast;client_port="; //3

static char play_camera_first[] = "PLAY rtsp://";
static char play_camera_second[] = "/axis-media/media.amp RTSP/1.0\r\nCSeq: ";
static char play_camera_three[] = "\r\nUser-Agent: WebRTC_Dan\r\nSession: "; // 4
static char play_camera_thirt[] = "\r\nRange: npt=0.000-\r\n\r\n";

static char parameters_camera_first[] = "GET_PARAMETER rtsp://";
static char parameters_camera_second[] = "/axis-media/media.amp RTSP/1.0\r\nCSeq: ";
static char parameters_camera_three[] = "\r\nUser-Agent: WebRTC_Dan\r\nSession: ";

static char teardown_camera_first[] = "TEARDOWN rtsp://";
static char teardown_camera_second[] = "/axis-media/media.amp RTSP/1.0\r\nCSeq: ";
static char teardown_camera_three[] = "\r\nUser-Agent: RTSPClient\r\nSession: ";

static char ice_candidate_first[] = "candidate:1968211759 1 udp 2122252543 ";
static char ice_candidate_second[] = " typ host generation 0 ufrag sEMT network-cost 999";
static char type_sdp[] = "SDP";
static char type_ice[] = "ICE";

static char command_describe[] = "DESCRIBE";
static char command_setup[] = "SETUP";
static char command_play[] = "PLAY";
static char command_get_parameter[] = "GET_PARAMETER";
static char command_teardown[] = "TEARDOWN";

static char error_unauthorized[] = "401"; 
static unsigned int port_ice_start = 53532; // Port for ice candidates server 53532 - 53536
static unsigned int port_camera_start = 43700; //Port for received rtp and rtcp from camera 43700 - 43704
static bool list[MAX_CLIENT] = {0};
static pthread_t tchilds[MAX_CLIENT] = {0}; /// Identificator threads
static bool other_user = false;
static bool ice_step = false;
static bool sdp_step = false;
static bool flag_sps_send = false;
static struct pthread_arguments* pthreads[MAX_CLIENT] = {0};
#endif
