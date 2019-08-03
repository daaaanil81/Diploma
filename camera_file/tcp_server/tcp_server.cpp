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

int main(int argv, char **argc)
{
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    uint clientlen;
    int sockfd;
    uint port = 8444;
    int res;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cout << "Error with socket." << std::endl;
        return 0;
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
    {
        std::cout << "Error with bind" << std::endl;
        perror("bind");
        return 0;
    }
    listen(sockfd, 10);
    for (;;)
    {
        clientlen = sizeof(clientaddr);
        if (accept(sockfd, (struct sockaddr *)&clientaddr, &clientlen) > 0)
        {
            std::cout << "Connection: " << inet_ntoa(clientaddr.sin_addr) << ":" << ntohs(clientaddr.sin_port) << std::endl;            
        }
    }
    close(sockfd);
    return 0;
}