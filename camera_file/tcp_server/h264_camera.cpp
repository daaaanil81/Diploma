#include "h264_camera.h"
int connect_camera(struct sockaddr_in& saddr, int& camerafd, char* host)
{
    camerafd = socket(AF_INET, SOCK_STREAM, 0);
    if(camerafd < 0)
    {
        printf("Error with socket to camera\n");
        return 1;
    }
    else
    {
        printf("Camerafd = %d\n", camerafd);
    }
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(TCP_PORT);
    printf("host = %s\n",host);
    inet_aton(host, &saddr.sin_addr);
    int res = connect(camerafd, (struct sockaddr* )&saddr, sizeof(saddr));
    if(res != 0)
    {
        printf("Error with connect to camera\n");
        perror("connect");
        return 1;
    }
    printf("Connect to camera successful!\n");
    return 0;
}
int option_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host)
{
    char option[REQUEST];
    char answer[OPTION_BUFFER_SIZE];
    sprintf(option, "%s%s%s", option_camera_first, host, option_camera_second);
    if(write(camerafd, option, strlen(option)) < 0)
    {
        return 1;
    }
    if(read(camerafd, answer, sizeof(answer)) < 0)
    {
        return 1;
    }
    if(DEBUG)
        printf("%s\n", answer);
    return 0;
}
int describe_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* ans)
{
    char describe[REQUEST];
    char answer[DESCRIBE_BUFFER_SIZE];
    sprintf(describe, "%s%s%s", describe_camera_first, host, describe_camera_second);
    if(write(camerafd, describe, strlen(describe)) < 0)
    {
        return 1;
    }
    if(read(camerafd, answer, sizeof(answer)) < 0)
    {
        return 1;
    }
    strcpy(ans,answer);
    if(DEBUG)
        printf("%s\n", answer);
    return 0;
}
int sdpParse(char* des, char* offer)
{
    char version[] = "v=0\r\no=Daniil_SDP_PARTA ";
    char sdp_f[] = " 0 IN IP4 0.0.0.0\r\ns=-\r\nt=0 0\r\nm=video 0 RTP/AVP 96\r\nc=IN IP4 0.0.0.0\r\na=sendrecv\r\n";
    char sdp_e[] = "\r\na=ice-pwd:7c10144cbaaeed7af228a76356db9d29\r\na=ice-ufrag:21c167f4\r\na=rtpmap:96 H264/90000\r\n";
    char fmtp[60] = {0};
    char sess_version[20] = {0};
    char* t;
    char* time;
    char* payload_type;
    
    t = strstr(des, "o=");
    t = strstr(t, " ");
    t += 1;
    time = t;
    time = strstr(t, " ");
    strncpy(sess_version, t, time - t);
    t = strstr(des, "a=fmtp");
    /*
     payload_type = t + 7;
     strncpy(fmtp, t, payload_type - t);
     strcat(fmtp, "120 ");
     t = strstr(des, "packetization");
     */
    time = strstr(des, "sprop-parameter-sets");
    strncat(fmtp, t, time - t - 2);
    sprintf(offer, "%s%s%s%s%s", version, sess_version, sdp_f, fmtp, sdp_e);
    if(DEBUG)
        printf("%s\n", offer);
    return 0;
}
