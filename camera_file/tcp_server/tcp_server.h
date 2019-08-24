#ifndef SYMBOL
#define SYMBOL
#include <iostream>
#include <stdio.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/poll.h>
#include "Base64.h"
#include <openssl/sha.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define DEBUG 1
#define LISTEN 10
#define TEXT_FRAME 0x1
#define CLOSE_CONNECTION 0x8
#define FRAGMENT_FRAME 0x0

static char response_ws[] = "HTTP/1.1 101 Switching Protocols\r\n"
"Upgrade: websocket\r\n"
"Connection: Upgrade\r\n"
"Sec-WebSocket-Accept: "; 

static char Key[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
struct args_thread
{
    int clientfd;
};
unsigned char* getMiddleText(unsigned char* b, uint &s);
unsigned char* getSmallText(unsigned char* b, uint &s);
void signalInt(int signum);
void signalKill(int signum);
void debug();

#endif
