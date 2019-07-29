#include <iostream>
#include <stdio.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/poll.h>
struct header_stun
{
    uint16_t mType;
    uint16_t mLength;
    uint32_t mCookie;
    uint32_t mId[3];
};
struct response_stun
{
    struct header_stun h_stun;
    unsigned char buf[256];
};
int main(int argv, char **argc)
{
    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    int sockfd;
    FILE *fdRand;
    uint port = 19302;
    int rto = 500;
    struct header_stun h_buffer = {htons(1), 0, htonl(0x2112A442)};
    struct response_stun r_stun;
    int length_r_sun = sizeof(r_stun);
    int res;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        std::cout << "Error with socket." << std::endl;
        return 0;
    }   
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, "64.233.177.127", &servaddr.sin_addr); //stun.l.google.com
    fdRand = fopen("/dev/urandom", "r");
    fread(&h_buffer.mId, sizeof(h_buffer.mId), 1, fdRand);
    fclose(fdRand);
    while (true)
    {
        res = sendto(sockfd, &h_buffer, sizeof(h_buffer), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        if (res != sizeof(h_buffer))
        {
            std::cout << "Error with send STUN request" << std::endl;
            break;
        }
        struct pollfd pfd = {
            sockfd,
            0x0001, //POLLIN
        };
        res = poll(&pfd, 1, rto);
        if (res == 0)
        {
            rto *= 2;
            std::cout << "Request..." << std::endl;
            continue;
        }
        if (res < 0)
            std::cout << "Error with poll" << std::endl;
        if (res > 0)
            break;
        if (rto > 600000)
            rto = 500;
    }
    res = recvfrom(sockfd, &r_stun, sizeof(r_stun), 0, (struct sockaddr *)&servaddr, (socklen_t*)&length_r_sun);
    if(ntohs(r_stun.h_stun.mType) != 0x0101 && ntohl(r_stun.h_stun.mCookie) != 0x2112A442)
    {
        std::cout << "Error with response" << std::endl;
    }
    if(memcmp(r_stun.h_stun.mId, h_buffer.mId, sizeof(h_buffer.mId) != 0))
    {
        std::cout << "Error with id message" << std::endl;
    }
    if((res - 20) == ntohs(r_stun.h_stun.mLength))
    {
        uint16_t aType = r_stun.buf[0] << 8 | r_stun.buf[1];    
        std::cout << "aType = " << (r_stun.buf[0] << 8 | r_stun.buf[1]) << std::endl;
        uint16_t aLength = r_stun.buf[2] << 8 | r_stun.buf[3];
        std::cout << "aLength = " << (r_stun.buf[2] << 8 | r_stun.buf[3]) << std::endl;
        //one byte anything
        uint8_t aFamily = r_stun.buf[5];
        uint16_t aPort = (r_stun.buf[6]^0x21) << 8 | (r_stun.buf[7]^0x12);
        std::cout << "aPort = " << ((r_stun.buf[6]^0x21) << 8 | (r_stun.buf[7]^0x12)) << std::endl; 
        char ip[21];
        ip[20] = '\0';
        sprintf(ip, "%d.%d.%d.%d", r_stun.buf[8]^0x21, r_stun.buf[9]^0x12, r_stun.buf[10]^0xA4, r_stun.buf[11]^0x42);
        std::cout << "IP = " << ip << std::endl;
    }

    close(sockfd);
    return 0;
}