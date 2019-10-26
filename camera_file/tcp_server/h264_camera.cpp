#include "h264_camera.h"
#include "HMAX_CTX.h"
void setUSERNAME(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index, char* name)
{
    struct iovec* i = &iov[last];
    i->iov_base = attr_tlv;
    attr_tlv->type = htons(USERNAME);
    attr_tlv->len = htons(strlen(name));
    printf("Type: %x\n", attr_tlv->type);
    printf("Length: %x\n", attr_tlv->len);
    i->iov_len = sizeof(*attr_tlv) + 3 + strlen(name);
    int t = 1;
    while(t >= 0)
    {
	    d_r[index] = USERNAME >> (8 * t);
	    index++;
	    t--;
    }
    t = 1;
    while(t >= 0)
    {
	    d_r[index] = strlen(name) >> (8 * t);
	    index++;
	    t--;
    }
    strcat((char*)(d_r + index), name);
    index += strlen(name);
    t = 0;
    while(t < 3)
    {
	    d_r[index] = 0x0;
	    index++;
	    t++; 
    }
    last += 1;
}
void setICE_CONTROLLING(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index)
{
    int rndfd;
    struct iovec* i = &iov[last];
    i->iov_base = attr_tlv;
    attr_tlv->type = htons(ICE_CONTROLLING);
    attr_tlv->len = htons(ICE_CONTROLLING_LENGTH);

    printf("Type: %x\n", attr_tlv->type);
    printf("Length: %x\n", attr_tlv->len);

    i->iov_len = sizeof(*attr_tlv) + ICE_CONTROLLING_LENGTH;
    //printf("Iov_len: %d\n", i->iov_len);
    int t = 1;
    while(t >= 0)
    {
	    d_r[index] = ICE_CONTROLLING >> (8 * t);
	    index++;
	    t--;
    }
    t = 1;
    while(t >= 0)
    {
	    d_r[index] = ICE_CONTROLLING_LENGTH >> (8 * t);
	    index++;
	    t--;
    }
    rndfd=open("/dev/urandom", 0);
    read(rndfd, (unsigned char*)(d_r+index), ICE_CONTROLLING_LENGTH);
    index += ICE_CONTROLLING_LENGTH;
    last++;
    close(rndfd);
}
void setPRIORITY(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index)
{
    struct iovec* i = &iov[last];
    i->iov_base = attr_tlv;
    attr_tlv->type = htons(PRIORITY);
    attr_tlv->len = htons(PRIORITY_LENGTH);

    printf("Type: %x\n", attr_tlv->type);
    printf("Length: %x\n", attr_tlv->len);

    i->iov_len = sizeof(*attr_tlv) + PRIORITY_LENGTH;
    int t = 1;
    while(t >= 0)
    {
	    d_r[index] = PRIORITY >> (8 * t);
	    index++;
	    t--;
    }
    t = 1;
    while(t >= 0)
    {
	    d_r[index] = PRIORITY_LENGTH >> (8 * t);
	    index++;
	    t--;
    }
    t = 3;
    while(t >= 0)
    {
	    printf("index = %d\n", index);
	    d_r[index] = PROPRITY_VALUE >> (8 * t);
	    index++;
	    t--;
    }
    last++;
}
void setUSE_CANDIDATE(struct tlv* attr_tlv, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index)
{
    struct iovec* i = &iov[last];
    i->iov_base = attr_tlv;
    attr_tlv->type = htons(USE_CANDIDATE);
    attr_tlv->len = htons(USE_CANDIDATE_LENGTH);

    printf("Type: %x\n", attr_tlv->type);
    printf("Length: %x\n", attr_tlv->len);
    i->iov_len = sizeof(*attr_tlv) + USE_CANDIDATE_LENGTH;
    int t = 1;
    while(t >= 0)
    {
	    d_r[index] = USE_CANDIDATE >> (8 * t);
	    index++;
	    t--;
    }
    t = 1;
    while(t >= 0)
    {
	    d_r[index] = 0x0;
	    index++;
	    t--;
    }
    last++;
}


void integrity(struct message_integrity* mi, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index, char* pwd)
{
    struct iovec* i = &iov[last];
    struct tlv* attr_tlv = &(mi->t);
    i->iov_base = attr_tlv;
    attr_tlv->type = htons(MESSAGE_INTEGRITY);
    attr_tlv->len = htons(MESSAGE_INTEGRITY_LENGTH);

    printf("Type: %x\n", attr_tlv->type);
    printf("Length: %x\n", attr_tlv->len);

    i->iov_len = sizeof(*attr_tlv) + MESSAGE_INTEGRITY_LENGTH;
    memset(mi->digest, 0, sizeof(mi->digest));
    Integrity(iov, last, pwd, mi->digest);
    printf("Digest: \n%s\n", mi->digest);
    int t = 1;
    while(t >= 0)
    {
	    d_r[index] = MESSAGE_INTEGRITY >> (8 * t);
	    index++;
	    t--;
    }
    t = 1;
    while(t >= 0)
    {
	    d_r[index] = MESSAGE_INTEGRITY_LENGTH >> (8*t);
	    index++;
	    t--;
    }
    strcpy((char*)d_r + index, mi->digest);
    index += MESSAGE_INTEGRITY_LENGTH;
    last++;

}
void Fingerprint(struct fingerprint* f, struct iovec* iov, char* d_r, unsigned int& last, unsigned int& index)
{
    struct iovec* i = &iov[last];
    struct tlv* attr_tlv = &(f->t);
    i->iov_base = attr_tlv;
    attr_tlv->type = htons(FINGERPRINT);
    attr_tlv->len = htons(FINGERPRINT_LENGTH);

    printf("Type: %x\n", attr_tlv->type);
    printf("Length: %x\n", attr_tlv->len);

    i->iov_len = sizeof(*attr_tlv) + FINGERPRINT_LENGTH;

    f->crc = crc32(0, NULL, 0);
    printf("Test1\n");
    for (int i = 0; i < last; i++)
	f->crc = crc32(f->crc, (unsigned char*)iov[i].iov_base, iov[i].iov_len);
    printf("test4\n");
    f->crc = htonl(f->crc ^ STUN_CRC_XOR);
    int t = 1;
    while(t >= 0)
    {
	    d_r[index] = FINGERPRINT >> (8 * t);
	    index++;
	    t--;
    }
    t = 1;
    while(t >= 0)
    {
	    d_r[index] = FINGERPRINT_LENGTH >> (8*t);
	    index++;
	    t--;
    }
    t = 3;
    while(t >= 0)
    {
	 d_r[index] = f->crc >> (8*t);
	 index++;
	 t--;
    }
    last++;
}



void setHeader(struct header* h, struct iovec* iov, char* d_r, unsigned int& index)
{
    int rndfd;
    struct iovec* i = &(iov[0]);
    i->iov_base = h;
    h->msg_type = htons(HEADER_TYPE);
    h->magick = htonl(MAGICK);
    i->iov_len = sizeof(*h);
    //printf("Header Iov_len: %lu\n", i->iov_len);
    int t = 1;
    while(t >= 0)
    {
	    d_r[index] = HEADER_TYPE >> (8 * t);
	    index++;
	    t--;
    }
    index += 2; //len
 
    t = 3;
    while(t >= 0)
    {
	    d_r[index] = MAGICK >> (8 * t);
	    index++;
	    t--;
    }

    rndfd=open("/dev/urandom", 0);
    read(rndfd, (unsigned char*)(d_r+index), 12);
    memcpy((char* )h->id, d_r+index, 12);
    index += 12;
    close(rndfd);
}
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
void generationSDP_BROWSER(struct pthread_arguments* p_a, unsigned char* buf)
{
	char time[4600] = {0};
	char* t1 = NULL;
	char* t2 = NULL;
	char* t3 = NULL;
	
	strcpy(time, (char*)buf+3);
	t1 = strstr(time, "IP4");
	t1 += 4;
	strncpy(p_a->sdp_offer, time, t1-time);
	strcat(p_a->sdp_offer, p_a->ip_server);
	strcat(p_a->sdp_offer, "\r\n");
	t1 = strstr(time, "s=-");
	t2 = strstr(time, "a=msid");
	strncat(p_a->sdp_offer, t1, t2-t1);
	t3 = strstr(t2, "\n");
	t3 += 1;
	strncat(p_a->sdp_offer, t2, t3-t2);
		
	strcat(p_a->sdp_offer, "m=video 60840 ");
	t1 = strstr(t1, "UDP");
	t2 = strstr(t1, "\r\n");
	strncat(p_a->sdp_offer, t1, t2-t1+2);
	strcat(p_a->sdp_offer, "c=IN IP4 ");
	strcat(p_a->sdp_offer, p_a->ip_server);
	strcat(p_a->sdp_offer, "\r\n");
	t1 = strstr(t1, "a=rtcp");
	strcat(p_a->sdp_offer, t1);	
}

//int sdpParse(char* des, char* flag,char* answer, char*ice)
//sdpParse(p_a->sdp_camera, p_a->uflag_server, p_a->sdp_answer, p_a->ice_server);
int sdpParse(struct pthread_arguments* p_a)

{
    char version[] = "v=0\r\n"
    "o=- ";
    char sdp_f[] = "a=rtpmap:96 H264/90000\r\n"
    "a=fmtp:96 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f\r\n"
    "a=rtcp-fb:96 transport-cc\r\n"
    "a=rtcp-fb:96 ccm fir\r\n"
    "a=rtcp-mux\r\n"
    "a=sendonly\r\n";
    
    
    strncpy(p_a->uflag_server, "sEMT",sizeof("sEMT"));
    memset(p_a->pwd_server, 0, sizeof(p_a->pwd_server));
    strcpy(p_a->pwd_server, "hvOt0NM+iKHs4rFN41uK2h/h");
    char fmtp[60] = {0};
    char sess_version[20] = {0};
    char* t;
    char* time;
    char* payload_type;
    
    t = strstr(p_a->sdp_camera, "o=");
    t = strstr(t, " ");
    t += 1;
    time = t;
    time = strstr(t, " ");
    strncpy(sess_version, t, time - t);
    t = strstr(p_a->sdp_camera, "a=fmtp");
    /*
     payload_type = t + 7;
     strncpy(fmtp, t, payload_type - t);
     strcat(fmtp, "120 ");
     t = strstr(des, "packetization");
     */
    time = strstr(p_a->sdp_camera, "sprop-parameter-sets");
    strncat(fmtp, t, time - t - 2);
    sprintf(p_a->answer_to_engine, "%s%s 2 IN IP4 %s\r\ns=Daniil Team\r\nc=IN IP4 %s\r\nt=0 0\r\nm=video %d RTP/AVP 96\r\n%s", version, sess_version, p_a->ip_server, p_a->ip_server, p_a->port_ice, sdp_f);
    if(DEBUG)
        printf("%s\n", p_a->answer_to_engine);
    return 0;
}
//void create_ice(char* ice_server, unsigned int port_ice, char* ip_server)
//create_ice(p_a->ice_server, p_a->port_ice, p_a->ip_server);
void create_ice(struct pthread_arguments* p_a)
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
    memset((char*)p_a->ip_server, 0, sizeof(p_a->ip_server));
    strcpy((char*)p_a->ip_server, IPbuffer);
    //sprintf(p_a->ice_server, "%s%s %d%s", ice_candidate_first, IPbuffer, p_a->port_ice, ice_candidate_second);
    if(DEBUG)
        printf("%s\n", p_a->ice_server);
}
//iceParse(p_a->ice_browser, p_a->ip_browser, p_a->ip_server, p_a->port_ice_browser, p_a->uflag_browser);
//void iceParse(char* ice, char* ip_browser, char* ip_server, unsigned int& port, char* uflag_browser)
void iceParse(struct pthread_arguments* p_a, unsigned int& port)
{
    char* t1 = NULL;
    char* t2;
    char time_port[7] = {0};
    int i = 0;
    t1 = strstr(p_a->ice_browser, " ");
    t1 += 1;
    while(i < 3)
    {
        t1 = strstr(t1, " ");
        t1 += 1;
        i++;
    }
    t2 = strstr(p_a->ice_browser, "local");
    if(t2 != NULL)
    {
        strcpy(p_a->ip_browser, p_a->ip_server);
        t2 = strstr(t1, " ");
    }
    else
    {
        t2 = strstr(t1, " ");
        strncpy(p_a->ip_browser, t1, t2 - t1);
        p_a->ip_browser[t2 - t1] = '\0';
    }
    t1 += t2 - t1 + 1;
    t2 = strstr(t1, " ");
    strncpy(time_port, t1, t2 - t1);
    port = atoi(time_port);
    t1 = strstr(p_a->ice_browser, "ufrag");
    t1 += 6;
    t2 = strstr(t1, " ");
    strncpy(p_a->uflag_browser, t1, t2 - t1);
    p_a->uflag_browser[t2 - t1] = '\0';
}
void pwdParse(struct pthread_arguments* p_a)
{
    //char _browser[20]; ///PWD from browser in sdp description
    //char _server[30]; ///PWD from server

    char* t1 = NULL;
    char* t2;
    int i = 0;
    memset(p_a->pwd_browser, 0, sizeof(p_a->pwd_browser));
    t1 = strstr(p_a->sdp_offer, "a=ice-pwd:");
    t1 += 10;

    t2 = strstr(t1, "\r\n");
    strncpy(p_a->pwd_browser, t1, t2-t1);
    printf("PWD: \n%s\n", p_a->pwd_browser);

}
//int generationSTUN(char* ip_server, char* ip_browser, unsigned int ice_port_browser, unsigned int ice_port_server, char* name)
//{
//    int n = 0;
//    int rndfd;
//    int sockfd;
//    unsigned int last_iov = 0;
//    unsigned int next_step = 0;
//    unsigned int size_all_stun = 0;
//    struct sockaddr_in stun_addr;
//    struct sockaddr_in stun_browser_addr;
//    unsigned char data_req[276];
//
//    struct iovecs iov[10] = {0};
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
//    //
//
//    data_req[next_step] = iov[hs].tlv[0] = 0x00;
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[1] = 0x01;
//    next_step+=1;
//    // first byte for length
//    next_step+=1;
//    // second byte for length
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[4] = 0x2112A442 >> 24;
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[5] = 0x2112A442 >> 16;
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[6] = 0x2112A442 >> 8;
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[7] = 0x2112A442;
//    next_step+=1;
//    // Enter Id for header
//    rndfd=open("/dev/urandom", 0);
//    read(rndfd, (unsigned char*)(data_req+next_step), 12);
//    read(rndfd, (unsigned char*)(iov[hs].tlv + next_step]), 12);
//    close(rndfd);
//    next_step += 12;
//    iov[hs].len = 20;
//    hs += 1;
//    // USERNAME
//    printf("USERNAME\n");
//    data_req[next_step] = iov[hs].tlv[0] = USERNAME >> 8;
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[1] = USERNAME;
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[2] = strlen(name) >> 8;
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[3] = strlen(name);
//    next_step+=1;
//    strcat((char*)(data_req + next_step), name);
//    next_step += strlen(name);
//    printf("Value = %s\n", name);
//    data_req[next_step] = 0x0;
//    next_step+=1;
//    data_req[next_step] = 0x0;
//    next_step+=1;
//    data_req[next_step] = 0x0;
//    next_step+=1;
//    size_all_stun = STUN_HEADER_ATTR + 3 + strlen(name);
//    iov[hs].len = STUN_HEADER_ATTR + 3 + strlen(name);
//    hs += 1;
//    // ICE-CONTROLLING
//    printf("ICE-CONTROLLING\n");
//    data_req[next_step] = iov[hs].tlv[0] = ICE_CONTROLLING >> 8;
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[1] = ICE_CONTROLLING;
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[2] = ICE_CONTROLLING_LENGTH >> 8;
//    next_step+=1;
//    data_req[next_step] = iov[hs].tlv[3] = ICE_CONTROLLING_LENGTH;
//    next_step+=1;
//    rndfd=open("/dev/urandom", 0);
//    read(rndfd, (unsigned char*)(data_req+next_step), ICE_CONTROLLING_LENGTH);
//    close(rndfd);
//    next_step+=ICE_CONTROLLING_LENGTH;
//    size_all_stun += (STUN_HEADER_ATTR + ICE_CONTROLLING_LENGTH);
//    iov[hs].len = STUN_HEADER_ATTR + ICE_CONTROLLING_LENGTH;
//    hs += 1;
//    // PRIORITY
//    printf("PRIORITY\n");
//    data_req[next_step] = PRIORITY >> 8;
//    next_step+=1;
//    data_req[next_step] = PRIORITY;
//    next_step+=1;
//    data_req[next_step] = PRIORITY_LENGTH >> 8;
//    next_step+=1;
//    data_req[next_step] = PRIORITY_LENGTH;
//    next_step+=1;
//    data_req[next_step] = PROPRITY_VALUE >> 24;
//    next_step+=1;
//    data_req[next_step] = PROPRITY_VALUE >> 16;
//    next_step+=1;
//    data_req[next_step] = PROPRITY_VALUE >> 8;
//    next_step+=1;
//    data_req[next_step] = PROPRITY_VALUE;
//    next_step+=1;
//    size_all_stun += STUN_HEADER_ATTR + PRIORITY_LENGTH;
//    printf("Value = %d\n", PROPRITY_VALUE);
//    // USE_CANDIDATE
//    printf("USE-CANDIDATE\n");
//    data_req[next_step] = USE_CANDIDATE >> 8;
//    next_step+=1;
//    data_req[next_step] = USE_CANDIDATE;
//    next_step+=1;
//    data_req[next_step] = USE_CANDIDATE_LENGTH >> 8;
//    next_step+=1;
//    data_req[next_step] = USE_CANDIDATE_LENGTH;
//    next_step+=1;
//    size_all_stun += STUN_HEADER_ATTR + USE_CANDIDATE_LENGTH;
//    // MESSAGE-INTEGRITY
//    printf("MESSAGE-INTEGRITY\n");
//    /*
//    data_req[next_step] = MESSAGE_INTEGRITY >> 8;
//    next_step+=1;
//    data_req[next_step] = MESSAGE_INTEGRITY;
//    next_step+=1;
//    data_req[next_step] = MESSAGE_INTEGRITY_LENGTH >> 8;
//    next_step+=1;
//    data_req[next_step] = MESSAGE_INTEGRITY_LENGTH;
//    next_step+=1;
//
//
//    n = 3;
//
//    while(n >= 0)
//    {
//        data_req[next_step] = MESSAGE_INTEGRITY_VALUE_1 >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//    n = 3;
//    while(n >= 0)
//    {
//        data_req[next_step] = MESSAGE_INTEGRITY_VALUE_2 >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//    n = 3;
//    while(n >= 0)
//    {
//        data_req[next_step] = MESSAGE_INTEGRITY_VALUE_3 >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//    n = 3;
//    while(n >= 0)
//    {
//        data_req[next_step] = MESSAGE_INTEGRITY_VALUE_4 >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//    n = 3;
//    while(n >= 0)
//    {
//        data_req[next_step] = MESSAGE_INTEGRITY_VALUE_5 >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//
//
//    size_all_stun += STUN_HEADER_ATTR + MESSAGE_INTEGRITY_LENGTH;
//    uint32_t crc = Crc32((unsigned char*)data_req, size_all_stun);
//    printf("Crc32 result = %x\n", crc);
//
//
//    //FINGERPRINT
//    printf("FINGERPRINT\n");
//    data_req[next_step] = crc >> 8;
//    next_step+=1;
//    data_req[next_step] = crc;
//    next_step+=1;
//    data_req[next_step] = FINGERPRINT_LENGTH >> 8;
//    next_step+=1;
//    data_req[next_step] = FINGERPRINT_LENGTH;
//    next_step+=1;
//    crc = crc ^ 0x5354554E;
//    n = 3;
//    while(n >= 0)
//    {
//        data_req[next_step] = crc >> (n*8) ;
//        next_step+=1;
//        n--;
//    }
//    
//
//    data_req[2] = size_all_stun >> 8;
//    data_req[3] = size_all_stun;
//    n = sendto(sockfd, data_req, (STUN_HEADER + size_all_stun), 0, (struct sockaddr*)&stun_browser_addr, sizeof(stun_browser_addr));
//    if(n < STUN_HEADER + size_all_stun)
//    {
//        printf("Error with sendto\n");
//        printf("Sendto n = %d\n", n);
//        return 1;
//    }
//    printf("Sendto n = %d\n", n);
//    return 0;
//}
int generationSTUN(char* ip_server, char* ip_browser, unsigned int ice_port_browser, unsigned int ice_port_server, char* name, char* pwd)
{
    /*int n = 0;
    int rndfd;
    int sockfd;
    unsigned int last_iov = 1;
    unsigned int next_step = 0;
    unsigned int size_all_stun = 0;
    struct sockaddr_in stun_addr;
    struct sockaddr_in stun_browser_addr;
    unsigned char data_req[276];
    struct iovec iov[10];
    struct header hd;
    struct username u_name;
    struct ice_controlling ice_c;
    struct priority p;
    struct use_candidate use_c;
    struct message_integrity mi;
    struct fingerprint f;
    // Socket server
    sockfd = socket(AF_INET, SOCK_DGRAM, 0); /// UDP
    bzero(&stun_addr, sizeof(stun_addr));
    stun_addr.sin_family = AF_INET;
    stun_addr.sin_port = htons(ice_port_server);
    inet_pton(AF_INET, ip_server, &stun_addr.sin_addr);
    n = bind(sockfd,(struct sockaddr *)&stun_addr,sizeof(stun_addr)); /// Was Opened port for stun request
    if(n != 0)
    {
        printf("Error with bind in Stun\n");
        return 1;
    }
    printf("Bind OK\n");
    printf("PWD_D: %s\n", pwd);
    // Socket for send message
    bzero(&stun_browser_addr, sizeof(stun_browser_addr));
    stun_browser_addr.sin_family = AF_INET;
    stun_browser_addr.sin_port = htons(ice_port_browser) ;
    inet_pton(AF_INET, ip_browser, &stun_browser_addr.sin_addr);
    //###########################################################################################
    setHeader(&hd, iov, (char*)data_req, next_step); //HEADER
    //###########################################################################################
    printf("USERNAME\n");
    setUSERNAME(&(u_name.t), iov, (char*)data_req, last_iov, next_step, name);//USERNAME
    printf("Value = %s\n", name);
    size_all_stun = STUN_HEADER_ATTR + 3 + strlen(name);
    printf("SIZE: %d\n", size_all_stun);
    //###########################################################################################
    printf("ICE-CONTROLLING\n");
    setICE_CONTROLLING(&(ice_c.t), iov, (char*)data_req, last_iov, next_step); //ICE-CONTROLLING
    size_all_stun += STUN_HEADER_ATTR + ICE_CONTROLLING_LENGTH;
    printf("SIZE: %d\n", size_all_stun);
    //##########################################################################################
    printf("PRIORITY\n");
    setPRIORITY(&(p.t), iov, (char*)data_req, last_iov, next_step); //PRIORITY
    size_all_stun += STUN_HEADER_ATTR + PRIORITY_LENGTH;
    printf("SIZE: %d\n", size_all_stun);
    //##########################################################################################
    printf("USE-CANDIDATE\n");
    setUSE_CANDIDATE(&(use_c.t), iov, (char*)data_req, last_iov, next_step); //USE-CANDIDATE
    size_all_stun += STUN_HEADER_ATTR + USE_CANDIDATE_LENGTH;
    printf("SIZE: %d\n", size_all_stun);
    //##########################################################################################
    printf("MESSAGE-INTEGRITY\n");
    size_all_stun += STUN_HEADER_ATTR + MESSAGE_INTEGRITY_LENGTH;
    hd.data_len = size_all_stun; 
    hd.data_len = htons(hd.data_len);
    integrity(&mi, iov, (char* )data_req, last_iov, next_step, pwd); //MESSAGE_INTEGRITY
    hd.data_len = htons(hd.data_len);
    printf("SIZE: %d\n", size_all_stun);
    //##########################################################################################
    printf("FINGERPRINT\n");
    size_all_stun += STUN_HEADER_ATTR + FINGERPRINT_LENGTH;
    hd.data_len += STUN_HEADER_ATTR + FINGERPRINT_LENGTH;
    hd.data_len = htons(hd.data_len);
    printf("Fing: %d\n", hd.data_len);
    Fingerprint(&f, iov, (char*)data_req, last_iov, next_step); //FINGERPRINT
    printf("SIZE: %d\n", size_all_stun);
    hd.data_len = htons(hd.data_len);
    //##########################################################################################
    

    data_req[2] = size_all_stun >> 8;
    data_req[3] = size_all_stun;
    hd.data_len = htons(size_all_stun);
    n = sendto(sockfd, data_req, (STUN_HEADER + size_all_stun), 0, (struct sockaddr*)&stun_browser_addr, sizeof(stun_browser_addr));
    if(n < STUN_HEADER + size_all_stun)
    {
        printf("Error with sendto\n");
        printf("Sendto n = %d\n", n);
        return 1;
    }
    printf("Sendto n = %d\n", n);*/
    int pid = fork();
        if(pid == 0)
	{	
            // Child
	    printf("In child process:\n");
	    char port_browser[10] = {0};
	    char port_server[10] = {0};
            sprintf(port_browser, "%d", ice_port_browser);
	    sprintf(port_server, "%d", ice_port_server);
            execl("./tcp_server/./perl_script.pl", "perl_script.pl", ip_server, port_server, ip_browser, port_browser, pwd, name, NULL);
	    return 0;
	}
	else
	{
            // Parent
            int status;
	    waitpid(pid, &status, 0);
	}
    	return 0;
}
int sendSDP_rtpengine(struct pthread_arguments* p_a)
{	
	char* rtpengine_path = "./tcp_server/./rtpengine-ng-client";
	char* command_offer = " offer";
	char* command__answer = " answer";
	char* flags_offer = " --trust-address --all --strict-source --from-tag=sgadhdage --protocol=RTP/AVPF --call-id=sfghjfsh --ICE=remove --rtcp-mux=demuxe --replace-origin --replace-session-connection --SDES=off";
	char* flags_answer = " --trust-address --all --strict-source --from-tag=sgadhdagm --protocol=RTP/SAVPF --to-tag=sgadhdagk --rtcp-mux=offer --replace-origin --replace-session-connection --SDES=off --call-id=sfghjfsh --ICE=force";
	char* sdp = " --sdp=$'";
	char all_command[4600]; 
	struct sockaddr_in to_rtp_addr;
	//##############################################
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0); /// UDP
	bzero(&to_rtp_addr, sizeof(to_rtp_addr));
	to_rtp_addr.sin_family = AF_INET;
	to_rtp_addr.sin_port = htons(60840);
	to_rtp_addr.sin_addr.s_addr = INADDR_ANY;
	bind(sockfd,(struct sockaddr *)&to_rtp_addr, sizeof(to_rtp_addr)); /// Was Opened port for stun request

	sprintf(all_command, "%s%s%s%s%sa=%s\r\na=%s%s%s\r\n\'", rtpengine_path, command_offer, flags_offer, sdp, p_a->sdp_offer, p_a->ice_browser, relay_candidate_1, p_a->ip_server, relay_candidate_2);
	int fd, len;
   	char buf[BUFSIZE];
	memset(buf, 0, sizeof(buf));
	system(all_command);
	
	//##############################################
	
	
	//##############################################
    	if ((fd = open(NAMEDPIPE_NAME, O_RDONLY)) <= 0 ) {
        	perror("open");
		return 1;
    	}
        memset(buf, 0, BUFSIZE);
       	if((len = read(fd, buf, BUFSIZE-1)) <= 0 ) {
        	perror("read");
           	close(fd);
        }
	printf("Sdp browser: %s\n", buf);
    	close(fd);
	remove(NAMEDPIPE_NAME);
	
	
	//#############################################
	sleep(3);	
	memset(all_command, 0, sizeof(all_command));
	sprintf(all_command, "%s%s%s%s%s\'", rtpengine_path, command__answer, flags_answer, sdp, p_a->answer_to_engine);
	system(all_command);
	//############################################
	if ((fd = open(NAMEDPIPE_NAME, O_RDONLY)) <= 0 ) {
        	perror("open");
		return 1;
    	}
        memset(buf, 0, BUFSIZE);
       	if((len = read(fd, buf, BUFSIZE-1)) <= 0 ) {
        	perror("read");
           	close(fd);
        }
    	close(fd);
	remove(NAMEDPIPE_NAME);
	
	char* t1 = strstr(buf, "a=candidate");
       	t1 += 2;
	char* t2 = strstr(t1, "\r");
	if (t1 != NULL && t2 != NULL)
	{
		strncpy(p_a->ice_server, t1, t2-t1);
		p_a->ice_server[t2-t1] = '\0';
	}
	t1 -= 2;
	strncpy(p_a->sdp_answer, buf, t1-buf);

	//strcat(p_a->sdp_answer, "\0");
	printf("SDP Rtpengine:i \n%s", p_a->sdp_answer);
	printf("ICE Rtpengine: \n%s\n", p_a->ice_server);
	//printf("Size: %d\nSdp: %c\n", strlen(p_a->sdp_answer), p_a->sdp_answer[strlen(p_a->sdp_answer)]);
	//#############################################
	//./rtpengine-ng-client offer --trust-address --from-tag=sgadhdagf --call-id=sfghjfsh --sdp=$'v=0\no=moo 1 2 IN IP4 192.168.1.90\ns=-\nc=IN IP4 192.168.1.90\nt=0 0\nm=audio 3456 RTP/AVP 0\n'
	//system(all_command);
	//system("./tcp_server/./test.pl");
	//sleep(10);
	close(sockfd);
	close(p_a->camerafd);
	

}	
