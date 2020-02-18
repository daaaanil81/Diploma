#ifndef _CRYPTO_
#define _CRYPTO_
#include <string.h>
#include <glib.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/err.h>
#include <time.h>
#include "pthread_arguments.h"
void aes_ctr(unsigned char *out, struct str_key *in, EVP_CIPHER_CTX *ecc, const unsigned char *iv);
void aes_ctr_no_ctx(unsigned char *out, struct str_key *in, const unsigned char *key, const EVP_CIPHER *ciph,
						   const unsigned char *iv);
void prf_n(struct str_key *out, const unsigned char *key, const EVP_CIPHER *ciph, const unsigned char *x);
int crypto_gen_session_key(struct crypto_context *c, struct str_key *out, unsigned char label, int index_len);




#endif
