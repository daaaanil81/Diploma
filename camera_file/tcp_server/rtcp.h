#ifndef _RTCP_
#define _RTCP_
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/err.h>
#include <time.h>
#include "pthread_arguments.h"
#include "crypto.h"
struct rtcp_header {
    char all_mark[2];
    uint16_t length;
    uint32_t ssrc;
};
int crypto_init_session_key_rtcp(struct crypto_context *c);
int check_session_keys_rtcp(struct crypto_context *c);
void rtcp_payload(struct rtcp_header* rtcp_h, struct str_key* payload, unsigned char* s, int length);
int rtcp_savp_to_avp(struct crypto_context *crypto_rtcp, unsigned char *rtcp, int* length); 
int rtcp_avp_to_savp(struct crypto_context *crypto_from_camera, unsigned char *rtcp, int* length, uint32_t* index_rtcp);
int crypto_decrypt_rtcp(struct crypto_context *c, struct str_key *payload, uint32_t ssrc, uint64_t index);




#endif
