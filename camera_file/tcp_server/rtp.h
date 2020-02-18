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
#include "crypto.h"
struct rtp_header {
    char all_mark[2];
    uint16_t seq_num;
    uint32_t ssrc;
    uint32_t timestamp;
};

void rtp_init(struct pthread_arguments *);
int crypto_init_session_key_rtp(struct crypto_context* );
int check_session_keys_rtp(struct crypto_context *c);
int crypto_encrypt_rtp(struct crypto_context *, struct str_key *, uint32_t , uint64_t );
unsigned int rtp_sps_parse(unsigned char *rtp, unsigned char *sps, unsigned int sequnce, struct pthread_arguments *p_a);
unsigned int rtp_payload(struct rtp_header *, struct str_key *, uint32_t *, uint32_t *, unsigned char *, struct str_key *, struct pthread_arguments *, int );
int crypto_hash_rtp(struct crypto_context *, unsigned char *, struct str_key *, uint64_t );
int rtp_to_srtp(struct pthread_arguments *, unsigned char *, unsigned char* , int* );



#endif
