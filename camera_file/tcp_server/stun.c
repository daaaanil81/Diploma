#include "stun.h"
unsigned int stun_header(struct msghdr *mh, struct iovec* iov, struct header* hdr, unsigned char* buf, unsigned int size)
{
    mh->msg_iov = iov;
    mh->msg_iovlen = 1;
    iov->iov_base = hdr;
    hdr->msg_type = htons(HEADER_TYPE);
    hdr->magick = htonl(MAGICK);
    hdr->data_len = 0;
    int rndfd;
    buf[size++] = HEADER_TYPE >> 8;
    buf[size++] = HEADER_TYPE;
    size += 2;
    short t = 3;
    while(t >= 0)
    {
	    buf[size++] = MAGICK >> (8 * t);
	    t--;
    }
    rndfd=open("/dev/urandom", 0);
    read(rndfd, (unsigned char*)(buf+size), 12);
    memcpy(hdr->id, (unsigned char*)(buf+size), sizeof(hdr->id));
    size += 12;
    close(rndfd);
    return size;
}
int stun_request(struct pthread_arguments* p_a)
{
    struct msghdr mh;
    struct iovec iov[10];
    struct header hdr;
    struct sockaddr_in stun_to_browser;
    struct socladdr_in stun_from_server;
    unsigned int size_stun_message = 0; 
    unsigned char stun_request[100];
    memset(&stun_to_browser, 0, sizeof(stun_to_browser));
    stun_to_browser.sin_family = AF_INET; // IPv4
    stun_to_browser.sin_addr.s_addr = INADDR_ANY;
    stun_to_browser.sin_port = htons(p_a->port_ice_browser);

    memset(&stun_from_server, 0, sizeof(stun_from_server));
    stun_from_server.sin_family = AF_INET; // IPv4
    stun_from_server.sin_addr.s_addr = INADDR_ANY;
    stun_from_server.sin_port = htons(p_a->port_ice);
    if ((p_a->socket_stream = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket stream before stun");
        return -1;
    }
    if (bind(socket_stream, (const struct sockaddr *)&stun_from_server, sizeof(stun_from_server)) < 0)
    {
        perror("bind failed socket before stun");
        return -1;
    }
    size_stun_message = stun_header(mh, iov, hdr, stun_request, size_stun_message);
    sendto(p_a->socket_stream, stun_request, size_stun_message, 0, (struct sockaddr *)&stun_to_browser, sizeof(stun_to_browser));
    return 0;
}