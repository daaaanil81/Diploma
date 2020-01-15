#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/hmac.h>
#include <sys/uio.h>

void Integrity(struct iovec* iov, unsigned int iov_cnt, char *pwd, char *digest);
