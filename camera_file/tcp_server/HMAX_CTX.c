#include "HMAX_CTX.h"

void Integrity(struct iovec* iov, unsigned int iov_cnt, char *pwd, char *digest)
{
    int i;
    HMAC_CTX *ctx;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    ctx = HMAC_CTX_new();
#else
    HMAC_CTX ctx_s;
    HMAC_CTX_init(&ctx_s);
    ctx = &ctx_s;
#endif
    /* do we need to SASLprep here? */
    HMAC_Init_ex(ctx, pwd, strlen(pwd), EVP_sha1(), NULL);
    printf("Test2\n");
    for (i = 0; i < iov_cnt; i++)
        HMAC_Update(ctx, (unsigned char*)iov[i].iov_base, iov[i].iov_len);
    HMAC_Final(ctx, (unsigned char*)digest, NULL);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    HMAC_CTX_free(ctx);
#else
    HMAC_CTX_cleanup(ctx);
#endif
}
