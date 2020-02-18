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
int stun_request(struct pthread_arguments* );
int stun_response(struct pthread_arguments *, unsigned char* , unsigned int ,struct sockaddr_in* );



unsigned int stun_header(struct msghdr *, struct iovec* , struct header* , unsigned char* , unsigned int* , unsigned char* );
unsigned int stun_software(struct msghdr *, struct software* , unsigned char* , unsigned int* );
unsigned int stun_username(struct msghdr *, struct username *, unsigned char *, unsigned int *, char *, size_t );
unsigned int stun_controlled(struct msghdr *, struct ice_controlling *, unsigned char *, unsigned int *);
unsigned int stun_priority(struct msghdr *, struct priority *, unsigned char *, unsigned int *);
unsigned int stun_integrity(struct msghdr *, struct message_integrity *, struct str *, unsigned char *, unsigned int *);
int stun_xor_mapped(struct msghdr *, struct xor_mapped_address *, unsigned char *, unsigned int *);




#endif


