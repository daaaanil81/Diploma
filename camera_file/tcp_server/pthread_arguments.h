#ifndef _PTHREAD_
#define _PTHREAD_
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
#include <sys/stat.h>
#include <endian.h>
#include <netdb.h>
#include <stdint.h>
#include <time.h>

#define TCP_PORT 554 /// TCP port for connect to camera
#define DESCRIBE_BUFFER_SIZE 1024 /// Size buffer for description camera
#define SETUP_BUFFER_SIZE 500 /// Size buffer for setup camera
#define OPTION_BUFFER_SIZE 300 /// Size buffer for option camera
#define PLAY_BUFFER_SIZE 300 /// Size buffer for play camera
#define STUN_HEADER 20 /// Size stun header
#define STUN_HEADER_ATTR 4 /// Size stun header
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
#define XOR_MAPPED_ADDRESS 0x0020
#define XOR_MAPPED_ADDRESS_LENGTH 0x0008
#define STUN_CRC_XOR 0x5354554eUL
#define SOFTWARE 0x8022
#define DTLS_FINGERPRINT_MAX_SIZE 25
#define DTLS_MESSAGES 1024
#define BUFSIZE 4600
#define NAMEDPIPE_NAME "/tmp/PipeOne"
#define FLAG_TESTING 0 
#define SRTP_MAX_MASTER_KEY_LEN 32
#define SRTP_MAX_MASTER_SALT_LEN 14
#define SRTP_MAX_SESSION_KEY_LEN 32
#define SRTP_MAX_SESSION_SALT_LEN 14
#define SRTP_MAX_SESSION_AUTH_LEN 20
struct str {
    size_t len;
    char s[30];
};
struct str_key {
    unsigned char* str;
    unsigned int len;
};
struct dtls_connection {
	SSL_CTX *ssl_ctx;
	SSL *ssl;
	BIO *r_bio, *w_bio;
};
struct dtls_fingerprint
{
    unsigned char digest_fingerprint[DTLS_FINGERPRINT_MAX_SIZE];
    unsigned int size;
};


struct crypto_suite;

struct crypto_params {
	const struct crypto_suite *crypto_suite;
	unsigned char master_key[SRTP_MAX_MASTER_KEY_LEN];
	unsigned char master_salt[SRTP_MAX_MASTER_SALT_LEN];
	unsigned char *mki;
	unsigned int mki_len;
};

struct crypto_context {
	struct crypto_params params;
	struct crypto_params server_params;
	char session_key[SRTP_MAX_SESSION_KEY_LEN]; /* k_e */
	char session_salt[SRTP_MAX_SESSION_SALT_LEN]; /* k_s */
	char session_auth_key[SRTP_MAX_SESSION_AUTH_LEN];
	void *session_key_ctx[2];
	int have_session_key;
};

typedef int (*crypto_func_rtp)(struct crypto_context *c, struct str_key *payload, uint32_t ssrc, uint64_t index);
typedef int (*crypto_func_rtcp)(struct crypto_context *c, struct str_key *payload, uint32_t ssrc, uint64_t index);

typedef int (*hash_func_rtp)(struct crypto_context *, unsigned char *payload, struct str_key* in, uint64_t index);
typedef int (*hash_func_rtcp)(struct crypto_context *, unsigned char *payload, struct str_key* in);

typedef int (*session_key_init_func)(struct crypto_context *);
typedef int (*session_key_cleanup_func)(struct crypto_context *);



struct crypto_suite {
	const char *name;
	const char *dtls_name;
	unsigned int
		master_key_len,
		master_salt_len,
		session_key_len,	/* n_e */
		session_salt_len,	/* n_s */
		srtp_auth_tag,		/* n_a */
		srtcp_auth_tag,
		srtp_auth_key_len,	/* n_a */
		srtcp_auth_key_len;
	unsigned long long int
		srtp_lifetime,
		srtcp_lifetime;
	int kernel_cipher;
	int kernel_hmac;
	crypto_func_rtp encrypt_rtp,
			decrypt_rtp;
	crypto_func_rtcp encrypt_rtcp,
			 decrypt_rtcp;
	hash_func_rtp hash_rtp;
	hash_func_rtcp hash_rtcp;
	session_key_init_func session_key_init;
	session_key_cleanup_func session_key_cleanup;
	//const char *dtls_profile_code; // unused
	const void *lib_cipher_ptr;
	unsigned int idx; // filled in during crypto_init_main()
};

struct pthread_arguments
{
    struct sockaddr_in sddr; /// Struct for create tcp socket for requests camera
    struct sockaddr_in stun_from_server; /// Struct for create udp socket for request to google stun server
    char ip_camera[20]; /// Ip address camera's
    int camerafd; /// Identificator for request on request
	int socket_rtp_fd; /// Socket for  camera rtp stream
	int socket_rtcp_fd; /// Socket for  camera rtcp stream
    char sdp_offer[4100]; /// Container for sdp from browser
    char sdp_camera[1024]; /// Container for sdp from camera
    char sdp_answer[4400]; /// Container for sdp from server
    unsigned int port_ice; /// Port for browser in sdp camera
    unsigned int port_camera; /// Port for receive stream from camera
    unsigned int port_rtcp_camera; /// Port rtcp for camera 
    int socket_stream; 
    unsigned int port_ice_browser; /// Port received from browser in candidate
    char ice_browser[300]; /// Ice candidate from browser
    char ice_server[300]; /// ICe candidate from server
    char session[20]; /// Number session in setup from camera
    char uflag_server[10]; /// Ice-ufrag in sdp from server
    char uflag_browser[10]; /// Ice-ufrag in sdp from browser
    char ip_server[20]; /// Host computer with server
    char ip_browser[20]; ///Host browser
    struct str pwd_browser; ///PWD from browser in sdp description
    struct str pwd_server; ///PWD from server
    unsigned char sps[30];
    unsigned short size_sps;
    unsigned char pps[10];
    unsigned short size_pps;
    struct dtls_fingerprint attr_fingerprint;
    struct dtls_connection dtls_cert;
    struct crypto_context crypto;
	struct crypto_context crypto_from_camera;
	struct crypto_context crypto_rtcp;
	uint32_t index_rtcp;
    X509* x509;
    EVP_PKEY* pkey;
    BIGNUM *exponent;
    BIGNUM* serial_number;
	RSA *rsa;
	ASN1_INTEGER *asn1_serial_number;
	X509_NAME *name;
    uint64_t index;
	uint16_t sequnce_origin;
	uint16_t sequnce_new;
	uint32_t qSec;
};
void gen_random(unsigned char *s, const int len);
static char ip_server_program[16];
void printText(unsigned char* text, unsigned int len);
static unsigned int master_key[] = {0x99,0xe1,0x3f,0x5f,0x45,0x95,0x1c,0x6f,0x5a,0x87,0xd3,0x05,0xea,0x84,0x2b,0xa5};
static unsigned int master_salt[] = {0x7d,0x9e,0x1e,0xdf,0xaa,0xda,0xba,0xa1,0x0f,0xfb,0x08,0xad,0xf2,0x7f};

#endif
