#include "h264_camera.h"
#include "HMAX_CTX.h"
#include "../Base64/base64.h" 
#include <time.h>
void gen_random(char *s, const int len) 
{
    srand(time(0)); 
    static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}
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
int setup_to_camera(struct sockaddr_in& saddr, int& camerafd, char* host, unsigned int port_camera, char* session, unsigned int& port_udp)
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
    port_udp = atoi(p) + 1;
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
int teardown_to_camera(int sockfd, char* host, char* session)
{
	char teardown_text[512] = {0};
	sprintf(teardown_text, "TEARDOWN rtsp://%s/axis-media/media.amp RTSP/1.0\r\nCSeq: 3\r\nUser-Agent: RTSPClient\r\nSession: %s\r\n\r\n", host, session);
	printf("%s", teardown_text);
	char buf[512] = {0};
	if (send(sockfd, teardown_text, strlen(teardown_text), 0) < 0)
	{
		perror("Send teardown");
		return 1;
	}
	if (recv(sockfd, buf, sizeof(buf), 0) < 0)
	{
		perror("Read teardown");
		return 1;
	}	
	return 0;
}

void generationSDP_BROWSER(struct pthread_arguments* p_a, char* buf)
{
	char* t1 = NULL;
	char* t2 = NULL;
	char* t3 = NULL;
	char portic[10];
    printf("generationSDP_BROWSER Start\n");
	//strcpy(time, (char*)buf+3);
	/*
	t1 = strstr(time, "IP4");
	t1 += 4;
	strncpy(p_a->sdp_offer, time, t1-time);
	strcat(p_a->sdp_offer, p_a->ip_server);
	strcat(p_a->sdp_offer, "\r\n");
	t1 = strstr(time, "s=-");
	*/
	t2 = strstr(buf, "a=msid");
	//strncat(p_a->sdp_offer, t1, t2-t1);
	t3 = strstr(t2, "\n");
	t3 += 1;
	strncpy(p_a->sdp_offer, buf, t3-buf);
		
	strcat(p_a->sdp_offer, "m=video ");
	sprintf(portic, "%d ", p_a->port_ice_browser);
	strcat(p_a->sdp_offer, portic);
	t1 = strstr(t3, "UDP");
	t2 = strstr(t1, "\r\n");

	strncat(p_a->sdp_offer, t1, t2-t1+2);
	
	strcat(p_a->sdp_offer, "c=IN IP4 ");
	strcat(p_a->sdp_offer, p_a->ip_server);
	strcat(p_a->sdp_offer, "\r\n");
	t1 = strstr(t1, "a=rtcp");
	t2 = strstr(t1, "a=recvonly");
	t3 = strstr(t2, "\n");
	t3 += 1;
	strncat(p_a->sdp_offer, t1, t2-t1);
	strcat(p_a->sdp_offer, "a=recvonly\r\n");
	strcat(p_a->sdp_offer, t3);
    printf("generationSDP_BROWSER end\n");
}

//int sdpParse(char* des, char* flag,char* answer, char*ice)
//sdpParse(p_a->sdp_camera, p_a->uflag_server, p_a->sdp_answer, p_a->ice_server);
int sdpParse(struct pthread_arguments* p_a)
{
    char version[] = "v=0\r\no=-";
    char sdp_f[] = "a=rtpmap:102 H264/90000\r\n"; 
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
    printf("Session_version: %s\n", sess_version);

    sess_version[time-t] = '\0';
    t = strstr(p_a->sdp_camera, "a=fmtp");
    time = strstr(t, ":");
    strncpy(fmtp, t, time - t);
    strcat(fmtp, ":102 ");
    time += 4;
    t = strstr(time, "sprop");
    t -= 2;
    strncat(fmtp, time, t - time); 
    
    time = strstr(t, "sets=");
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
    
   /*
    t = strstr(p_a->sdp_camera, "o=");
    t = strstr(t, " ");
    t += 1;
    time = t;
    time = strstr(t, " ");
    strncpy(sess_version, t, time - t);
    t = strstr(p_a->sdp_camera, "a=fmtp");
    time = strstr(t, ":");
    strncpy(fmtp, t, time - t);
    strcat(fmtp, ":102 ");
    time += 4;
    t = strstr(time, "profile-level-id"); 
    strncat(fmtp, time, t - time); 
    strcat(fmtp, "profile-level-id=42001f; ");
    t = strstr(t, "sprop");
    time = strstr(t, "\r");
    strncat(fmtp, t, time - t); 
    */

    p_a->port_from_rtpengine = p_a->port_ice;
    sprintf(p_a->answer_to_engine, "%s %s 2 IN IP4 127.0.0.1\r\ns=Daniil Team\r\nc=IN IP4 127.0.0.1\r\nt=0 0\r\nm=video %d RTP/AVP 102\r\n%s%s\r\na=sendonly\r\n", version, sess_version,  p_a->port_ice, sdp_f, fmtp);
    printf("%s\n", p_a->answer_to_engine);
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
    strcpy((char*)p_a->ip_server, "10.168.168.165");
    //sprintf(p_a->ice_server, "%s%s %d%s", ice_candidate_first, IPbuffer, p_a->port_ice, ice_candidate_second);
    if(DEBUG)
        printf("%s\n", p_a->ice_server);
}
//iceParse(p_a->ice_browser, p_a->ip_browser, p_a->ip_server, p_a->port_ice_browser, p_a->uflag_browser);
//void iceParse(char* ice, char* ip_browser, char* ip_server, unsigned int& port, char* uflag_browser)
void iceParse(struct pthread_arguments* p_a)
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
    p_a->port_ice_browser = atoi(time_port);
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
int generationSTUN(char* ip_server, char* ip_browser, unsigned int ice_port_browser, unsigned int ice_port_server, char* name, char* pwd)
{
       	return 0;
}
void send_delete(struct pthread_arguments* p_a)
 {
    char send_delete[512] = {0};
    sprintf(send_delete, "./tcp_server/./rtpengine-ng-client delete --from-tag=%s --protocol=RTP/AVPF --call-id=%s --ICE=remove ", p_a->from_tag, p_a->call_id);
    system(send_delete);
    printf("Send delete\n");
 }
int sendSDP_rtpengine(struct pthread_arguments* p_a)
{	
	const char* rtpengine_path = "./tcp_server/./rtpengine-ng-client";
	const char* command_offer = " offer";
	const char* command__answer = " answer";
	
    gen_random(p_a->from_tag, 11);
    printf("%s\n", p_a->from_tag);
    gen_random(p_a->to_tag, 11);
    printf("%s\n", p_a->to_tag);
    gen_random(p_a->call_id, 11);
    printf("%s\n", p_a->call_id);
    char flags_offer[512] = {0};
    sprintf(flags_offer, " --trust-address --all --from-tag=%s --strict-source --protocol=RTP/AVPF --SDES=off --call-id=%s --ICE=remove --rtcp-mux=demux --replace-origin --replace-session-connection", p_a->from_tag, p_a->call_id);
	char flags_answer[512] = {0};
    sprintf(flags_answer, " --trust-address --all --from-tag=%s  --strict-source --protocol=RTP/SAVPF --SDES=off --to-tag=%s --rtcp-mux=offer --replace-origin --replace-session-connection --call-id=%s --ICE=force", p_a->from_tag, p_a->to_tag, p_a->call_id);
	const char* sdp = " --sdp=$\'";
	char all_command[4600]; 
	FILE* fd_offer = fopen("Offer_from_browser.txt", "w");
	FILE* fd_answer = fopen("Answer_from_browser.txt", "w");
    //sprintf(all_command, "%s%s%s%s%sa=%s\r\na=%s%s%s\r\n\'", rtpengine_path, command_offer, flags_offer, sdp, p_a->sdp_offer, p_a->ice_browser, relay_candidate_1, "10.168.168.165", relay_candidate_2);
	sprintf(all_command, "%s%s%s%s%sa=%s\r\n\'", rtpengine_path, command_offer, flags_offer, sdp, p_a->sdp_offer, p_a->ice_browser);
    fprintf(fd_offer, "%s", p_a->sdp_offer);
    fclose(fd_offer);
	//printf("\n%s\n", all_command);
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
        return 1;
    }
	printf("Sdp browser: %s\n", buf);

	char* tt1 =  strstr(buf, "m=video");
	tt1 += 7;
	char* tt2 = strstr(tt1, "RTP");
	tt2 -= 1;
	char port_to_rtp[10] = {0};
	strncpy(port_to_rtp, tt1, tt2-tt1+1);
	p_a->port_to_rtpengine = atoi(port_to_rtp);
    close(fd);
	remove(NAMEDPIPE_NAME);
	
	
	//#############################################	
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
        return 1;
    }
    if (strstr(buf, "video") == NULL)
    {
        perror("Error sdp");
       	close(fd);
        return 1;
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
	printf("SDP Rtpengine: \n%s", p_a->sdp_answer);
    fprintf(fd_answer, "%s", p_a->sdp_answer);
    fclose(fd_answer);
	printf("ICE Rtpengine: \n%s\n", p_a->ice_server);
	//printf("Size: %d\nSdp: %c\n", strlen(p_a->sdp_answer), p_a->sdp_answer[strlen(p_a->sdp_answer)]);
	//#############################################
	//./rtpengine-ng-client offer --trust-address --from-tag=sgadhdagf --call-id=sfghjfsh --sdp=$'v=0\no=moo 1 2 IN IP4 192.168.1.90\ns=-\nc=IN IP4 192.168.1.90\nt=0 0\nm=audio 3456 RTP/AVP 0\n'
	//system(all_command);
	//system("./tcp_server/./test.pl");
	//sleep(10);
	return 0;
}
unsigned int rtp_parse(char* rtp, unsigned char* rtp_sps, unsigned int* sequnce, unsigned int* sequnce_origin, struct pthread_arguments* p_a)
{
	unsigned char v = (rtp[0] & 0xC0) >> 6;
    unsigned char p = (rtp[0] & 0x20) >> 5;
	unsigned char x = (rtp[0] & 0x10) >> 4;
	unsigned char cc = (rtp[0] & 0x0F);
	unsigned char m = (rtp[1] & 0x80) >> 7;
	unsigned char payload_type = rtp[1] & 0x7F;
    unsigned int seq_num = (rtp[2] & 0xFF) << 8 | (rtp[3] & 0xFF); // 1054i // 1055 // 1056 // 1057i // 1058
    unsigned long timestamp = (rtp[4] & 0xFF) << 24 | (rtp[5] & 0xFF) << 16 | (rtp[6] & 0xFF) << 8 | (rtp[7] & 0xFF); 
	unsigned long ssrc = (rtp[8] & 0xFF) << 24 | (rtp[9] & 0xFF) << 16 | (rtp[10]& 0xFF) << 8 | (rtp[11] & 0xFF);
    unsigned short indicator_type = rtp[12] & 0x1F;
    unsigned short header_type = rtp[13] & 0x1F;
    unsigned short header_start = (rtp[13] & 0x80) >> 7;
    unsigned int size_sps_packet = 0;
    rtp[1] = (rtp[1] & 0x80) | 102;
    payload_type = rtp[1] & 0x7F;
    m = (rtp[1] & 0x80) >> 7;
    if((*sequnce_origin) == 0)
    {
        (*sequnce_origin) = seq_num;
    }
    (*sequnce) = (*sequnce) + seq_num - (*sequnce_origin); // sequnce 1 + 1054 - 1054 = 1 // sequnce = 2 + 1055 - 1054 = 3 // sequnce = 3 + 1056 - 1055 = 4 //sequnce = 4 + 1057 - 1056 = 5
    *sequnce = *sequnce % 0x01FFFF;
    ////////////////////////////////////////////////////////////////////
    if(indicator_type == 28 && header_type == 5 && header_start == 1) 
    {   
        size_sps_packet = rtp_sps_parse(rtp, rtp_sps, *sequnce, p_a); // sequnce = 1 // sequnce = 5
        (*sequnce)++; // sequence = 2 // sequnce = 6
    }
    rtp[2] = ((*sequnce) & 0xFF00) >> 8; // sequnce = 2 //sequnce = 3 //sequnce = 4 //sequnce = 6 // sequnce = 7
    rtp[3] = (*sequnce) & 0xFF; 
    (*sequnce_origin) = seq_num; // sequnce_origin = 1054 //sequnce_origin = 1055 //sequnce_origin = 1056 
    if (DEBUG)
    {
	    /* 0 byte */
        printf("V = %u\n", v);
	    printf("P = %u\n", p);
	    printf("X = %u\n", x);
	    printf("CC = %u\n", cc);
        /* 1 byte */
	    printf("M = %u\n", m);
	    printf("Payload tupe = %u\n", payload_type);
	    /* 2-3 bytes */
        printf("Sequence number = %u\n", *sequnce);
	    /* 4-7 bytes */
        printf("Timestamp = %lu\n", timestamp);
        /* 8-11 bytes */
        printf("SSRC = %lu\n", ssrc);
        /* 12-13 bytes */
        if(indicator_type == 28 && header_type == 5 && header_start == 1)
        {
            printf("Type payload is FU\n");
            printf("Type frame is IDR\n");
            printf("This frame is start I-frame\n");
        }
    }
    return size_sps_packet;
}
unsigned int rtp_sps_parse(char* rtp, unsigned char* sps, unsigned int sequnce, struct pthread_arguments* p_a)
{
    //unsigned int seq_num = (rtp[2] & 0xFF) << 8 | (rtp[3] & 0xFF);
    unsigned int seq_num = sequnce;
    unsigned short index = 0;
    unsigned char h = rtp[2];
    unsigned char l = rtp[3];
    for (int i = 0; i < 2; i++)
    {
        sps[index++] = rtp[i];
    }
    sps[index++] = seq_num >> 8;
    sps[index++] = seq_num; 
    for (int i = 0; i < 8; i++)
    {
        sps[index++] = rtp[i+4];     
    }
    sps[index++] = 56; // 0x38
    sps[index++] = (p_a->size_sps) & 0xFF >> 8;
    sps[index++] = (p_a->size_sps) & 0xFF;
    //sps[index++] = 39; // 0x27 Nal header byte with sps payload
    for (int i = 0; i < p_a->size_sps; i++)
    {
        sps[index++] = p_a->sps[i];
    } 
    sps[index++] = (p_a->size_pps) & 0xFF >> 8;
    sps[index++] = (p_a->size_pps) & 0xFF;
    //sps[index++] = 40; // 0x28 Nal header byte with pps payload
    for (int i = 0; i < p_a->size_pps; i++)
    {
        sps[index++] = p_a->pps[i];
    }
    return index;
 }
 
