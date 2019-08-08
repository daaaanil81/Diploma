#include "tcp_server.h"
int sockfd;
void getSmallText(unsigned char *in, unsigned char *out, uint &s)
{
    unsigned char masking[4];
    std::cout << "size <= 125" << std::endl;
    std::cout << "Size = " << s << std::endl;
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
    std::cout << std::dec << "Size = " << s << std::endl;
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
    std::cout << "FIN = 0x" << std::hex << (buf[0] & 0x01) << std::endl;
    std::cout << "RSV1 = 0x" << std::hex << (buf[0] & 0x02) << std::endl;
    std::cout << "RSV2 = 0x" << std::hex << (buf[0] & 0x04) << std::endl;
    std::cout << "RSV3 = 0x" << std::hex << (buf[0] & 0x08) << std::endl;
    std::cout << "Opcode = 0x" << std::hex << (buf[0] & 0x0f) << std::endl;
    std::cout << "MASK = 0x" << std::hex << (buf[1] & 0x01) << std::endl;
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
    unsigned char out_buffer[1000];
    int message_size = (int)strlen((char *)(in_message));
    if (message_size <= 125)
    {
        strncpy((char*)(out_buffer + 2), (char*)in_message, message_size);
        out_buffer[0] = 0x81;
        out_buffer[1] = (char)message_size;
    }
    else
    {
        strncpy((char*)(out_buffer + 4), (char*)in_message, message_size);
        out_buffer[0] = 0x81;
        out_buffer[1] = (char)126;
        out_buffer[2] = (message_size >> 8) & 0xff;
        out_buffer[3] = message_size & 0xff;
    }
    if (send(clientfd, out_buffer, message_size + 4, 0) == -1) // отправка
    {

        return -1;
    }
    return 0;
}
void *tcp_client_webrtc(void *arg)
{
    args_thread a_t;
    a_t.clientfd = ((args_thread *)arg)->clientfd;
    std::cout << "clientfd: " << a_t.clientfd << std::endl;
    unsigned char out_message[1000] = {0};
    unsigned char in_message[1000] = {0};
    int res = Read_text(out_message, a_t.clientfd);
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
            close(a_t.clientfd);
        }
    }
    std::cout << out_message << std::endl;
    strcpy((char*)in_message,"Hi client");
    res = Write_text(in_message, a_t.clientfd);
    if (res != 0)
    {
        std::cout << "Error with send" << std::endl;
        close(a_t.clientfd);
    }
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
