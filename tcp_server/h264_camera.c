#include <time.h>
#include <openssl/md5.h>
#include "h264_camera.h"

void init_ports(unsigned int* port_ice, unsigned int* port_camera, int index)
{
    *port_ice = port_ice_start + index; 
    *port_camera = port_camera_start + index*2;
}

int connect_camera(struct pthread_arguments* p_a)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
        return 1;
    struct sockaddr_in sddr; /// Struct for create tcp socket for requests camera
    memset(&sddr, 0, sizeof(sddr));

    sddr.sin_family = AF_INET;
    sddr.sin_port = htons(TCP_PORT);

    printf("host = %s\n", p_a->ip_camera);
    inet_aton(p_a->ip_camera, &sddr.sin_addr);
    if(connect(fd, (struct sockaddr*)&sddr, sizeof(struct sockaddr_in)) != 0)
        return 1;
    p_a->camerafd = fd;
    p_a->qSec = 0;
    return 0;
}

int option_to_camera(struct pthread_arguments* p_a)
{
    char option[REQUEST];
    char answer[OPTION_BUFFER_SIZE];
    sprintf(option, "%s%s%s%u%s", option_camera_first, p_a->ip_camera, option_camera_second, p_a->qSec, option_camera_three);
    printf("%s\n", option);
    if(write(p_a->camerafd, option, strlen(option)) < 0)
        return 1;
    if(read(p_a->camerafd, answer, sizeof(answer)) < 0)
        return 1;
    p_a->qSec++;
    
    if(DEBUG)
        printf("%s\n", answer);
    return 0;
}

int describe_to_camera(struct pthread_arguments* p_a) /// Description camera
{
    char describe[REQUEST];
    char answer[DESCRIBE_BUFFER_SIZE];
    sprintf(describe, "%s%s%s%u%s", describe_camera_first, p_a->ip_camera, describe_camera_second, p_a->qSec, describe_camera_three);
    printf("%s\n", describe);
    if(write(p_a->camerafd, describe, strlen(describe)) < 0)
        return 1;
    if(read(p_a->camerafd, answer, sizeof(answer)) < 0)
        return 1;
    if(DEBUG)
        printf("%s\n", answer);
    if(strncmp(answer+9, error_unauthorized, 3) == 0)
        p_a->flags |= FLAG_RTSP_AUTH;
    p_a->qSec++;
    if (p_a->flags & FLAG_RTSP_AUTH)
    {
        memset(describe, 0, sizeof(describe));
        unsigned char response[50] = {0};
        unsigned char H1[100] = {0};
        unsigned char H1_r[MD5_DIGEST_LENGTH];
        unsigned char H2[100] = {0};
        unsigned char H2_r[MD5_DIGEST_LENGTH];
        unsigned char H1_nonce_H2[300] = {0};
        char* time = (char*)H1_nonce_H2;
        char* nonce = NULL;
        char* realm = NULL;
        char* begin = NULL;
        char* end;

        /*
          Read from answer description realm
          In answer, realm is string to get response
          Example: realm="AXIS_00408CBEB36A"
         */
        realm = strstr(answer, "realm");
        if (realm == NULL)
            return 1;

        begin = strstr((const char*)realm, "\"");
        begin += 1;
        end = strstr(begin, "\"");
        strncpy((char*)p_a->realm, begin, end - begin);
        p_a->realm[end - begin] = '\0';
        
        /*
          Read from answer description nonce
          In answer, nonce is string to get response
          Example: nonce="0007d547Y294796f2161efd4668c264215f2ea8f7ccf13"
         */
        nonce = strstr(end+1, "nonce");
        if (nonce == NULL)
            return 1;

        begin = strstr((const char*)nonce, "\"");
        begin += 1;
        end = strstr(begin, "\"");
        strncpy((char*)p_a->nonce, begin, end - begin);
        p_a->nonce[end - begin] = '\0';
        
        // URI is string to get response
        sprintf((char*)p_a->uri, "rtsp://%s:554/axis-media/media.amp", p_a->ip_camera); 

        sprintf((char*)H1, "root:%s:pass", p_a->realm); // Create string for H1: username:realm:password 
        MD5_encoder(H1, H1_r, strlen((const char*)H1)); // Create hash for H1
        sprintf((char*)H2, "%s:%s", command_describe, p_a->uri); // Create string for H2: command:uri
        MD5_encoder(H2, H2_r, strlen((const char*)H2)); // Create hash for H2

        // Create H1_nonce_H2: H1:nonce:H2
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time, "%02x", H1_r[i]);
        
        time += sprintf(time, ":");
        strncpy(time,(const char*)p_a->nonce, strlen((const char*)(p_a->nonce))); 
        time += strlen((const char*)(p_a->nonce));
        time += sprintf(time, ":");
        
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time , "%02x", H2_r[i]);
        *(++time) = '\0';
        MD5_encoder(H1_nonce_H2, H1, strlen((const char*)H1_nonce_H2)); // Create hash for H1_nonce_H2

        // Create response
        time = (char*)response;
        for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time, "%02x", H1[i]);
        *(++time) = '\0';

        // Create message 
        sprintf(describe, "%s%s%s%u\r\n"
                "Authorization: Digest username=\"root\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", "
                "response=\"%s\"%s", describe_camera_first, p_a->ip_camera, describe_camera_second, p_a->qSec, p_a->realm, p_a->nonce, p_a->uri, response, describe_camera_three);
        
        if(write(p_a->camerafd, describe, strlen(describe)) < 0)
            return 1;
        if(read(p_a->camerafd, answer, sizeof(answer)) < 0)
            return 1;
        
        if(strncmp(answer+9, error_unauthorized, 3) == 0)
            return 1;
        p_a->qSec++;
    }
    if(DEBUG)
    {
        printf("%s\n", describe);
        printf("%s\n", answer);
    }
    char* start = strstr((char *)answer, "v=");
    char* end = strstr(start, "\r\n\r\n");
    end += 4;
    strncpy(p_a->sdp_camera, start, end - start);
    return 0;
}

int parameters_to_camera(struct pthread_arguments* p_a)
{
    char parameters[REQUEST];
    char answer[OPTION_BUFFER_SIZE];
    if (p_a->flags & FLAG_RTSP_AUTH)
    {
        memset(parameters, 0, sizeof(parameters));
        unsigned char response[50] = {0};
        unsigned char H1[100] = {0};
        unsigned char H1_r[MD5_DIGEST_LENGTH];
        unsigned char H2[100] = {0};
        unsigned char H2_r[MD5_DIGEST_LENGTH];
        unsigned char H1_nonce_H2[300] = {0};
        char* time = (char*)H1_nonce_H2;
               
        sprintf((char*)H1, "root:%s:pass", p_a->realm); // Create string for H1: username:realm:password 
        MD5_encoder(H1, H1_r, strlen((const char*)H1)); // Create hash for H1
        sprintf((char*)H2, "%s:%s", command_get_parameter, p_a->uri); // Create string for H2: command:uri
        MD5_encoder(H2, H2_r, strlen((const char*)H2)); // Create hash for H2        
       
        // Create H1_nonce_H2: H1:nonce:H2
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time, "%02x", H1_r[i]);
        
        time += sprintf(time, ":");
        strncpy(time,(const char*)p_a->nonce, strlen((const char*)p_a->nonce)); 
        time += strlen((const char*)p_a->nonce);
        time += sprintf(time, ":");
        
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time , "%02x", H2_r[i]);
        *(++time) = '\0';
        
        MD5_encoder(H1_nonce_H2, H1, strlen((const char*)H1_nonce_H2)); // Create hash for H1_nonce_H2        

        // Create response 
        time = (char*)response;
        for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time, "%02x", H1[i]);
        *(++time) = '\0';

        // Create nessage 
        sprintf(parameters, "%s%s%s%u\r\n"
                "Authorization: Digest username=\"root\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", "
                "response=\"%s\"%s%s\r\n\r\n", parameters_camera_first,
                p_a->ip_camera, parameters_camera_second, p_a->qSec, p_a->realm, p_a->nonce, p_a->uri, response, parameters_camera_three, p_a->session);
        
        
        if(write(p_a->camerafd, parameters, strlen(parameters)) < 0)
            return 1;
        if(read(p_a->camerafd, answer, sizeof(answer)) < 0)
            return 1;
        
        if(strncmp(answer+9, error_unauthorized, 3) == 0)
            return 1;
        p_a->qSec++;
    }
    else
    {
        sprintf(parameters, "%s%s%s%u%s%s\r\n\r\n", parameters_camera_first, p_a->ip_camera, parameters_camera_second, p_a->qSec, parameters_camera_three, p_a->session);
        if(write(p_a->camerafd, parameters, strlen(parameters)) < 0)
            return 1;
        if(read(p_a->camerafd, answer, sizeof(answer)) < 0)
            return 1;
        p_a->qSec++;
    }
    if(DEBUG)
    {
        printf("%s\n", parameters);
        printf("%s\n", answer);
    }
    return 0;
}

int setup_to_camera(struct pthread_arguments* p_a)    
{
    char setup[REQUEST];
    char answer[SETUP_BUFFER_SIZE];
    char* t1 = NULL;
    char* t2 = NULL;
    if (p_a->flags & FLAG_RTSP_AUTH)
    {
        memset(setup, 0, sizeof(setup));
        unsigned char response[50] = {0};
        unsigned char H1[100] = {0};
        unsigned char H1_r[MD5_DIGEST_LENGTH];
        unsigned char H2[100] = {0};
        unsigned char H2_r[MD5_DIGEST_LENGTH];
        unsigned char H1_nonce_H2[300] = {0};
        char* time = (char*)H1_nonce_H2;
               
        sprintf((char*)H1, "root:%s:pass", p_a->realm); // Create string for H1: username:realm:password 
        MD5_encoder(H1, H1_r, strlen((const char*)H1)); // Create hash for H1
        sprintf((char*)H2, "%s:%s", command_setup, p_a->uri); // Create string for H2: command:uri
        MD5_encoder(H2, H2_r, strlen((const char*)H2)); // Create hash for H2        
       
        // Create H1_nonce_H2: H1:nonce:H2
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time, "%02x", H1_r[i]);
        
        time += sprintf(time, ":");
        strncpy(time,(const char*)p_a->nonce, strlen((const char*)p_a->nonce)); 
        time += strlen((const char*)p_a->nonce);
        time += sprintf(time, ":");
        
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time , "%02x", H2_r[i]);
        *(++time) = '\0';
        
        MD5_encoder(H1_nonce_H2, H1, strlen((const char*)H1_nonce_H2)); // Create hash for H1_nonce_H2        

        // Create response 
        time = (char*)response;
        for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time, "%02x", H1[i]);
        *(++time) = '\0';

        // Create nessage 
        sprintf(setup, "%s%s%s%u\r\n"
                "Authorization: Digest username=\"root\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", "
                "response=\"%s\"%s%d-%d\r\n\r\n", setup_camera_first, p_a->ip_camera,
                setup_camera_second, p_a->qSec, p_a->realm, p_a->nonce, p_a->uri, response, setup_camera_three, p_a->port_camera, p_a->port_camera + 1);
        
        
        if(write(p_a->camerafd, setup, strlen(setup)) < 0)
            return 1;
        if(read(p_a->camerafd, answer, sizeof(answer)) < 0)
            return 1;
        if(strncmp(answer+9, error_unauthorized, 3) == 0)
            return 1;
        p_a->qSec++;
    }
    else
    {
        sprintf(setup, "%s%s%s%u%s%d-%d\r\n\r\n", setup_camera_first, p_a->ip_camera, setup_camera_second, p_a->qSec, setup_camera_three, p_a->port_camera, p_a->port_camera + 1);
        
        if(write(p_a->camerafd, setup, strlen(setup)) < 0)
            return 1;
        if(read(p_a->camerafd, answer, sizeof(answer)) < 0)
            return 1;
        p_a->qSec++;
    }
    if(DEBUG)
    {
        printf("%s\n",setup);
        printf("%s\n", answer);
    }
    t1 = strstr(answer, "Session");
    if (t1 == NULL)
        return 1;
    t1 += 9;
    t2 = strstr(t1, ";");
    strncpy(p_a->session, t1, t2 - t1);
    char p[10] = {0};
    t1 = strstr(answer, "server_port=");
    if (t1 == NULL)
        return 1;
    t1 += 12;
    t2 = strstr(t1, "-");
    strncpy(p, t1, t2 - t1);
    p_a->port_rtcp_camera = atoi(p) + 1;
    return 0;
}


int play_to_camera(struct pthread_arguments* p_a)
{
    char play[REQUEST];
    char answer[PLAY_BUFFER_SIZE];
    if (p_a->flags & FLAG_RTSP_AUTH)
    {
        memset(play, 0, sizeof(play));
        unsigned char response[50] = {0};
        unsigned char H1[100] = {0};
        unsigned char H1_r[MD5_DIGEST_LENGTH];
        unsigned char H2[100] = {0};
        unsigned char H2_r[MD5_DIGEST_LENGTH];
        unsigned char H1_nonce_H2[300] = {0};
        char* time = (char*)H1_nonce_H2;
        
        sprintf((char*)H1, "root:%s:pass", p_a->realm); // Create string for H1: username:realm:password 
        MD5_encoder(H1, H1_r, strlen((const char*)H1)); // Create hash for H1
        sprintf((char*)H2, "%s:%s", command_play, p_a->uri); // Create string for H2: command:uri
        MD5_encoder(H2, H2_r, strlen((const char*)H2)); // Create hash for H2        
       
        // Create H1_nonce_H2: H1:nonce:H2
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time, "%02x", H1_r[i]);
        
        time += sprintf(time, ":");
        strncpy(time,(const char*)p_a->nonce, strlen((const char*)p_a->nonce)); 
        time += strlen((const char*)p_a->nonce);
        time += sprintf(time, ":");
        
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time , "%02x", H2_r[i]);
        *(++time) = '\0';
        
        MD5_encoder(H1_nonce_H2, H1, strlen((const char*)H1_nonce_H2)); // Create hash for H1_nonce_H2        

        // Create response 
        time = (char*)response;
        for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time, "%02x", H1[i]);
        *(++time) = '\0';

        // Create nessage 
        sprintf(play, "%s%s%s%u\r\n"
                "Authorization: Digest username=\"root\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", "
                "response=\"%s\"%s%s%s\r\n\r\n", play_camera_first, p_a->ip_camera,
                play_camera_second, p_a->qSec, p_a->realm, p_a->nonce, p_a->uri, response, play_camera_three, p_a->session, play_camera_thirt);
        
        
        if(write(p_a->camerafd, play, strlen(play)) < 0)
            return 1;
        if(read(p_a->camerafd, answer, sizeof(answer)) < 0)
            return 1;
        if(strncmp(answer+9, error_unauthorized, 3) == 0)
            return 1;
        p_a->qSec++;
    }
    else
    {
        sprintf(play, "%s%s%s%u%s%s%s", play_camera_first, p_a->ip_camera, play_camera_second, p_a->qSec, play_camera_three, p_a->session, play_camera_thirt);
        if(write(p_a->camerafd, play, strlen(play)) < 0)
            return 1;
        if(read(p_a->camerafd, answer, sizeof(answer)) < 0)
            return 1;
        p_a->qSec++;
    }
    if(DEBUG)
    {
        printf("%s\n", play); 
        printf("%s\n", answer);
    }
    return 0;
}

int teardown_to_camera(struct pthread_arguments* p_a)    
{
    char teardown_text[512] = {0};
    char answer[PLAY_BUFFER_SIZE] = {0};
    if (p_a->flags & FLAG_RTSP_AUTH)
    {
        memset(teardown_text, 0, sizeof(teardown_text));
        unsigned char response[50] = {0};
        unsigned char H1[100] = {0};
        unsigned char H1_r[MD5_DIGEST_LENGTH];
        unsigned char H2[100] = {0};
        unsigned char H2_r[MD5_DIGEST_LENGTH];
        unsigned char H1_nonce_H2[300] = {0};
        char* time = (char*)H1_nonce_H2;
        
        sprintf((char*)H1, "root:%s:pass", p_a->realm); // Create string for H1: username:realm:password 
        MD5_encoder(H1, H1_r, strlen((const char*)H1)); // Create hash for H1
        sprintf((char*)H2, "%s:%s", command_teardown, p_a->uri); // Create string for H2: command:uri
        MD5_encoder(H2, H2_r, strlen((const char*)H2)); // Create hash for H2        
        
        // Create H1_nonce_H2: H1:nonce:H2
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time, "%02x", H1_r[i]);
        
        time += sprintf(time, ":");
        strncpy(time,(const char*)p_a->nonce, strlen((const char*)p_a->nonce)); 
        time += strlen((const char*)p_a->nonce);
        time += sprintf(time, ":");
        
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time , "%02x", H2_r[i]);
        *(++time) = '\0';
        
        MD5_encoder(H1_nonce_H2, H1, strlen((const char*)H1_nonce_H2)); // Create hash for H1_nonce_H2        
        
        // Create response 
        time = (char*)response;
        for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
            time += sprintf(time, "%02x", H1[i]);
        *(++time) = '\0';

        // Create nessage 
        sprintf(teardown_text, "%s%s%s%u\r\n"
                "Authorization: Digest username=\"root\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", "
                "response=\"%s\"%s%s\r\n\r\n", teardown_camera_first, p_a->ip_camera,
                teardown_camera_second, p_a->qSec, p_a->realm, p_a->nonce, p_a->uri, response, teardown_camera_three, p_a->session);
           
        if (send(p_a->camerafd, teardown_text, strlen(teardown_text), 0) < 0)
            return 1;
        if (recv(p_a->camerafd, answer, sizeof(answer), 0) < 0)
            return 1;
        p_a->qSec++;
    }
    else
    {        
        sprintf(teardown_text, "%s%s%s%u%s%s\r\n\r\n", teardown_camera_first, p_a->ip_camera, teardown_camera_second, p_a->qSec, teardown_camera_three, p_a->session);
        if (send(p_a->camerafd, teardown_text, strlen(teardown_text), 0) < 0)
            return 1;
        if (recv(p_a->camerafd, answer, sizeof(answer), 0) < 0)
            return 1;
        p_a->qSec++;
    }
    if(DEBUG)
    {
        printf("%s\n", teardown_text); 
        printf("%s\n", answer);
    }
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
    char* err = NULL;
    struct sockaddr_in servaddr; 
    printf("Ip: %s\n", p_a->ip_server);
    strcpy(p_a->uflag_server, "sEMT");
    strcpy(hostname, p_a->ip_server);
    /// 74.125.134.127 stun google

    setSockaddr(&servaddr, (unsigned char*)"74.125.134.127", 19302);
    err = "Error with create socket for stun";
    if (createSockaddr(&p_a->stun_from_server, NULL, p_a->port_ice, &p_a->socket_stream))
        return 1;
     
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

void iceParse(struct pthread_arguments* p_a) //candidate:2182926537 1 udp 2113937151 10.168.191.246 56429 typ host generation 0 ufrag 2uGL network-cost 999
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

int pwdParse(struct pthread_arguments* p_a)
{
    char* t1 = NULL;
    char* t2;
    int i = 0;
    memset(p_a->pwd_browser.s, 0, sizeof(p_a->pwd_browser.s)); 
    t1 = strstr(p_a->sdp_offer, "a=ice-pwd:");
    if (t1 == NULL)
        return 1;
    t1 += 10;
    t2 = strstr(t1, "\r\n");
    strncpy(p_a->pwd_browser.s, t1, t2-t1);
    p_a->pwd_browser.len = t2 - t1;
    printf("PWD: \n+++%s+++\n", p_a->pwd_browser.s);
    return 0;
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

void sendSocketMessage(struct lws* wsi, char* message, int index)
{
    unsigned char buf[4000] = {0};
    int n = sprintf((char*)buf+ LWS_PRE , "%d:%s", index, message);
    lws_write(wsi, &buf[LWS_PRE], n, LWS_WRITE_TEXT);
}

int send_Stun_Sdp_Ice(struct pthread_arguments* arg_pthread, int index_list, struct lws* wsi)
{
    printf("Create Threads SDP\n");
    char buf[3700];
    char* err = NULL;
    do{
        err = "Error with create ice.";
        if (create_ice(arg_pthread))
            break;
        printf("Create_ice successful.\n");
        
        err = "Error with stun requests";
        if (stun_request(arg_pthread))
            break;
        printf("Stun_requests successful.\n");

        err = "Error with sdp parse";
        if (sdpParse(arg_pthread))
            break;
        printf("SdpParse successful.\n");

        memset(buf, 0, sizeof(buf));
        sprintf((char *)buf, "%s%s", type_sdp, arg_pthread->sdp_answer); // Send answer into browser
        sendSocketMessage(wsi, buf, index_list);

        memset(buf, 0, sizeof(buf));
        sprintf((char *)buf + LWS_PRE, "%s%s", type_ice, arg_pthread->ice_server); // Send ice candidate into browser
        sendSocketMessage(wsi, buf, index_list);

        err = "Error with stun response";
        if (stun_response(arg_pthread, NULL, 0, NULL))
            break;
        printf("stun_response succeful.\n");
        err = "Can't create thread!";
        if (pthread_create(&tchilds[index_list], 0, udp_stream, arg_pthread) < 0)
            break;
        //printf("ID threads: %");
        err = NULL;
    } while(0);
    if (err)
    {
        sendSocketMessage(wsi, err, index_list);
        free_all(arg_pthread);
        printf("Error\n");
        return 1;
    }    
    return 0;
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

void MD5_encoder(unsigned char* data, unsigned char* hash, unsigned int n)
{
    
    MD5_CTX md5_hash;
    MD5_Init(&md5_hash);
    MD5_Update(&md5_hash, data, n);
    MD5_Final(hash, &md5_hash);
}

void free_all(struct pthread_arguments* p_a)
{
    if(p_a->socket_rtp_fd > 0) {
        close(p_a->socket_rtp_fd);
        p_a->socket_rtp_fd = -1;
    }
    if(p_a->socket_rtcp_fd > 0) {
        close(p_a->socket_rtcp_fd);
        p_a->socket_rtcp_fd = -1;
    }
    if(p_a->socket_stream > 0) {
        close(p_a->socket_stream);
        p_a->socket_stream = -1;
    }
    if(p_a->camerafd > 0) {
        close(p_a->camerafd);
        p_a->camerafd = -1;
    }
    dtls_connection_cleanup(&p_a->dtls_cert);
    dtls_fingerprint_free(p_a);
    crypto_cleanup(&p_a->crypto);
    printf("Crypto_cleanup crypto\n");
    crypto_cleanup(&p_a->crypto_from_camera);
    printf("Crypto_cleanup crypto_from_camera\n");
    crypto_cleanup(&p_a->crypto_rtcp);
    printf("Crypto_cleanup crypto_rtcp\n");
    free(p_a);
    printf("Free p_a\n");
    p_a = NULL;
    printf("Free\n");
}
