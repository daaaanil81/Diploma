
#include "h264_camera.h"

static int interrupted;
static int index_arr;
unsigned char buf[4100] = {0};
char timing[4300] = {0};
static struct pthread_arguments *arg_pthread = NULL; /// Arguments for settings connection
void* udp_stream(void* arg)
{
    	int socket_udp;
	int socket_to_rtpengine;
	int socket_from_rtpengine;
	struct sockaddr_in servaddr;
	struct sockaddr_in addr_to_rtpengine;
	struct sockaddr_in addr_from_rtpengine;
	char buf_camera[512] = {0};
	printf("Udp stream create.\n");
    	struct pthread_arguments *p_a = (struct pthread_arguments*)arg;
    
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET; // IPv4 
        servaddr.sin_addr.s_addr = INADDR_ANY; 
        servaddr.sin_port = htons(p_a->port_camera); 
	
	if ((socket_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
 	{
        	perror("socket udp");
        	return 0;
        }
	if (bind(socket_udp, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
    	{ 
       		perror("bind failed"); 
        	return 0; 
    	} 

	play_to_camera(p_a->sddr, p_a->camerafd, p_a->ip_camera, p_a->session);	
    	
	socket_from_rtpengine = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&addr_to_rtpengine, 0, sizeof(addr_to_rtpengine));
	addr_to_rtpengine.sin_family = AF_INET; // IPv4 
        addr_to_rtpengine.sin_addr.s_addr = INADDR_ANY; 
        addr_to_rtpengine.sin_port = htons(p_a->port_to_rtpengine); 
	
	memset(&addr_from_rtpengine, 0, sizeof(addr_from_rtpengine));
	addr_from_rtpengine.sin_family = AF_INET; // IPv4 
        addr_from_rtpengine.sin_addr.s_addr = INADDR_ANY; 
        addr_from_rtpengine.sin_port = htons(p_a->port_from_rtpengine);

	if (bind(socket_from_rtpengine, (const struct sockaddr *)&addr_from_rtpengine, sizeof(addr_from_rtpengine)) < 0) 
    	{ 
       		perror("bind failed"); 
        	return 0; 
    	} 
	int i = 0;
	while(i < 5000)
	{
		int n = recvfrom(socket_udp, (char *)buf_camera, sizeof(buf_camera), 0, NULL, 0); //resvfrom stream from camera
		
		i++;
	}
	//TEARDOWN to camera
	close(socket_from_rtpengine);
	close(p_a->camerafd);
    	free(p_a);
    	return 0;
}
static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    return 0;
}
static int callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_PROTOCOL_INIT: /// Initial protocols
            index_arr = 0;
            while(index_arr < SIZE_CAMERA)
            {
                afc[index_arr].port_ice = 0;
                afc[index_arr].port_camera = 0;
                afc[index_arr].ip[0] = '\0';
                index_arr++;
            }
            break;
        case LWS_CALLBACK_ESTABLISHED: /// Connection clients
            printf("connection established\n");
            if(!busy)
            {
                memset(buf, 0, sizeof(buf));
                arg_pthread = (struct pthread_arguments*)calloc(1, sizeof(pthread_arguments)); /// Create struct for settings
                if(arg_pthread == NULL)
                {
                    printf("Error with arg_pthread");/// Error if problems with create object struct pthread_arguments
                    memcpy((char *)buf + LWS_PRE, "Error", strlen("Error"));
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error"), LWS_WRITE_TEXT); /// Send Error into clients
                }
                else
                {
                    memcpy((char *)buf + LWS_PRE, "Connect", strlen("Connect"));
                    lws_write(wsi, &buf[LWS_PRE], strlen("Connect"), LWS_WRITE_TEXT); /// Send connect into clients
                    busy = true; /// Server will work only one clients on time
                    printf("Connect\n");
                }
            }
            else
            {
                memset(buf, 0, sizeof(buf));
                memcpy((char *)buf + LWS_PRE, "Busy", strlen("Busy"));
                lws_write(wsi, &buf[LWS_PRE], strlen("Busy"), LWS_WRITE_TEXT); /// Send message about that server have already worked with client
            }
        case LWS_CALLBACK_RECEIVE:
            memset(buf, 0, sizeof(buf));
            if(len <= 1)
            {
                break;
            }
            strncpy((char*)buf, (char*)in, len);
            
            if(strcmp((char*)buf, "Connect") == 0) /// Repiet request on connection
            {
                //printf("Connect\n");
                if(!busy)
                {
                    memset(buf, 0, sizeof(buf));
                    arg_pthread = (pthread_arguments*)calloc(1, sizeof(pthread_arguments));
                    if(arg_pthread == NULL)
                    {
                        printf("Error with arg_pthread");
                        memcpy((char *)buf + LWS_PRE, "Error", strlen("Error"));
                        lws_write(wsi, &buf[LWS_PRE], strlen("Error"), LWS_WRITE_TEXT);
                        break;
                    }
                    else
                    {
                        printf("Connect");
                        memcpy((char *)buf + LWS_PRE, "Connect", strlen("Connect"));
                        lws_write(wsi, &buf[LWS_PRE], strlen("Connect"), LWS_WRITE_TEXT);
                        busy = true;
                    }
                }
                else
                {
                    memset(buf, 0, sizeof(buf));
                    memcpy((char *)buf + LWS_PRE, "Busy", strlen("Busy"));
                    lws_write(wsi, &buf[LWS_PRE], strlen("Busy"), LWS_WRITE_TEXT);
                }
            }
             /// Server receive host from camera
            if(strncmp((char*)buf, "host", strlen("host")) == 0)
            {
                strcpy(arg_pthread->ip_camera, (char*)buf + strlen("host"));
                memset(buf, 0, sizeof(buf));
                index_arr = 0;
                other_user = false;
                while( afc[index_arr].port_ice != 0 && index_arr < SIZE_CAMERA ) /// Search stream in list
                {
                    if(strncmp(afc[index_arr].ip, arg_pthread->ip_camera, strlen(arg_pthread->ip_camera)) == 0)
                    {
                        other_user =  false;
                        break;
                    }
                    index_arr++;
                }
                if(other_user) /// If stream from camera other user have already used
                {
                    printf("Camera used other people.\n\n");
                    memcpy((char *)buf + LWS_PRE, "Error: Camera used other people.", strlen("Error: Camera used other people."));
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error: Camera used other people."), LWS_WRITE_TEXT);
                    busy = false;
                    break;
                }
                if(index_arr == 0) /// If stream first on server
                {
                    arg_pthread->port_ice = afc[index_arr].port_ice = port_ice_start;
                    arg_pthread->port_camera = afc[index_arr].port_camera = port_camera_start;
                    strcpy(afc[index_arr].ip, arg_pthread->ip_camera);
                }
                else /// If this stream no first on server
                {
                    arg_pthread->port_ice = afc[index_arr].port_ice = afc[index_arr-1].port_ice+1000;
                    arg_pthread->port_camera = afc[index_arr].port_camera = afc[index_arr-1].port_camera+1000;
                    strcpy(afc[index_arr].ip, arg_pthread->ip_camera);
                }
                 /** Connection on stream
                 *
                 *
                 *
                 */
                if(connect_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera) != 0)
                {
                    memcpy((char *)buf + LWS_PRE, "Error with connect to camera", strlen("Error with connect to camera"));
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with connect to camera"), LWS_WRITE_TEXT);
                    busy = false;
                    break;
                }
                else
                {
                    memcpy((char *)buf + LWS_PRE, "OK", strlen("OK")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("OK"), LWS_WRITE_TEXT);
                    printf("Next steps\n\n\n");
                     /** Option into camera
                     *
                     *
                     *
                     */
                    if(option_to_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera) != 0)
                    {
                        printf("Error with option\n");
                        busy = false;
                        break;
                    }
                     /** Describe into camera
                     *
                     *
                     *
                     */
                    if(describe_to_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera, (char*)buf) != 0)
                    {
                        printf("Error with describe\n");
                        busy = false;
                        break;
                    }
                    strcpy(arg_pthread->sdp_camera, strstr((char*)buf,"v="));
                     /** Create server's ice candidate for browser
                     *
                     *
                     *
                     */
                     create_ice(arg_pthread);
                     /** Parsing sdp camera's for create sdp answer for browser
                     *
                     *
                     *
                     */
                    sdpParse(arg_pthread);
                    memset(buf, 0, sizeof(buf));
                }
            }
            if(strncmp((char*)buf, "SDP", strlen("SDP")) == 0)/// Server receive SDP offer from browser and browser wait answer
            {
                printf("SDP Offer\n");
		memset(arg_pthread->sdp_offer, 0, sizeof(arg_pthread->sdp_offer));
		//generationSDP_BROWSER(arg_pthread, buf);
 		strcpy(timing, (char*)buf + strlen("SDP"));
                //pwdParse(arg_pthread); 
                //printf("Offer: \n\n%s\n", arg_pthread->sdp_offer);
                 /** Setup into camera
                 *
                 *
                 *
                 */
                if(setup_to_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera, arg_pthread->port_camera, arg_pthread->session) != 0)
                {
                    printf("Error with setup\n");
                    busy = false;
                    break;
                }
                
                //memset(buf, 0, sizeof(buf));
                //sprintf((char*)buf + LWS_PRE, "%s%s", type_sdp, arg_pthread->sdp_answer);
                //lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->sdp_answer)+strlen(type_sdp), LWS_WRITE_TEXT); /// Send answer into browser
                
                //memset(buf, 0, sizeof(buf));
                //sprintf((char*)buf + LWS_PRE, "%s%s", type_ice, arg_pthread->ice_server);
                //lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->ice_server)+strlen(type_ice), LWS_WRITE_TEXT); /// Send ice candidate into browser
                sdp_step = true; /// Marker, sdp step was finished
                //printf("ICE send\n");
                if(ice_step && sdp_step) /// was finished all step
                {
		    sendSDP_rtpengine(arg_pthread);
                    
		    printf("Send answer: \n");
		    memset(buf, 0, sizeof(buf));
                    sprintf((char*)buf + LWS_PRE, "%s%s", type_sdp, arg_pthread->sdp_answer);
                    lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->sdp_answer)+strlen(type_sdp), LWS_WRITE_TEXT); /// Send answer into browser
		    
		    printf("Send Ice: \n");
		    memset(buf, 0, sizeof(buf));
                    sprintf((char*)buf + LWS_PRE, "%s%s", type_ice, arg_pthread->ice_server);
                    lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->ice_server)+strlen(type_ice), LWS_WRITE_TEXT); /// Send ice candidate into browser
		    
                    /*if(pthread_create(&arg_pthread->tchild,0,udp_stream,arg_pthread) < 0)
                    {
                        perror("Can't create thread!");
                    }
                    pthread_join(arg_pthread->tchild,0);*/
                }
            }
            if(strncmp((char*)buf, "ICE", strlen("ICE")) == 0)
            {
                strcpy(arg_pthread->ice_browser, (char*)buf + strlen("ICE"));
                printf("ICE: \n\n%s\n", arg_pthread->ice_browser);
		//unsigned int port_ice_browser; /// Port ice dandidate browser
    		//iceParse(p_a->ice_browser, p_a->ip_browser, p_a->ip_server, port_ice_browser, p_a->uflag_browser);
    		iceParse(arg_pthread);
		
                ice_step = true; /// Marker, ice step was finished
                if(ice_step && sdp_step)
                {
                    printf("Create Threads\n");
		    generationSDP_BROWSER(arg_pthread, timing);
		    //printf("Offer: \n\n%s\n", arg_pthread->sdp_offer);
		    sendSDP_rtpengine(arg_pthread);
		  
                    //printf("Offer: \n\n%s\n", arg_pthread->sdp_offer);
		    	 
		    printf("Send answer: \n");
		    memset(buf, 0, sizeof(buf));
                    sprintf((char*)buf + LWS_PRE, "%s%s", type_sdp, arg_pthread->sdp_answer);
	  	    lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->sdp_answer)+strlen(type_sdp), LWS_WRITE_TEXT); /// Send answer into browser
		    
		    printf("Send Ice: \n");
		    memset(buf, 0, sizeof(buf));
                    sprintf((char*)buf + LWS_PRE, "%s%s", type_ice, arg_pthread->ice_server);
                    lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->ice_server)+strlen(type_ice), LWS_WRITE_TEXT); /// Send ice candidate into browser
		    busy = false;
		    sleep(1);
		    free(arg_pthread);		    
                    /*if(pthread_create(&arg_pthread->tchild,0,udp_stream,arg_pthread) < 0)
                    {
                        perror("Can't create thread!");
                    }
		    */
                    //pthread_join(arg_pthread->tchild,0);
		    
                }
            }
            break;
        case LWS_CALLBACK_CLOSED: /// Close server
            printf("Close\n");
	    if(busy == true)
	    {
            	free(arg_pthread);
	    }
            break;
        default:
            break;
    }
    
    return 0;
}
static struct lws_protocols protocols[] = {
    { "http", callback_http, 0, 0 },
    { "lws-minimal", callback_dumb_increment, 0, 0, 0, NULL, 0 },
    { NULL, NULL, 0, 0 } /* terminator*/
};

void sigint_handler(int sig)
{
    interrupted = 1;
    
}
void handler_sigsegv(int signum)
{
    printf("My Segmentation fault\n");
    interrupted = 1;
}

int main(int argc, const char **argv)
{
    struct lws_context_creation_info info;
    struct lws_context *context;
    const char *p;
    int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
    
    signal(SIGINT, sigint_handler);
    signal(SIGSEGV, handler_sigsegv);

    lws_set_log_level(logs, NULL);
    lwsl_user("WebSocket security: http://10.168.191.245:8666\n");
    printf("Enter Ctrl + C for exit.\n");
    memset(&info, 0, sizeof info);
    info.port = 8666;
    info.mounts = NULL;
    info.protocols = protocols;
    info.vhost_name = argv[1]; //"10.168.191.245"; // argv[1]
    info.ws_ping_pong_interval = 10;
    info.options = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    lwsl_user("Server using TLS\n");
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.ssl_cert_filepath = "./certificate/danil_petrov.cert";
    info.ssl_private_key_filepath = "./certificate/danil_petrov.key";
    
    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("lws init failed\n");
        return 1;
    }
    
    while (n >= 0 && !interrupted)
        n = lws_service(context, 0);
    
    lws_context_destroy(context);
    
    return 0;
}


