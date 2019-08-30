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
int setup_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, unsigned int port_camera, char* session)
{
    char setup[REQUEST];
    char answer[SETUP_BUFFER_SIZE];
    char* t1;
    char* t2;
    sprintf(setup, "%s%s%s%d-%d\r\n\r\n", setup_camera_first, host, setup_camera_second, port_camera, port_camera + 1);
    printf("%s\n", setup);
    if(write(camerafd, setup, strlen(setup)) < 0)
    {
        return 1;
    }
    if(read(camerafd, answer, sizeof(answer)) < 0)
    {
        return 1;
    }
    printf("%s\n", answer);
    printf("setup_to_camera\n\n\n");
    t1 = strstr(answer, "Session");
    t1 += 9;
    t2 = strstr(t1, ";");
    strncpy(session, t1, t2 - t1);
    if(DEBUG)
        printf("%s\n", answer);
    return 0;
}
int play_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, char* session)
{
    char play[REQUEST];
    char answer[PLAY_BUFFER_SIZE];
    char* t1;
    char* t2;
    sprintf(play, "%s%s%s%s%s", play_camera_first, host, play_camera_second, session, play_camera_thirt);
    if(write(camerafd, play, strlen(play)) < 0)
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

int sdpParse(char* des, char* flag,char* offer)
{
    char version[] = "v=0\r\no=Daniil_SDP_PARTA ";
    char sdp_f[] = " 0 IN IP4 0.0.0.0\r\ns=-\r\nt=0 0\r\nm=video 0 RTP/AVP 96\r\nc=IN IP4 0.0.0.0\r\na=sendrecv\r\n";
    char sdp_e[] = "\r\na=ice-ufrag:XJ0D\r\na=ice-pwd:curBadEHKRtc/CDXsoCKMaKM\r\na=rtpmap:96 H264/90000\r\n";
    strncpy(flag,"XJ0D",sizeof("XJ0D"));
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
void create_ice(char* ice_server, unsigned int port_ice)
{
    printf("Ice create\n\n");
    char hostname[50];
    char *IPbuffer;
    struct hostent *host_entry;
    gethostname(hostname, sizeof(hostname));
    printf("%s\n", hostname);
    host_entry = gethostbyname(hostname);
    IPbuffer = inet_ntoa(*((struct in_addr*)
                           host_entry->h_addr_list[0]));
    printf("%s\n", IPbuffer);
    sprintf(ice_server, "%s%s %d%s", ice_candidate_first, IPbuffer, port_ice, ice_candidate_second);
    if(DEBUG)
        printf("%s\n", ice_server);
}
