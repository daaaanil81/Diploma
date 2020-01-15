#include "h264_camera.h"

static int interrupted;
static int index_arr;
unsigned char buf[4100] = {0};
char timing[4300] = {0};
static struct pthread_arguments *arg_pthread = NULL; /// Arguments for settings connection
void *udp_stream(void *arg)
{
    int socket_rtp;
    int socket_rtcp;

    int socket_rtcp_to_rtpengine;
    int socket_rtp_to_rtpengine;

    struct sockaddr_in servaddr;
    struct sockaddr_in addr_to_rtpengine;
    struct sockaddr_in addr_from_rtpengine;

    struct sockaddr_in addr_rtcp;
    struct sockaddr_in addr_rtcp_from_rtpengine;
    struct sockaddr_in addr_rtcp_to_rtpengine;

    struct sockaddr_in addr_rtcp_camera;
    char buf_rtp[1510] = {0};
    char buf_rtcp[200] = {0};
    int i = 0;
    short fd_count = 3;
    struct pollfd fds[3];
    int n_rtp;
    int n_rtcp;  
    unsigned int sequnce_number = 1;
    unsigned int sequnce_number_origin = 0;
    struct pthread_arguments *p_a = (struct pthread_arguments *)arg;

    printf("Udp stream create.\n");

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(p_a->port_camera);

    memset(&addr_rtcp, 0, sizeof(addr_rtcp));
    addr_rtcp.sin_family = AF_INET; // IPv4
    addr_rtcp.sin_addr.s_addr = INADDR_ANY;
    addr_rtcp.sin_port = htons(p_a->port_camera + 1);

    memset(&addr_rtcp_camera, 0, sizeof(addr_rtcp_camera));
    addr_rtcp_camera.sin_family = AF_INET; // IPv4
    addr_rtcp_camera.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, p_a->ip_camera, &addr_rtcp_camera.sin_addr);
    addr_rtcp_camera.sin_port = htons(p_a->port_udp_camera); ////////////////////////////////////////////////////

    if ((socket_rtcp = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket rtcp");
        return 0;
    }
    if ((socket_rtp = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket udp");
        return 0;
    }
    if (bind(socket_rtp, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed rtp");
        return 0;
    }
    if (bind(socket_rtcp, (const struct sockaddr *)&addr_rtcp, sizeof(addr_rtcp)) < 0)
    {
        perror("bind failed rtcp");
        return 0;
    }
    
    if ( play_to_camera(p_a->sddr, p_a->camerafd, p_a->ip_camera, p_a->session) != 0)
    {
        perror("play_to_camera");
        return 0;
    }
    //////////////////////////////////////////////////////////////////////////////

    if ((socket_rtp_to_rtpengine = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket rtp to rtpengine");
        return 0;
    }

    //Address for rtp from rtpengine
    memset(&addr_from_rtpengine, 0, sizeof(addr_from_rtpengine));
    addr_from_rtpengine.sin_family = AF_INET; // IPv4
    inet_pton(AF_INET, p_a->ip_server, &addr_from_rtpengine.sin_addr);
    addr_from_rtpengine.sin_port = htons(p_a->port_from_rtpengine); // Port = 53532

    //Address for rtp to rtpengine
    memset(&addr_to_rtpengine, 0, sizeof(addr_to_rtpengine));
    addr_to_rtpengine.sin_family = AF_INET; // IPv4
    inet_pton(AF_INET, p_a->ip_server, &addr_to_rtpengine.sin_addr);
    addr_to_rtpengine.sin_port = htons(p_a->port_to_rtpengine); // Port = 12302

    //Address for rtcp to rtpengine
    if ((socket_rtcp_to_rtpengine = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket rtcp to rtpengine");
        return 0;
    }

    memset(&addr_rtcp_to_rtpengine, 0, sizeof(addr_rtcp_to_rtpengine));
    addr_rtcp_to_rtpengine.sin_family = AF_INET; // IPv4
    inet_pton(AF_INET, p_a->ip_server, &addr_rtcp_to_rtpengine.sin_addr);
    addr_rtcp_to_rtpengine.sin_port = htons(p_a->port_to_rtpengine + 1); 

    memset(&addr_rtcp_from_rtpengine, 0, sizeof(addr_rtcp_from_rtpengine));
    addr_rtcp_from_rtpengine.sin_family = AF_INET; // IPv4
    //inet_pton(AF_INET, p_a->ip_server, &addr_rtcp_from_rtpengine.sin_addr);
    addr_rtcp_from_rtpengine.sin_addr.s_addr = INADDR_ANY;
    addr_rtcp_from_rtpengine.sin_port = htons(p_a->port_from_rtpengine + 1);
    if (bind(socket_rtp_to_rtpengine, (const struct sockaddr *)&addr_from_rtpengine, sizeof(addr_from_rtpengine)) < 0)
    {
        perror("bind failed");
        return 0;
    }
    if (bind(socket_rtcp_to_rtpengine, (const struct sockaddr *)&addr_rtcp_from_rtpengine, sizeof(addr_rtcp_from_rtpengine)) < 0)
    {
        perror("bind failed");
        return 0;
    }
    /* First struct with descriptor */
    fds[0].fd = socket_rtp;
    fds[0].events = POLLIN;
    /* Second struct with descriptor */
    fds[1].fd = socket_rtcp;
    fds[1].events = POLLIN;
    /* Thirt struct with descriptor */
    fds[2].fd = socket_rtcp_to_rtpengine;
    fds[2].events = POLLIN;
    
    while(1)
    {
        if (poll(fds, fd_count, 1) < 0)
        {
            perror("Poll");
            return 0;
        }
        if ((fds[0].revents & POLLIN) != 0)
        {
            bzero(buf_rtp, sizeof(buf_rtp));
            n_rtp = recvfrom(socket_rtp, (char *)buf_rtp, sizeof(buf_rtp), 0, NULL, 0); //resvfrom stream from camera
            unsigned char rtp_sps[200] = {0};
            unsigned int size_sps_packet;
            size_sps_packet = rtp_parse(buf_rtp, rtp_sps, &sequnce_number, &sequnce_number_origin, p_a);
            if(size_sps_packet)
            {
                sendto(socket_rtp_to_rtpengine, rtp_sps, size_sps_packet, 0, (struct sockaddr *)&addr_to_rtpengine, sizeof(addr_to_rtpengine));
                printf("SPS was send = %d\n", size_sps_packet);
            }
            n_rtp = sendto(socket_rtp_to_rtpengine, buf_rtp, n_rtp, 0, (struct sockaddr *)&addr_to_rtpengine, sizeof(addr_to_rtpengine));
            printf("CAMERA->RTP->RTPengine\n");
            printf("\n");
            i++;
        } 
        if ((fds[1].revents & POLLIN) != 0)
        {
            bzero(buf_rtcp, sizeof(buf_rtcp));
            n_rtcp = recvfrom(socket_rtcp, (char *)buf_rtcp, sizeof(buf_rtcp), 0, NULL, 0); //recvfrom rtcp from camera
            n_rtcp = sendto(socket_rtcp_to_rtpengine, buf_rtcp, n_rtcp, 0, (struct sockaddr *)&addr_rtcp_to_rtpengine, sizeof(addr_rtcp_to_rtpengine));    
            printf("CAMERA->RTCP->RTPengine\n");
            printf("\n");
        }
        if ((fds[2].revents & POLLIN) != 0)
        {
            bzero(buf_rtcp, sizeof(buf_rtcp));
            n_rtcp = recvfrom(socket_rtcp_to_rtpengine, (char *)buf_rtcp, sizeof(buf_rtcp), 0, NULL, 0); //recvfrom rtcp from camera    
            n_rtcp = sendto(socket_rtcp, buf_rtcp, n_rtcp, 0, (struct sockaddr *)&addr_rtcp_camera, sizeof(addr_rtcp_camera));
            printf("CAMERA<-RTCP<-RTPengine\n");
            printf("\n");

        }
        if (interrupted == 1)
        {
            break;
        }
    }
    
    //TEARDOWN to camera
    teardown_to_camera(p_a->camerafd, p_a->ip_camera, p_a->session);
    close(socket_rtp);
    close(socket_rtcp);
    close(socket_rtcp_to_rtpengine);
    close(socket_rtp_to_rtpengine);
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
    switch (reason)
    {
    case LWS_CALLBACK_PROTOCOL_INIT: /// Initial protocols
        index_arr = 0;
        while (index_arr < SIZE_CAMERA)
        {
            afc[index_arr].port_ice = 0;
            afc[index_arr].port_camera = 0;
            afc[index_arr].ip[0] = '\0';
            index_arr++;
        }
        break;
    case LWS_CALLBACK_ESTABLISHED: /// Connection clients
        printf("connection established\n");
        if (busy)
        {
            pthread_cancel(arg_pthread->tchild);
            free(arg_pthread);
            busy = false;
        }
            memset(buf, 0, sizeof(buf));
            arg_pthread = (struct pthread_arguments *)calloc(1, sizeof(struct pthread_arguments)); /// Create struct for settings
            if (arg_pthread == NULL)
            {
                printf("Error with arg_pthread"); /// Error if problems with create object struct pthread_arguments
                memcpy((char *)buf + LWS_PRE, "Error", strlen("Error"));
                lws_write(wsi, &buf[LWS_PRE], strlen("Error"), LWS_WRITE_TEXT); /// Send Error into clients
            }
            else
            {
                memcpy((char *)buf + LWS_PRE, "Connect", strlen("Connect"));
                lws_write(wsi, &buf[LWS_PRE], strlen("Connect"), LWS_WRITE_TEXT); /// Send connect into clients
                busy = true;                                                      /// Server will work only one clients on time
                printf("Connect\n");
            }
    case LWS_CALLBACK_RECEIVE:
        memset(buf, 0, sizeof(buf));
        if (len <= 1)
        {
            break;
        }
        strncpy((char *)buf, (char *)in, len);

        if (strcmp((char *)buf, "Connect") == 0) /// Repiet request on connection
        {
            //printf("Connect\n");
            if (!busy)
            {
                memset(buf, 0, sizeof(buf));
                arg_pthread = (struct pthread_arguments *)calloc(1, sizeof(struct pthread_arguments));
                if (arg_pthread == NULL)
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
        if (strncmp((char *)buf, "host", strlen("host")) == 0)
        {
            strcpy(arg_pthread->ip_camera, (char *)buf + strlen("host"));
            memset(buf, 0, sizeof(buf));
            index_arr = 0;
            other_user = false;
            while (afc[index_arr].port_ice != 0 && index_arr < SIZE_CAMERA) /// Search stream in list
            {
                if (strncmp(afc[index_arr].ip, arg_pthread->ip_camera, strlen(arg_pthread->ip_camera)) == 0)
                {
                    other_user = true;
                    break;
                }
                index_arr++;
            }
            if (other_user) /// If stream from camera other user have already used
            {
                printf("Camera used other people.\n\n");
                memcpy((char *)buf + LWS_PRE, "Error: Camera used other people.", strlen("Error: Camera used other people."));
                lws_write(wsi, &buf[LWS_PRE], strlen("Error: Camera used other people."), LWS_WRITE_TEXT);
                busy = false;
                break;
            }
            if (index_arr == 0) /// If stream first on server
            {
                arg_pthread->port_ice = afc[index_arr].port_ice = port_ice_start;
                arg_pthread->port_camera = afc[index_arr].port_camera = port_camera_start;
                strcpy(afc[index_arr].ip, arg_pthread->ip_camera);
            }
            else /// If this stream no first on server
            {
                arg_pthread->port_ice = afc[index_arr].port_ice = afc[index_arr - 1].port_ice + 1000;
                arg_pthread->port_camera = afc[index_arr].port_camera = afc[index_arr - 1].port_camera + 1000;
                strcpy(afc[index_arr].ip, arg_pthread->ip_camera);
            }
            /** Connection on stream
                 *
                 *
                 *
                 */
            if (connect_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera) != 0)
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
                if (option_to_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera) != 0)
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
                if (describe_to_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera, (char *)buf) != 0)
                {
                    printf("Error with describe\n");
                    busy = false;
                    break;
                }
                strcpy(arg_pthread->sdp_camera, strstr((char *)buf, "v="));
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
        if (strncmp((char *)buf, "SDP", strlen("SDP")) == 0) /// Server receive SDP offer from browser and browser wait answer
        {
            printf("SDP Offer\n");
            memset(arg_pthread->sdp_offer, 0, sizeof(arg_pthread->sdp_offer));
            strcpy(timing, (char *)buf + strlen("SDP"));
            /** Setup into camera
                 *
                 *
                 *
            */
            if (setup_to_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera, arg_pthread->port_camera, arg_pthread->session, arg_pthread->port_udp_camera) != 0)
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
            if (ice_step && sdp_step) /// was finished all step
            {
                printf("Create Threads SDP\n");
                generationSDP_BROWSER(arg_pthread, timing);
                
                if (sendSDP_rtpengine(arg_pthread) != 0)
                {
                    printf("Error with sdp parse");
                    busy = false;
                    free(arg_pthread);
                    break;
                }

                printf("Send answer: \n");
                memset(buf, 0, sizeof(buf));
                sprintf((char *)buf + LWS_PRE, "%s%s", type_sdp, arg_pthread->sdp_answer);
                lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->sdp_answer) + strlen(type_sdp), LWS_WRITE_TEXT); /// Send answer into browser

                printf("Send Ice: \n");
                memset(buf, 0, sizeof(buf));
                sprintf((char *)buf + LWS_PRE, "%s%s", type_ice, arg_pthread->ice_server);
                lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->ice_server) + strlen(type_ice), LWS_WRITE_TEXT); /// Send ice candidate into browser
                busy = false;
                sleep(1);
                //free(arg_pthread);
                if (pthread_create(&arg_pthread->tchild, 0, udp_stream, arg_pthread) < 0)
                {
                    perror("Can't create thread!");
                }
                pthread_detach(arg_pthread->tchild);
            }
        }
        if (strncmp((char *)buf, "ICE", strlen("ICE")) == 0)
        {
            strcpy(arg_pthread->ice_browser, (char *)buf + strlen("ICE"));
            printf("ICE: \n\n%s\n", arg_pthread->ice_browser);
            //unsigned int port_ice_browser; /// Port ice dandidate browser
            //iceParse(p_a->ice_browser, p_a->ip_browser, p_a->ip_server, port_ice_browser, p_a->uflag_browser);
            iceParse(arg_pthread);

            ice_step = true; /// Marker, ice step was finished
            if (ice_step && sdp_step)
            {
                printf("Create Threads ICE\n");
                generationSDP_BROWSER(arg_pthread, timing);
                //printf("Offer: \n\n%s\n", arg_pthread->sdp_offer);
                
                if (sendSDP_rtpengine(arg_pthread) != 0)
                {
                    printf("Error with sdp parse");
                    busy = false;
                    free(arg_pthread);
                    break;
                }
                printf("Send answer: \n");
                memset(buf, 0, sizeof(buf));
                sprintf((char *)buf + LWS_PRE, "%s%s", type_sdp, arg_pthread->sdp_answer);
                lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->sdp_answer) + strlen(type_sdp), LWS_WRITE_TEXT); /// Send answer into browser

                printf("Send Ice: \n");
                memset(buf, 0, sizeof(buf));
                
                sprintf((char *)buf + LWS_PRE, "%s%s", type_ice, arg_pthread->ice_server);
                lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->ice_server) + strlen(type_ice), LWS_WRITE_TEXT); /// Send ice candidate into browser
                busy = false;
                sleep(1);
                if (pthread_create(&arg_pthread->tchild, 0, udp_stream, arg_pthread) < 0)
                {
                    perror("Can't create thread!");
                }
                pthread_detach(arg_pthread->tchild);
            }
        }
        break;
    case LWS_CALLBACK_CLOSED: /// Close server
        printf("Close\n");
        break;
    default:
        break;
    }

    return 0;
}
static struct lws_protocols protocols[] = {
    {"http", callback_http, 0, 0},
    {"lws-minimal", callback_dumb_increment, 0, 0, 0, NULL, 0},
    {NULL, NULL, 0, 0} /* terminator*/
};

void sigint_handler(int sig)
{
    printf("Int\n");
    interrupted = 1;
}
void handler_sigsegv(int signum)
{
    printf("My Segmentation fault\n");
    interrupted = 1;
    exit(1);
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
    if (!context)
    {
        lwsl_err("lws init failed\n");
        return 1;
    }

    while (n >= 0 && !interrupted)
        n = lws_service(context, 0);

    lws_context_destroy(context);

    return 0;
}
