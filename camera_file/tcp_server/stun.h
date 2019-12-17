#include "h264_camera.h"
#include <zlib.h>
int stun_request(struct pthread_arguments* p_a);
unsigned int stun_header(struct msghdr *mh, struct iovec* iov, struct header* hdr, unsigned char* buf, unsigned int size);


