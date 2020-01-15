#include "tcp_server.h"
#include "h264_camera.h"
int sockfd;
void getSmallText(unsigned char *in, unsigned char *out, uint &s)
{
    unsigned char masking[4];
    std::cout << "size <= 125" << std::endl;
    std::cout << "Size = " << std::hex << s << std::endl;
    masking[0] = in[2];
    masking[1] = in[3];
    masking[2] = in[4];
    masking[3] = in[5];
    unsigned int i = 6;
    unsigned int pl = 0;
    for (; pl < s; i++, pl++)
    {
        out[pl] = masking[pl % 4] ^ in[i];
    }
}
void getMiddleText(unsigned char *in, unsigned char *out, uint &s)
{
    unsigned char masking[4];
    std::cout << "Size > 125 and < 65535" << std::endl;
    s = in[2] << 8 | in[3];
    std::cout << std::dec << "Size = " << std::hex << s << std::endl;
    masking[0] = in[4];
    masking[1] = in[5];
    masking[2] = in[6];
    masking[3] = in[7];
    unsigned int i = 8;
    unsigned int pl = 0;
    for (; pl < s; i++, pl++)
    {
        out[pl] = masking[pl % 4] ^ in[i];
    }
}
void debug(unsigned char *buf)
{
    std::cout << "FIN = 0x" << std::hex << ((buf[0] & 0x80) >> 7) << std::endl;
    std::cout << "RSV1 = 0x" << std::hex << ((buf[0] & 0x40) >> 6) << std::endl;
    std::cout << "RSV2 = 0x" << std::hex << ((buf[0] & 0x20) >> 5) << std::endl;
    std::cout << "RSV3 = 0x" << std::hex << ((buf[0] & 0x10) >> 4) << std::endl;
    std::cout << "Opcode = 0x" << std::hex << (buf[0] & 0x0f) << std::endl;
    std::cout << "MASK = 0x" << std::hex << ((buf[1] & 0x80) >> 7) << std::endl;
}
int Read_text(unsigned char *out_message, int clientfd)
{
    unsigned char in_buffer[1000] = {0};
    int res = read(clientfd, (char*)in_buffer, sizeof(in_buffer));
    if (res > 0)
    {
        if (DEBUG)
            debug(in_buffer);
        unsigned int size = (in_buffer[1] & 0x7F);
        uint opcode = (in_buffer[0] & 0x0f);
        if (opcode == TEXT_FRAME)
        {
            if (size <= 125)
                getSmallText(in_buffer, out_message, size);
            else if (size == 126)
                getMiddleText(in_buffer, out_message, size);
            else
                std::cout << "Size > 65553" << std::endl;
        }
        if (opcode == CLOSE_CONNECTION)
            return 1;
    }
    else
    {
        if (res < 0)
            return -1;
    }
    return 0;
}
int Write_text(unsigned char *in_message, int clientfd)
{
    unsigned char out_buffer[1024];
    int message_size = (int)strlen((char *)(in_message));
    if (message_size <= 125)
    {
        strncpy((char*)(out_buffer + 2), (char*)in_message, message_size);
        out_buffer[0] = 0x81;
        out_buffer[1] = (char)message_size;
        if (send(clientfd, out_buffer, message_size + 2, 0) == -1) // отправка
        {
            std::cout << "Error with text < 126" << std::endl;
            return -1;
        }
    }
    else
    {
        strncpy((char*)(out_buffer + 4), (char*)in_message, message_size);
        out_buffer[0] = 0x81;
        out_buffer[1] = (char)126;
        out_buffer[2] = (message_size >> 8) & 0xff;
        out_buffer[3] = message_size & 0xff;
        if (send(clientfd, out_buffer, message_size + 4, 0) == -1) // отправка
        {
            std::cout << "Error with text > 126" << std::endl;
            return -1;
        }
    }
    
    return 0;
}
int sdpParse(char* des)
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
    memset((char*)des, 0, sizeof(des));
    sprintf(des, "%s%s%s%s%s", version, sess_version, sdp_f, fmtp, sdp_e);
    if(DEBUG)
        std::cout << des << std::endl;
    return 0;
}
void *tcp_client_webrtc(void *arg)
{
    args_thread a_t;
    char host[20] = {0};
    int camerafd;
    struct sockaddr_in sddr;
    char describe[DESCRIBE_BUFFER_SIZE];
    a_t.clientfd = ((args_thread *)arg)->clientfd;
    std::cout << "clientfd: " << a_t.clientfd << std::endl;
    unsigned char out_message[BUFFER_OUT_OR_IN] = {0};
    unsigned char in_message[BUFFER_OUT_OR_IN] = {0};
    int res = Read_text(out_message, a_t.clientfd);
    if (res < 0)
    {
        std::cout << "Error with read" << std::endl;
        return 0;
    }
    else
    {
        if (res > 0)
        {
            std::cout << "Was receive Close" << std::endl;
            return 0;
        }
    }
    strcpy(host,(char*)out_message);
    memset(out_message,0,sizeof(out_message));
    if(connect_camera(sddr, camerafd, host) != 0)
    {
        strcpy((char*)in_message,"Error with connect to camera");
        res = Write_text(in_message, a_t.clientfd);
        if (res != 0)
        {
            std::cout << "Error with send" << std::endl;
        }
        return 0;
    }
    strcpy((char*)in_message,"OK");
    res = Write_text(in_message, a_t.clientfd);
    if (res != 0)
    {
        std::cout << "Error with send" << std::endl;
        return 0;
    }
    if(option_to_camera(sddr, camerafd, host) != 0)
    {
        std::cout << "Error with option" << std::endl;
        return 0;
    }
    if(describe_to_camera(sddr, camerafd, host, (char*)out_message) != 0)
    {
        std::cout << "Error with describe" << std::endl;
        return 0;
    }
    strcpy(describe, strstr((char*)out_message,"v="));
    sdpParse(describe);
    memset(in_message,0,sizeof(in_message));
    sprintf((char*)in_message, "%s%s", type_sdp, describe);
    res = Write_text(in_message, a_t.clientfd);
    if (res != 0)
    {
        std::cout << "Error with send" << std::endl;
        close(a_t.clientfd);
    }
    
    memset(out_message,0,sizeof(out_message));
    res = Read_text(out_message, a_t.clientfd);
    if (res < 0)
    {
        std::cout << "Error with read" << std::endl;
        close(a_t.clientfd);
    }
    else
    {
        if (res > 0)
        {
            std::cout << "Was receive Close" << std::endl;
            return 0;
        }
    }
    std::cout << out_message << std::endl;
    memset(out_message,0,sizeof(out_message));
    return 0;
}
void signalInt(int signum)
{
    std::cout << "Signal Ctrl+C" << std::endl;
    close(sockfd);
    exit(1);
}
void signalKill(int signum)
{
    std::cout << "Signal Ctrl+KILL" << std::endl;
    close(sockfd);
    exit(1);
}
int main(int argv, char **argc)
{
    signal(SIGINT, signalInt);
    signal(SIGKILL, signalKill);
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;

    pthread_t threads[LISTEN];
    uint clientlen;
    int clientfd;
    uint port = 8555;
    int res;
    char buf[550] = {0};
    char resultstr[60] = {0};
    char *p = NULL;
    uint size_client = 0;
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
    listen(sockfd, LISTEN);
    for (;;)
    {
        clientlen = sizeof(clientaddr);
        if ((clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientlen)) > 0)
        {
            std::cout << "Connection: " << inet_ntoa(clientaddr.sin_addr) << ":" << ntohs(clientaddr.sin_port) << std::endl;
            res = read(clientfd, buf, sizeof(buf));
            if (res > 0)
            {
                std::cout << "Was read bytes: " << res << std::endl;
                std::cout << buf << std::endl;
                if ((p = strstr(buf, "Sec-WebSocket-Key:")) != NULL)
                {
                    p += sizeof("Sec-WebSocket-Key:");
                    strncpy(resultstr, p, 24);
                    strcat(resultstr, Key);
                    unsigned char *sha_result = SHA1((unsigned char *)(resultstr), strlen(resultstr), nullptr);
                    std::string base64_result = base64_encode((const char *)(sha_result));
                    strcpy(resultstr, base64_result.c_str());
                    std::cout << resultstr << std::endl;
                    sprintf(buf, "%s%s%s", response_ws, resultstr, "\r\n\r\n");
                    std::cout << buf << std::endl;
                    write(clientfd, buf, strlen(buf));
                    args_thread time;
                    time.clientfd = clientfd;
                    pthread_create(&threads[size_client], 0, tcp_client_webrtc, &time);
                    pthread_join(threads[size_client], 0);
                    break;
                }
                else
                {
                    std::cout << "Sec-WebSocket-Key wasn`t found." << std::endl;
                }
            }
            else
            {
                perror("Read");
            }
        }
    }
    close(sockfd);
    return 0;
}
