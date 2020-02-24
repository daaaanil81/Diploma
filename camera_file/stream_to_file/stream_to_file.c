#include <stdio.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h> 
int setup_to_camera(int sockfd, char* host, unsigned int port, char* session)
{
	char setup_text[512] = {0};        
	sprintf(setup_text, "SETUP rtsp://%s/axis-media/media.amp/trackID=1 RTSP/1.0\r\nCSeq: 1\r\nUser-Agent: RTSPClient\r\nTransport: RTP/AVP;unicast;client_port=%d-%d\r\n\r\n", host, port, port+1);
	printf("%s", setup_text);
	char buf[512] = {0};
	if (send(sockfd, setup_text, strlen(setup_text), 0) < 0)
	{
		perror("Send setup");
		return 1;
	}
	if (recv(sockfd, buf, sizeof(buf), 0) < 0)
	{
		perror("Read setup");
		return 1;
	}	
	char* t1 = strstr(buf, "Session:");
	if(t1 == NULL)
	{
		perror("strstr");
		return 1;
	}
	t1 += 9;
	char* t2 = strstr(t1, ";");
	strncpy(session, t1, t2 - t1);
	return 0;
}
int play_to_camera(int sockfd, char* host, char* session)
{
	char play_text[512] = {0};
	sprintf(play_text, "PLAY rtsp://%s/axis-media/media.amp RTSP/1.0\r\nCSeq: 2\r\nUser-Agent: RTSPClient\r\nSession: %s\r\nRange: npt=0.000-\r\n\r\n", host, session);
	printf("%s", play_text);
	char buf[512] = {0};
	if (send(sockfd, play_text, strlen(play_text), 0) < 0)
	{
		perror("Send play");
		return 1;
	}
	if (recv(sockfd, buf, sizeof(buf), 0) < 0)
	{
		perror("Read play");
		return 1;
	}	
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
int main(int argc, char* argv[])
{
	char ip_camera[16] = {0};
	unsigned int port_camera = 554;
	unsigned int port_stream = 0;
	int socket_udp;
	struct sockaddr_in tcp_camera_addr;
	struct sockaddr_in servaddr;
	int socket_tcp;
	char session[10] = {0};
	unsigned short i = 0;
	FILE* fd;
	if (argv[1] == NULL || argv[2] == NULL)
	{
		perror("ARGV");
		return 1;
	}
	strncpy(ip_camera, argv[1], sizeof(ip_camera));
	ip_camera[sizeof(ip_camera)-1] = '\0';
       	port_stream = atoi(argv[2]);
	
	if ((socket_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket tcp");
            return 1;
        }
	

	bzero(&tcp_camera_addr, sizeof(tcp_camera_addr));
	tcp_camera_addr.sin_family = AF_INET;
        tcp_camera_addr.sin_port = htons(port_camera);
	if (inet_aton(ip_camera, &tcp_camera_addr.sin_addr) == 0)
	{
		perror("inet phon");
		return 1;
	}

	if (connect(socket_tcp, (struct sockaddr *)&tcp_camera_addr, sizeof(tcp_camera_addr)) == -1) 
	{
            	perror("connect");
            	return 1;
        }
	if (setup_to_camera(socket_tcp, ip_camera, port_stream, session) != 0)
	{
		perror("Setup");
		return 1;
	}
	if ((socket_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
	{
        	perror("socket udp");
        	return 0;
        }

	servaddr.sin_family = AF_INET; // IPv4 
        servaddr.sin_addr.s_addr = INADDR_ANY; 
        servaddr.sin_port = htons(port_stream); 
    	if (bind(socket_udp, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
    	{ 
       		perror("bind failed"); 
        	return 1; 
    	} 

	if (play_to_camera(socket_tcp, ip_camera, session) != 0)
	{	
		perror("Play");
		return 1;
	}		
	fd = fopen("UDP_stream", "w");
	char buffer[1442] = {0};
	while(i < 10)
	{
		int n = recvfrom(socket_udp, (char *)buffer, sizeof(buffer), 0, NULL, 0); 
		printf("Read: %d\n", n);
		n = fwrite(buffer, sizeof(char), n, fd);
		printf("Write: %d\n\n", n);
		i++;
	}	
	fclose(fd);
	if (teardown_to_camera(socket_tcp, ip_camera, session) != 0)
	{
		perror("Teardown");
		return 1;
	}
	close(socket_tcp);
	close(socket_udp);
	return 0;
}
