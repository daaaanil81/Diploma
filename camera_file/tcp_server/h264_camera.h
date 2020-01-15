
#ifndef _H264_
#define _H264_
#include "dtls.h"
#include "base64.h" 
#include "stun.h"


<<<<<<< HEAD
=======
struct dtls_fingerprint
{
    unsigned char digest_fingerprint[DTLS_FINGERPRINT_MAX_SIZE];
    unsigned int size;
};
struct pthread_arguments
{
    struct sockaddr_in sddr; /// Struct for create tcp socket for requests camera
    char ip_camera[20]; /// Ip address camera's
    int camerafd; /// Identificator for request on request
    char sdp_offer[4100]; /// Container for sdp from browser
    char sdp_camera[1024]; /// Container for sdp from camera
    char sdp_answer[4400]; /// Container for sdp from server
    unsigned int port_ice; /// Port for browser in sdp camera
    unsigned int port_camera; /// Port for receive stream from camera
    unsigned int port_udp_camera; 
    int socket_stream;
    unsigned int port_ice_browser; /// Port received from browser in candidate
    char ice_browser[300]; /// Ice candidate from browser
    char ice_server[300]; /// ICe candidate from server
    char session[20]; /// Number session in setup from camera
    char uflag_server[10]; /// Ice-ufrag in sdp from server
    char uflag_browser[10]; /// Ice-ufrag in sdp from browser
    char ip_server[20]; /// Host computer with server
    char ip_browser[20]; ///Host browser
    char pwd_browser[30]; ///PWD from browser in sdp description
    char pwd_server[30]; ///PWD from server
    unsigned char sps[30];
    unsigned short size_sps;
    unsigned char pps[10];
    unsigned short size_pps;
    struct dtls_fingerprint attr_fingerprint;
    pthread_t tchild; /// Identificator thread
    X509* x509;
    EVP_PKEY* pkey;
    BIGNUM *exponent;
    BIGNUM* serial_number;
	RSA *rsa;
	ASN1_INTEGER *asn1_serial_number;
	X509_NAME *name;
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
>>>>>>> b21222ae0d64a15732cb38342a33f405512c52b0
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
<<<<<<< HEAD
void free_all(struct pthread_arguments* p_a);
unsigned int rtp_parse(char* rtp, unsigned char* rtp_sps, unsigned int* sequnce, unsigned int* sequnce_origin, struct pthread_arguments* p_a);
unsigned int rtp_sps_parse(char* rtp, unsigned char* sps, unsigned int sequnce, struct pthread_arguments* p_a);
=======
void setUSERNAME(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index, char* name);
void setHeader(struct header* h, struct iovec* iov, char* d_r, unsigned int& index);
void setICE_CONTROLLING(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index);
void setPRIORITY(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index);
void setUSE_CANDIDATE(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index);
void integrity(struct message_integrity* mi, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index, char* pwd);
void Fingerprint(struct fingerprint* f, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index);
void free_all(struct pthread_arguments* p_a);
unsigned int rtp_parse(char* rtp, unsigned char* rtp_sps, unsigned int* sequnce, unsigned int* sequnce_origin, struct pthread_arguments* p_a);
unsigned int rtp_sps_parse(char* rtp, unsigned char* sps, unsigned int sequnce, struct pthread_arguments* p_a);
void gen_random(char *s, const int len);
struct argumenst_for_camera {
    unsigned int port_ice;
    unsigned int port_camera;
    char ip[20];
};
>>>>>>> b21222ae0d64a15732cb38342a33f405512c52b0


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
