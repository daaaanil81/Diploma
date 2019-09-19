#include "h264_camera.h"
#include "Crc32.h"
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

int sdpParse(char* des, char* flag,char* answer, char*ice)
{
    char version[] = "v=0\r\n"
    "o=- ";
    char sdp_f[] = " 2 IN IP4 0.0.0.0\r\n"
    "s=-\r\n"
    "t=0 0\r\n"
    "m=video 0 RTP/AVP 96\r\n"
    "c=IN IP4 0.0.0.0\r\n"
    "a=ice-ufrag:sEMT\r\n"
    "a=ice-pwd:hvOt0NM+iKHs4rFN41uK2h/h\r\n"
    "a=ice-options:trickle\r\n"
    "a=sendonly\r\n"
    "a=rtpmap:96 H264/90000\r\n"
    "a=fmtp:96 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f\r\n";
    
    strncpy(flag,"sEMT",sizeof("sEMT"));
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
    sprintf(answer, "%s%s%s", version, sess_version, sdp_f);
    if(DEBUG)
        printf("%s\n", answer);
    return 0;
}
void create_ice(char* ice_server, unsigned int port_ice,char* ip_server)
{
    char hostname[50];
    char *IPbuffer;
    struct hostent *host_entry;
    gethostname(hostname, sizeof(hostname));
    printf("%s\n", hostname);
    host_entry = gethostbyname(hostname);
    IPbuffer = inet_ntoa(*((struct in_addr*)
                           host_entry->h_addr_list[0]));
    printf("%s\n", IPbuffer);
    memset((char*)ip_server, 0, sizeof(ip_server));
    strcpy((char*)ip_server, IPbuffer);
    sprintf(ice_server, "%s%s %d%s", ice_candidate_first, IPbuffer, port_ice, ice_candidate_second);
    if(DEBUG)
        printf("%s\n", ice_server);
}
void iceParse(char* ice, char* ip_browser, char* ip_server, unsigned int& port, char* uflag_browser)
{
    char* t1 = NULL;
    char* t2;
    char time_port[7] = {0};
    int i = 0;
    t1 = strstr(ice, " ");
    t1 += 1;
    while(i < 3)
    {
        t1 = strstr(t1, " ");
        t1 += 1;
        i++;
    }
    t2 = strstr(ice, "local");
    if(t2 != NULL)
    {
        strcpy(ip_browser, ip_server);
        t2 = strstr(t1, " ");
    }
    else
    {
        t2 = strstr(t1, " ");
        strncpy(ip_browser, t1, t2 - t1);
        ip_browser[t2 - t1] = '\0';
    }
    t1 += t2 - t1 + 1;
    t2 = strstr(t1, " ");
    strncpy(time_port, t1, t2 - t1);
    port = atoi(time_port);
    t1 = strstr(ice, "ufrag");
    t1 += 6;
    t2 = strstr(t1, " ");
    strncpy(uflag_browser, t1, t2 - t1);
    uflag_browser[t2 - t1] = '\0';
}
//int generationSTUN(char* ip_server, char* ip_browser, unsigned int ice_port_browser, unsigned int ice_port_server, char* name)
//{
//    int n = 0;
//    int rndfd;
//    int sockfd;
//    unsigned int next_step = 0;
//    unsigned int size_all_stun = 0;
//    struct sockaddr_in stun_addr;
//    struct sockaddr_in stun_browser_addr;
//    struct stun_message sm = {htons(1), 0, htonl(0x2112A442)};
//    struct stun_request sr = {htons(1), 0, htonl(0x2112A442)};
//
//    // Enter Id for header
//    rndfd=open("/dev/urandom", 0);
//    read(rndfd, (char*)sm.id, sizeof sm.id);
//    read(rndfd, (char*)sr.id, sizeof sr.id);
//    close(rndfd);
//    // Socket server
//    sockfd = socket(AF_INET, SOCK_DGRAM, 0); /// UDP
//    bzero(&stun_addr, sizeof(stun_addr));
//    stun_addr.sin_family = AF_INET;
//    stun_addr.sin_port = htons(ice_port_server);
//    n = bind(sockfd,(struct sockaddr *)&stun_addr,sizeof(stun_addr)); /// Was Opened port for stun request
//    if(n != 0)
//    {
//        printf("Error with bind in Stun\n");
//        return 1;
//    }
//    printf("Bind OK\n");
//    // Socket for send message
//    bzero(&stun_browser_addr, sizeof(stun_browser_addr));
//    stun_browser_addr.sin_family = AF_INET;
//    stun_browser_addr.sin_port = htons(ice_port_browser);
//    inet_pton(AF_INET, ip_browser, &stun_browser_addr.sin_addr);
//    printf("Stun request\n");
//    n = sendto(sockfd, &sr, 20, 0, (struct sockaddr*)&stun_browser_addr, sizeof(stun_browser_addr));
//    // USERNAME
//    printf("USERNAME\n");
//    sm.data[next_step] = USERNAME >> 8;
//    next_step+=1;
//    sm.data[next_step] = USERNAME;
//    next_step+=1;
//    sm.data[next_step] = strlen(name) >> 8;
//    next_step+=1;
//    sm.data[next_step] = strlen(name);
//    next_step+=1;
//    strcat((char*)(sm.data + next_step), name);
//    next_step += strlen(name);
//    printf("Value = %s\n", name);
//    sm.data[next_step] = 0x0;
//    next_step+=1;
//    sm.data[next_step] = 0x0;
//    next_step+=1;
//    sm.data[next_step] = 0x0;
//    next_step+=1;
//    size_all_stun = STUN_HEADER_ATTR + 3 + strlen(name);
//    // ICE-CONTROLLING
//    printf("ICE-CONTROLLING\n");
//    sm.data[next_step] = ICE_CONTROLLING >> 8;
//    next_step+=1;
//    sm.data[next_step] = ICE_CONTROLLING;
//    next_step+=1;
//    sm.data[next_step] = ICE_CONTROLLING_LENGTH >> 8;
//    next_step+=1;
//    sm.data[next_step] = ICE_CONTROLLING_LENGTH;
//    next_step+=1;
//    rndfd=open("/dev/urandom", 0);
//    read(rndfd, (char*)(sm.data+next_step), ICE_CONTROLLING_LENGTH);
//    close(rndfd);
//    next_step+=ICE_CONTROLLING_LENGTH;
//    size_all_stun += (STUN_HEADER_ATTR + ICE_CONTROLLING_LENGTH);
//    // PRIORITY
//    printf("PRIORITY\n");
//    sm.data[next_step] = PRIORITY >> 8;
//    next_step+=1;
//    sm.data[next_step] = PRIORITY;
//    next_step+=1;
//    sm.data[next_step] = PRIORITY_LENGTH >> 8;
//    next_step+=1;
//    sm.data[next_step] = PRIORITY_LENGTH;
//    next_step+=1;
//    sm.data[next_step] = PROPRITY_VALUE >> 24;
//    next_step+=1;
//    sm.data[next_step] = PROPRITY_VALUE >> 16;
//    next_step+=1;
//    sm.data[next_step] = PROPRITY_VALUE >> 8;
//    next_step+=1;
//    sm.data[next_step] = PROPRITY_VALUE;
//    next_step+=1;
//    size_all_stun += STUN_HEADER_ATTR + PRIORITY_LENGTH;
//    printf("Value = %d\n", PROPRITY_VALUE);
//    // USE_CANDIDATE
//    printf("USE-CANDIDATE\n");
//    sm.data[next_step] = USE_CANDIDATE >> 8;
//    next_step+=1;
//    sm.data[next_step] = USE_CANDIDATE;
//    next_step+=1;
//    sm.data[next_step] = USE_CANDIDATE_LENGTH >> 8;
//    next_step+=1;
//    sm.data[next_step] = USE_CANDIDATE_LENGTH;
//    next_step+=1;
//    size_all_stun += STUN_HEADER_ATTR + USE_CANDIDATE_LENGTH;
//    // MESSAGE-INTEGRITY
//    printf("MESSAGE-INTEGRITY\n");
//    sm.data[next_step] = MESSAGE_INTEGRITY >> 8;
//    next_step+=1;
//    sm.data[next_step] = MESSAGE_INTEGRITY;
//    next_step+=1;
//    sm.data[next_step] = MESSAGE_INTEGRITY_LENGTH >> 8;
//    next_step+=1;
//    sm.data[next_step] = MESSAGE_INTEGRITY_LENGTH;
//    next_step+=1;
//    n = 3;
//    while(n >= 0)
//    {
//        sm.data[next_step] = MESSAGE_INTEGRITY_VALUE_1 >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//    n = 3;
//    while(n >= 0)
//    {
//        sm.data[next_step] = MESSAGE_INTEGRITY_VALUE_2 >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//    n = 3;
//    while(n >= 0)
//    {
//        sm.data[next_step] = MESSAGE_INTEGRITY_VALUE_3 >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//    n = 3;
//    while(n >= 0)
//    {
//        sm.data[next_step] = MESSAGE_INTEGRITY_VALUE_4 >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//    n = 3;
//    while(n >= 0)
//    {
//        sm.data[next_step] = MESSAGE_INTEGRITY_VALUE_5 >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//    size_all_stun += STUN_HEADER_ATTR + MESSAGE_INTEGRITY_LENGTH;
//    uint32_t crc = Crc32((unsigned char*)sm, size_all_stun);
//    printf("Crc32 result = %x\n", crc);
//    // SEND
//    sm.data_len = htons(size_all_stun);
//    n = sendto(sockfd, &sm, (STUN_HEADER + size_all_stun), 0, (struct sockaddr*)&stun_browser_addr, sizeof(stun_browser_addr));
//    if(n < STUN_HEADER + size_all_stun)
//    {
//        printf("Error with sendto\n");
//        printf("Sendto n = %d\n", n);
//        return 1;
//    }
//    printf("Sendto n = %d\n", n);
//    return 0;
//}
int generationSTUN(char* ip_server, char* ip_browser, unsigned int ice_port_browser, unsigned int ice_port_server, char* name)
{
    int n = 0;
    int rndfd;
    int sockfd;
    unsigned int next_step = 0;
    unsigned int size_all_stun = 0;
    struct sockaddr_in stun_addr;
    struct sockaddr_in stun_browser_addr;
    unsigned char data_req[276];
    // Socket server
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); /// UDP
    bzero(&stun_addr, sizeof(stun_addr));
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_port = htons(ice_port_server);
    n = bind(sockfd,(struct sockaddr *)&stun_addr,sizeof(stun_addr)); /// Was Opened port for stun request
    if(n != 0)
    {
        printf("Error with bind in Stun\n");
        return 1;
    }
    printf("Bind OK\n");
    // Socket for send message
    bzero(&stun_browser_addr, sizeof(stun_browser_addr));
    stun_browser_addr.sin_family = AF_INET;
    stun_browser_addr.sin_port = htons(ice_port_browser);
    inet_pton(AF_INET, ip_browser, &stun_browser_addr.sin_addr);
    //
    
    data_req[next_step] = 0x00;
    next_step+=1;
    data_req[next_step] = 0x01;
    next_step+=1;
    
    next_step+=1;
    
    next_step+=1;
    data_req[next_step] = 0x2112A442 >> 24;
    next_step+=1;
    data_req[next_step] = 0x2112A442 >> 16;
    next_step+=1;
    data_req[next_step] = 0x2112A442 >> 8;
    next_step+=1;
    data_req[next_step] = 0x2112A442;
    next_step+=1;
    // Enter Id for header
    rndfd=open("/dev/urandom", 0);
    read(rndfd, (unsigned char*)(data_req+next_step), 12);
    close(rndfd);
    next_step += 12;
    // USERNAME
    printf("USERNAME\n");
    data_req[next_step] = USERNAME >> 8;
    next_step+=1;
    data_req[next_step] = USERNAME;
    next_step+=1;
    data_req[next_step] = strlen(name) >> 8;
    next_step+=1;
    data_req[next_step] = strlen(name);
    next_step+=1;
    strcat((char*)(data_req + next_step), name);
    next_step += strlen(name);
    printf("Value = %s\n", name);
    data_req[next_step] = 0x0;
    next_step+=1;
    data_req[next_step] = 0x0;
    next_step+=1;
    data_req[next_step] = 0x0;
    next_step+=1;
    size_all_stun = STUN_HEADER_ATTR + 3 + strlen(name);
    // ICE-CONTROLLING
    printf("ICE-CONTROLLING\n");
    data_req[next_step] = ICE_CONTROLLING >> 8;
    next_step+=1;
    data_req[next_step] = ICE_CONTROLLING;
    next_step+=1;
    data_req[next_step] = ICE_CONTROLLING_LENGTH >> 8;
    next_step+=1;
    data_req[next_step] = ICE_CONTROLLING_LENGTH;
    next_step+=1;
    rndfd=open("/dev/urandom", 0);
    read(rndfd, (unsigned char*)(data_req+next_step), ICE_CONTROLLING_LENGTH);
    close(rndfd);
    next_step+=ICE_CONTROLLING_LENGTH;
    size_all_stun += (STUN_HEADER_ATTR + ICE_CONTROLLING_LENGTH);
    // PRIORITY
    printf("PRIORITY\n");
    data_req[next_step] = PRIORITY >> 8;
    next_step+=1;
    data_req[next_step] = PRIORITY;
    next_step+=1;
    data_req[next_step] = PRIORITY_LENGTH >> 8;
    next_step+=1;
    data_req[next_step] = PRIORITY_LENGTH;
    next_step+=1;
    data_req[next_step] = PROPRITY_VALUE >> 24;
    next_step+=1;
    data_req[next_step] = PROPRITY_VALUE >> 16;
    next_step+=1;
    data_req[next_step] = PROPRITY_VALUE >> 8;
    next_step+=1;
    data_req[next_step] = PROPRITY_VALUE;
    next_step+=1;
    size_all_stun += STUN_HEADER_ATTR + PRIORITY_LENGTH;
    printf("Value = %d\n", PROPRITY_VALUE);
    // USE_CANDIDATE
    printf("USE-CANDIDATE\n");
    data_req[next_step] = USE_CANDIDATE >> 8;
    next_step+=1;
    data_req[next_step] = USE_CANDIDATE;
    next_step+=1;
    data_req[next_step] = USE_CANDIDATE_LENGTH >> 8;
    next_step+=1;
    data_req[next_step] = USE_CANDIDATE_LENGTH;
    next_step+=1;
    size_all_stun += STUN_HEADER_ATTR + USE_CANDIDATE_LENGTH;
    // MESSAGE-INTEGRITY
    printf("MESSAGE-INTEGRITY\n");
    data_req[next_step] = MESSAGE_INTEGRITY >> 8;
    next_step+=1;
    data_req[next_step] = MESSAGE_INTEGRITY;
    next_step+=1;
    data_req[next_step] = MESSAGE_INTEGRITY_LENGTH >> 8;
    next_step+=1;
    data_req[next_step] = MESSAGE_INTEGRITY_LENGTH;
    next_step+=1;
    n = 3;
    while(n >= 0)
    {
        data_req[next_step] = MESSAGE_INTEGRITY_VALUE_1 >> (n*8) ;
        next_step+=1;
        n--;
    }
    n = 3;
    while(n >= 0)
    {
        data_req[next_step] = MESSAGE_INTEGRITY_VALUE_2 >> (n*8) ;
        next_step+=1;
        n--;
    }
    n = 3;
    while(n >= 0)
    {
        data_req[next_step] = MESSAGE_INTEGRITY_VALUE_3 >> (n*8) ;
        next_step+=1;
        n--;
    }
    n = 3;
    while(n >= 0)
    {
        data_req[next_step] = MESSAGE_INTEGRITY_VALUE_4 >> (n*8) ;
        next_step+=1;
        n--;
    }
    n = 3;
    while(n >= 0)
    {
        data_req[next_step] = MESSAGE_INTEGRITY_VALUE_5 >> (n*8) ;
        next_step+=1;
        n--;
    }
    size_all_stun += STUN_HEADER_ATTR + MESSAGE_INTEGRITY_LENGTH;
    uint32_t crc = Crc32((unsigned char*)data_req, size_all_stun);
    printf("Crc32 result = %x\n", crc);
    //FINGERPRINT
    printf("FINGERPRINT\n");
    data_req[next_step] = crc >> 8;
    next_step+=1;
    data_req[next_step] = crc;
    next_step+=1;
    data_req[next_step] = FINGERPRINT_LENGTH >> 8;
    next_step+=1;
    data_req[next_step] = FINGERPRINT_LENGTH;
    next_step+=1;
    crc = crc ^ 0x5354554E;
    n = 3;
    while(n >= 0)
    {
        data_req[next_step] = crc >> (n*8) ;
        next_step+=1;
        n--;
    }
    size_all_stun += STUN_HEADER_ATTR + FINGERPRINT_LENGTH;
    data_req[2] = size_all_stun >> 8;
    data_req[3] = size_all_stun;
    n = sendto(sockfd, data_req, (STUN_HEADER + size_all_stun), 0, (struct sockaddr*)&stun_browser_addr, sizeof(stun_browser_addr));
    if(n < STUN_HEADER + size_all_stun)
    {
        printf("Error with sendto\n");
        printf("Sendto n = %d\n", n);
        return 1;
    }
    printf("Sendto n = %d\n", n);
    return 0;
}
