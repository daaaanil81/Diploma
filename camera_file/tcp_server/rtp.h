#ifndef _RTP_
#define _RTP_
#include <string.h>
#include <glib.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/err.h>
#include <time.h>
#include "pthread_arguments.h"
struct rtp_header {
    char all_mark[2];
    uint16_t seq_num;
    uint32_t ssrc;
    uint32_t timestamp;
};

void rtp_init(struct pthread_arguments *p_a);
int crypto_init_session_key(struct crypto_context* c);
int crypto_encrypt_rtp(struct crypto_context *c, unsigned char* rtp);
int crypto_encrypt_rtcp(struct crypto_context *c, unsigned char* rtcp);
unsigned int rtp_payload(struct rtp_header *rtp, struct str_key *payload, uint32_t* sequnce_origin, uint32_t* sequnce_new, unsigned char *all_mess, int l);
int crypto_gen_session_key(struct crypto_context *c, struct str_key* out, unsigned char label, int index_len);
int crypto_hash_rtp(struct crypto_context *c, unsigned char *payload, struct str_key* to_tag, uint64_t index);
int rtp_to_srtp(struct pthread_arguments *p_a, unsigned char *rtp, unsigned char* rtp_sps, int* l);



#endif
