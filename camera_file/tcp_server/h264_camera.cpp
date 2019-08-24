#include "h264_camera.h"
int connect_camera(struct sockaddr_in& saddr, int& camerafd, char* host)
{
    camerafd = socket(AF_INET, SOCK_STREAM, 0);
    if(camerafd < 0)
    {
        std::cout << "Error with socket to camera" << std::endl;
        return 1;
    }
    else
    {
        std::cout << "Camerafd = " << camerafd << std::endl;
    }
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(TCP_PORT);
    std::cout << host << std::endl;
    inet_aton(host, &saddr.sin_addr);
    //inet_pton(AF_INET, host, &saddr.sin_addr);
    int res = connect(camerafd, (struct sockaddr* )&saddr, sizeof(saddr));
    if(res != 0)
    {
        std::cout << "Error with connect to camera" << std::endl;
        perror("connect");
        return 1;
    }
    std::cout << "Connect to camera successful!" << std::endl;
    return 0;
}
int option_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host)
{
    char option[REQUEST];
    char answer[OPTION_BUFFER_SIZE];
    sprintf(option, "%s%s%s", option_camera_first, host, option_camera_second);
    write(camerafd, option, strlen(option));
    read(camerafd, answer, sizeof(answer));
    std::cout << answer << std::endl;
    return 0;
}
int describe_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* ans)
{
    char describe[REQUEST];
    char answer[DESCRIBE_BUFFER_SIZE];
    sprintf(describe, "%s%s%s", describe_camera_first, host, describe_camera_second);
    write(camerafd, describe, strlen(describe));
    read(camerafd, answer, sizeof(answer));
    strcpy(ans,answer);
    std::cout << "Size Description = " << strlen(answer) << std::endl;
    return 0;
}
