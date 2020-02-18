#include <time.h>
#include "h264_camera.h"

int connect_camera(struct sockaddr_in* saddr, int* camerafd, char* host)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
        return 1;
    bzero(saddr, sizeof(saddr));
    saddr->sin_family = AF_INET;
    saddr->sin_port = htons(TCP_PORT);
    printf("host = %s\n",host);
    inet_aton(host, &saddr->sin_addr);
    if(connect(fd, (struct sockaddr*)saddr, sizeof(struct sockaddr_in)) != 0)
        return 1;
    *camerafd = fd;
    return 0;
}
int option_to_camera(int camerafd, char* host)
{
    char option[REQUEST];
    char answer[OPTION_BUFFER_SIZE];
    sprintf(option, "%s%s%s", option_camera_first, host, option_camera_second);
    if(write(camerafd, option, strlen(option)) < 0)
        return 1;
    if(read(camerafd, answer, sizeof(answer)) < 0)
       return 1;
    if(DEBUG)
        printf("%s\n", answer);
    return 0;
}
int describe_to_camera(int camerafd, char* host, char* ans) /// Description camera
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
int setup_to_camera(int camerafd, char* host, unsigned int port_camera, char* session, unsigned int* port_udp)
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
    char p[10] = {0};
    t1 = strstr(answer, "server_port=");
    t1 += 12;
    t2 = strstr(t1, "-");
    strncpy(p, t1, t2 - t1);
    *port_udp = atoi(p) + 1;
    if(DEBUG)
        printf("%s\n", answer);
    return 0;
}
int play_to_camera(int camerafd, char* host, char* session)
{
    char play[REQUEST];
    char answer[PLAY_BUFFER_SIZE];
    char* t1;
    char* t2;
    sprintf(play, "%s%s%s%s%s", play_camera_first, host, play_camera_second, session, play_camera_thirt);
    if(write(camerafd, play, strlen(play)) < 0)
        return 1;
    if(read(camerafd, answer, sizeof(answer)) < 0)
        return 1;
    if(DEBUG)
        printf("%s\n", answer);
    return 0;
}
int teardown_to_camera(int sockfd, char* host, char* session)
{
	char teardown_text[512] = {0};
	sprintf(teardown_text, "TEARDOWN rtsp://%s/axis-media/media.amp RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: RTSPClient\r\nSession: %s\r\n\r\n", host, session);
	printf("%s", teardown_text);
	char buf[512] = {0};
	if (send(sockfd, teardown_text, strlen(teardown_text), 0) < 0)
		return 1;
	if (recv(sockfd, buf, sizeof(buf), 0) < 0)
		return 1;
	return 0;
}

int sdpParse(struct pthread_arguments* p_a)
{
    char version[] = "v=0\r\no=-";
    char sdp_f[] = "a=rtpmap:102 H264/90000\r\n";
    char fingerprint[200];
    char attribute_c[40] = {0};
    char attribute_m[30] = {0};
    char attribute_rtcp_port[25] = {0};
    char attribute_uflag[30] = {0};
    char attribute_pwd[50];
    char fmtp[200] = {0};
    char sess_version[20] = {0};
    char* t;
    char* time;
    char* payload_type;
    char sdp_sps[30];
    unsigned short size_sps;
    char sdp_pps[15];
    unsigned short size_pps;
    
    t = strstr(p_a->sdp_camera, "o=");
    t = strstr(t, " ");
    t += 1;
    time = t;
    time = strstr(t, " ");
    strncpy(sess_version, t, time - t);

    sess_version[time-t] = '\0';
    t = strstr(p_a->sdp_camera, "a=fmtp");
    time = strstr(t, ":");
    strncpy(fmtp, t, time - t);
    strcat(fmtp, ":102 ");
    time += 4;
    t = strstr(time, "sprop");
    t -= 2;
    strncat(fmtp, time, t - time); 
    strcat(fmtp, "\r\n");

    //SPS and PPS
    time = strstr(p_a->sdp_camera, "sets=");
    time += 5;
    t = strstr(time, ",");
    strncpy(sdp_sps, time, t - time);
    sdp_sps[t-time] = '\0';    
    p_a->size_sps = t - time;
    printf("SPS: %s\n", sdp_sps);
    p_a->size_sps = base64_decode((char*)sdp_sps, (char*)p_a->sps);
    printf("Size: %d\n", p_a->size_sps);
    for(int j = 0; j < p_a->size_sps; j++)
    {
        printf("%02X ", p_a->sps[j]);
    }
    printf("\n");
    t++;

    time = strstr(t, "\r");
    strncpy(sdp_pps, t, time - t);
    sdp_pps[time-t] = '\0';
    p_a->size_pps = time - t;
    printf("PPS: %s\n", sdp_pps);
    p_a->size_pps = base64_decode((char* )sdp_pps, (char* )p_a->pps);
    printf("Size: %d\n", p_a->size_pps);
    for(int j = 0; j < p_a->size_pps; j++)
    {
        printf("%02X ", p_a->pps[j]);
    }
    printf("\n");
    strcpy(p_a->pwd_server.s, "YdvF780e9vG5SNCtkAur1ShFEj");
    p_a->pwd_server.len = strlen("YdvF780e9vG5SNCtkAur1ShFEj");

    sprintf(p_a->sdp_answer, "%s %s 2 IN IP4 127.0.0.1\r\n", version, sess_version);
    strcat(p_a->sdp_answer, "s=Daniil Team\r\n");
    sprintf(attribute_c, "c=IN IP4 %s\r\n", p_a->ip_server);
    strcat(p_a->sdp_answer, attribute_c);
    strcat(p_a->sdp_answer, "t=0 0\r\n");
    sprintf(attribute_m, "m=video %d RTP/SAVP 102\r\n", p_a->port_ice);
    strcat(p_a->sdp_answer, attribute_m);
    strcat(p_a->sdp_answer, "a=mid:0\r\n");
    strcat(p_a->sdp_answer, sdp_f);
    strcat(p_a->sdp_answer, fmtp);
    strcat(p_a->sdp_answer, "a=sendonly\r\n");
    sprintf(attribute_rtcp_port, "a=rtcp:%d\r\n", p_a->port_ice);
    strcat(p_a->sdp_answer, attribute_rtcp_port);
    strcat(p_a->sdp_answer, "a=rtcp-mux\r\n");
    strcat(p_a->sdp_answer, "a=setup:active\r\n");
    if (cert_init(p_a) < 0)
    {
        printf("Error with certificate\n");
        return -1;
    }
    time = fingerprint;
    time += sprintf(time, "a=fingerprint:sha-1 ");
    int i = 0;
    for(i = 0 ; i < p_a->attr_fingerprint.size-1; i++)
    {
        time += sprintf(time, "%02X:", p_a->attr_fingerprint.digest_fingerprint[i]);
    }
    i = p_a->attr_fingerprint.size-1;
    time += sprintf(time, "%02X\r\n", p_a->attr_fingerprint.digest_fingerprint[i]);
    strncat(p_a->sdp_answer, fingerprint, time - fingerprint);
    sprintf(attribute_uflag, "a=ice-ufrag:%s\r\n", p_a->uflag_server);
    strcat(p_a->sdp_answer, attribute_uflag);
    sprintf(attribute_pwd, "a=ice-pwd:%s\r\n", p_a->pwd_server.s);
    strcat(p_a->sdp_answer, attribute_pwd);
    strcat(p_a->sdp_answer, "a=ice-options:trickle\r\n");
    printf("FINGERPRINT: \n%s\n", fingerprint); 
    if(DEBUG)
        printf("%s\n", p_a->sdp_answer);
    return 0;
}
int create_ice(struct pthread_arguments* p_a)
{
    char hostname[20];
    unsigned char buf[50] = {0};
    size_t i = 0;
    int sockfd;
    struct sockaddr_in servaddr; 
    printf("Ip: %s\n", p_a->ip_server);
    strcpy(p_a->uflag_server, "sEMT");
    strcpy(hostname, p_a->ip_server);
    /// 74.125.134.127 stun google


    if ((p_a->socket_stream = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed");  
        return 1;
    }
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&p_a->stun_from_server, 0, sizeof(p_a->stun_from_server)); 

    servaddr.sin_family = AF_INET; // IPv4 
    inet_aton("74.125.134.127", &servaddr.sin_addr);
    servaddr.sin_port = htons(19302); 

    p_a->stun_from_server.sin_family = AF_INET; // IPv4 
    p_a->stun_from_server.sin_port = htons(p_a->port_ice); // 53532
    p_a->stun_from_server.sin_addr.s_addr = INADDR_ANY; 

    if (bind(p_a->socket_stream, (struct sockaddr *)&p_a->stun_from_server, sizeof(p_a->stun_from_server)) < 0 ) 
    { 
        perror("bind failed"); 
        return 1;
    } 
    buf[i++] = 0x00;
    buf[i++] = 0x01;
    buf[i++] = 0x00;
    buf[i++] = 0x00;
    buf[i++] = (MAGICK & 0xFF000000) >> (8 * 3);
    buf[i++] = (MAGICK & 0xFF0000) >> (8 * 2);
    buf[i++] = (MAGICK & 0xFF00) >> (8 * 1);
    buf[i++] = (MAGICK & 0xFF);

    int rndfd;
    rndfd = open("/dev/urandom", 0);
    read(rndfd, (unsigned char *)(buf + i), 12);
    close(rndfd);
    int n = sendto(p_a->socket_stream, (char *)buf, 20, 0, (struct sockaddr *)&servaddr, sizeof(servaddr)); 
    if (n < 0)
    {
        perror("Send failed");
        return 1;
    }
    n = recv(p_a->socket_stream, buf, sizeof(buf), 0);
    if (n < 0)
    {
        perror("Recv failed");
        return 1;
    }

    unsigned int crc_port = ((unsigned char)buf[27] ^ 0x12) | (((unsigned char)buf[26] ^ 0x21) << 8);
    if (strncmp(p_a->ip_browser, "10.168", strlen("10.168")) != 0)
    {
        sprintf(p_a->ip_server, "%d.%d.%d.%d", buf[28]^0x21, buf[29]^0x12, buf[30]^0xA4, buf[31]^0x42);
        sprintf(p_a->ice_server, "candidate:842163049 1 udp 1677729535 %s %d typ srflx raddr %s rport %d generation 0 ufrag sEMT network-cost 999", p_a->ip_server, crc_port, hostname, p_a->port_ice);
    }
    else
    { 
        sprintf(p_a->ice_server, "%s%s %d%s", ice_candidate_first, p_a->ip_server, p_a->port_ice, ice_candidate_second);
    }
    if(DEBUG)
        printf("%s\n", p_a->ice_server);
    return 0;
}
void iceParse(struct pthread_arguments* p_a)//candidate:2182926537 1 udp 2113937151 10.168.191.246 56429 typ host generation 0 ufrag 2uGL network-cost 999
{
    char* t1 = NULL;
    char* t2;
    char time_port[7] = {0};
    unsigned short i = 0;
    t1 = strstr(p_a->ice_browser, " ");
    t1 += 1;
    while(i < 3)
    {
        t1 = strstr(t1, " ");
        t1 += 1;
        i++;
    }
    t2 = strstr(t1, " ");
    strncpy(p_a->ip_browser, t1, t2 - t1);
    p_a->ip_camera[t2 -t1] = '\0';
    t1 = t2;
    t1++;
    t2 = strstr(t1, " ");
    strncpy(time_port, t1, t2 - t1);
    time_port[t2-t1] = '\0';
    p_a->port_ice_browser = atoi(time_port);
    t1 = strstr(t2, "ufrag");
    t1 += 6;
    t2 = strstr(t1, " ");
    strncpy(p_a->uflag_browser, t1, t2 - t1);
    p_a->uflag_browser[t2 - t1] = '\0';
    printf("Host: %s\nPort: %d\nUfrag: %s\n\n", p_a->ip_browser, p_a->port_ice_browser, p_a->uflag_browser);
}
void pwdParse(struct pthread_arguments* p_a)
{
    char* t1 = NULL;
    char* t2;
    int i = 0;
    memset(p_a->pwd_browser.s, 0, sizeof(p_a->pwd_browser.s));
    t1 = strstr(p_a->sdp_offer, "a=ice-pwd:");
    t1 += 10;

    t2 = strstr(t1, "\r\n");
    strncpy(p_a->pwd_browser.s, t1, t2-t1);
    p_a->pwd_browser.len = t2 - t1;
    printf("PWD: \n+++%s+++\n", p_a->pwd_browser.s);
}

void setSockaddr(struct sockaddr_in* in, unsigned char* ip, unsigned int port)
{
    memset(in, 0, sizeof(struct sockaddr_in));
    in->sin_family = AF_INET; // IPv4
    if (ip != NULL)
        inet_aton((const char* )ip, &in->sin_addr);   /// Address browser
    else
        in->sin_addr.s_addr = INADDR_ANY;
    in->sin_port = htons(port);
}

int createSockaddr(struct sockaddr_in* in, unsigned char* ip, unsigned int port, int* socket_in)
{   
    int fd = 0;
    setSockaddr(in, ip, port);
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        return 1;
    if (bind(fd, (const struct sockaddr *)in, sizeof(struct sockaddr_in)) < 0)
        return 1;
    *socket_in = fd;
    return 0;
}
void free_all(struct pthread_arguments* p_a)
{
    list[p_a->index] = false;
    if(p_a->socket_rtp_fd >= 0)
    {
        close(p_a->socket_rtp_fd);
        p_a->socket_rtp_fd = -1;
    }
    if(p_a->socket_stream >= 0)
    {
        close(p_a->socket_stream);
        p_a->socket_stream = -1;
    }
    if(p_a->camerafd >= 0)
    {
        close(p_a->camerafd);
        p_a->camerafd = -1;
    }
    dtls_connection_cleanup(&p_a->dtls_cert);
    dtls_fingerprint_free(p_a);
    free(p_a);
    printf("Free\n");
}
