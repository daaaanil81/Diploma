#include <string.h>
#include <glib.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/err.h>
#include <time.h>
#include "h264_camera.h"

#define CERT_EXPIRY_TIME (60*60*24*30) /** 30 days */

static void buf_dump_free(char *buf, size_t len) ;
int cert_init(struct pthread_arguments* p_a);
static unsigned int generic_func(unsigned char *o, X509 *x, const EVP_MD *md);
static unsigned int sha_1_func(unsigned char *o, X509 *x);
static void dump_cert(struct pthread_arguments *cert);
void dtls_fingerprint_free(struct pthread_arguments *cert);