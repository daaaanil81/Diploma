#ifndef _DTLS_
#define _DTLS_
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
#define CERT_EXPIRY_TIME (60*60*24*30) /** 30 days */

static char ciphers_str[1024];
static int num_crypto_suites = 2;
static void buf_dump_free(char *buf, size_t len) ;
int cert_init(struct pthread_arguments* p_a);
static unsigned int generic_func(unsigned char *o, X509 *x, const EVP_MD *md);
static unsigned int sha_1_func(unsigned char *o, X509 *x);
static void dump_cert(struct pthread_arguments *cert);
void dtls_fingerprint_free(struct pthread_arguments *cert);
void dtls_init();
void crypto_init_main();
int try_connect(struct dtls_connection *d);
int dtls_connection_init(struct pthread_arguments* p_a);
void dtls_connection_cleanup(struct dtls_connection *c);
int dtls(struct pthread_arguments* p_a, unsigned char* buf, unsigned int len);
int dtls_setup_crypto(struct dtls_connection *d, struct crypto_context *crypto, struct crypto_context *crypto_rtcp, struct crypto_context* crypto_from_camera);
int readDTLS(int fd, unsigned char* dtls_answers);
static int aes_cm_encrypt_rtp(struct crypto_context *, struct str_key* payload, uint32_t ssrc, uint64_t index);
static int hmac_sha1_rtp(struct crypto_context *, unsigned char *payload, struct str_key* in, uint64_t index);
static int hmac_sha1_rtcp(struct crypto_context *c, unsigned char *payload, struct str_key *in);
static int aes_cm_session_key_init(struct crypto_context *c);
static int evp_session_key_cleanup(struct crypto_context *c);
void crypto_cleanup(struct crypto_context *c);

#endif
