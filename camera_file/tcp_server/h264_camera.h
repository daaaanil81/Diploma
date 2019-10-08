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
#include <openssl/sha.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <libwebsockets.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>
#include <pthread.h>

#define TCP_PORT 554
#define DESCRIBE_BUFFER_SIZE 1024
#define SETUP_BUFFER_SIZE 500
#define OPTION_BUFFER_SIZE 300
#define PLAY_BUFFER_SIZE 300
#define STUN_HEADER 20
#define STUN_HEADER_ATTR 4
#define SIZE_CAMERA 3
#define REQUEST 300
#define DEBUG 1
#define HEADER_TYPE 0x0001
#define MAGICK 0x2112A442
#define USERNAME 0x0006
#define ICE_CONTROLLING 0x802A
#define ICE_CONTROLLING_LENGTH 0x0008
#define ICE_CONTROLLED 0x8029
#define PROPRITY_VALUE 0x6E7F00FF
#define PRIORITY 0x0024
#define PRIORITY_LENGTH 0x0004
#define USE_CANDIDATE 0x0025
#define USE_CANDIDATE_LENGTH 0x0
#define USE_CANDIDATE_VALUE 0x0
#define MESSAGE_INTEGRITY 0x0008
#define MESSAGE_INTEGRITY_LENGTH 0x0014
#define FINGERPRINT 0x8028
#define FINGERPRINT_LENGTH 0x0004
#define STUN_CRC_XOR 0x5354554eUL
/*
struct iovec {
    void  *iov_base;    // Starting address
    size_t iov_len;     // Number of bytes to transfer
};
*/
struct pthread_arguments
{
    struct sockaddr_in sddr; /// Struct for create tcp socket for requests camera
    char ip_camera[20]; /// Ip address camera's
    int camerafd; /// Identificator for request on request
    char sdp_offer[4100]; /// Container for sdp from browser
    char sdp_camera[1024]; /// Container for sdp from camera
    char sdp_answer[1024]; /// Container for sdp from server
    unsigned int port_ice; /// Port in ice candidate on server
    unsigned int port_camera; /// Port for receive stream from camera
    char ice_browser[300]; /// Ice candidate from browser
    char ice_server[300]; /// ICe candidate from server
    char session[20]; /// Number session in setup from camera
    char uflag_server[10]; /// Ice-ufrag in sdp from server
    char uflag_browser[10]; /// Ice-ufrag in sdp from browser
    char ip_server[20]; /// Host computer with server
    char ip_browser[20]; ///Host browser
    char pwd_browser[30]; ///PWD from browser in sdp description
    char pwd_server[30]; ///PWD from server
    pthread_t tchild; /// Identificator thread
};
struct header {
    uint16_t msg_type;
    uint16_t data_len;
    uint32_t magick;
    uint32_t id[3];
};

struct tlv {
    uint16_t type;
    uint16_t len;
};

struct ice_controlling {
    struct tlv t;
    uint64_t value;
};

struct priority {
    struct tlv t;
    uint32_t priority;
};

struct use_candidate {
    struct tlv t;
};
struct username {
    struct tlv t;
};
struct message_integrity {
    struct tlv t;
    char digest[20];		
};
struct fingerprint {
    struct tlv t;
    uint32_t crc;
};
int connect_camera(struct sockaddr_in& saddr, int& camerafd, char* host);
int option_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host);
int describe_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* ans);
int setup_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, unsigned int port, char* session);
int play_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* session);
void create_ice(struct pthread_arguments* p_a);
int sdpParse(struct pthread_arguments* p_a);
void iceParse(struct pthread_arguments* p_a, unsigned int& port);
void pwdParse(struct pthread_arguments* p_a);
void* udp_stream(void* arg);
int generationSTUN(char* ip_server, char*ip_browser, unsigned int ice_port_browser, unsigned int ice_port_server, char* name, char* pwd);
void setUSERNAME(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index, char* name);
void setHeader(struct header* h, struct iovec* iov, char* d_r, unsigned int& index);
void setICE_CONTROLLING(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index);
void setPRIORITY(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index);
void setUSE_CANDIDATE(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index);
void integrity(struct message_integrity* mi, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index, char* pwd);
void Fingerprint(struct fingerprint* f, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index);


struct argumenst_for_camera {
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
static char ice_candidate_second[] = " typ host generation 0 ufrag sEMT network-cost 999";
static char type_sdp[] = "SDP";
static char type_ice[] = "ICE";

static unsigned int port_ice_start = 53532;
static unsigned int port_camera_start = 43700;

static bool busy = false;
static bool other_user = false;
static bool ice_step = false;
static bool sdp_step = false;
#endif
