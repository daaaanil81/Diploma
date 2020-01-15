#ifndef _STUN_
#define _STUN_
#include "pthread_arguments.h"
#include <zlib.h>
#include <stdint.h>


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
    uint32_t value[2];
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
    unsigned char digest[20];		
};
struct fingerprint {
    struct tlv t;
    uint32_t crc;
};
struct software {
    struct tlv t;
    char str[128];
};

struct xor_mapped_address {
	struct tlv t;
	u_int16_t family;
	u_int16_t port;
	u_int32_t address[4];
};
int stun_request(struct pthread_arguments* p_a);
int stun_response(struct pthread_arguments *p_a);

unsigned int stun_header(struct msghdr *mh, struct iovec* iov, struct header* hdr, unsigned char* buf, unsigned int* index, unsigned char* transaction);
unsigned int stun_software(struct msghdr *mh, struct software* sw, unsigned char* buf, unsigned int* index);
unsigned int stun_username(struct msghdr *mh, struct username* user, unsigned char* buf, unsigned int* index, char* buf_name);
unsigned int stun_controlled(struct msghdr *mh, struct ice_controlling *contr, unsigned char *buf, unsigned int *index);
unsigned int stun_priority(struct msghdr *mh, struct priority *prior, unsigned char *buf, unsigned int *index);
unsigned int stun_integrity(struct msghdr *mh, struct message_integrity *mi, str *browser_pwd, unsigned char *buf, unsigned int *index);
int stun_xor_mapped(struct msghdr *mh, struct xor_mapped_address *x_m_a, unsigned char *buf, unsigned int *index);




#endif


