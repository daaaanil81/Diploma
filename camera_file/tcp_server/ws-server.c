#include "h264_camera.h"
#include <libwebsockets.h>

static int interrupted;
static int index_arr;
unsigned char buf[4450] = {0};
bool flag_teardown = true;
bool busy = false;
static struct pthread_arguments *arg_pthread = NULL; /// Arguments for settings connection
static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    return 0;
}
void *udp_stream(void *arg)
{
    struct pthread_arguments *p_a = (struct pthread_arguments *)arg;
    unsigned char dtls_answers[1024] = {0};
    struct sockaddr_in message_to_browser;
    unsigned char mes[1500] = {0};
    struct sockaddr_in address_camera_rtp;
    struct sockaddr_in address_camera_rtcp;
    struct sockaddr_in address_to_camera_rtcp;

    struct pollfd fds[3];
    time_t start = 0, finish;
    unsigned int i = 0;
    int fd_count = 0;
    int len;
    rtp_init(p_a);
    if (dtls_connection_init(p_a) < 0)
    {
        printf("%s:%d:Dtls_connection init: error\n", __func__, __LINE__);
        goto done;
    }
    if (dtls(p_a, NULL, 0) <= 0)
    {
        printf("%s:%d:Dtls: error\n", __func__, __LINE__);
        goto done;
    }
    len = readDTLS(p_a->socket_stream, dtls_answers);
    if (len < 0)
    {
        printf("%s:%d:ReadDtls: error\n", __func__, __LINE__);
        goto done;
    }
    if (dtls(p_a, dtls_answers, len) <= 0)
    {
        printf("%s:%d:Dtls: error\n", __func__, __LINE__);
        goto done;
    }
    memset(dtls_answers, 0, DTLS_MESSAGES);
    len = readDTLS(p_a->socket_stream, dtls_answers);
    if (len < 0)
    {
        printf("%s:%d:ReadDtls: error\n", __func__, __LINE__);
        goto done;
    }
    if (dtls(p_a, dtls_answers, len) <= 0)
    {
        printf("%s:%d:Dtls: error\n", __func__, __LINE__);
        goto done;
    }
    if (play_to_camera(p_a->camerafd, p_a->ip_camera, p_a->session, &p_a->qSec) != 0)
    {
        printf("%s:%d:play_to_camera\n", __func__, __LINE__);
        goto done;
    }
    flag_teardown = false;
    printf("Was send play on camera\n");
    
    if(createSockaddr(&address_camera_rtp, NULL, p_a->port_camera, &p_a->socket_rtp_fd)!= 0) // Open port on server for rtp 43700
        return 0;
    if(createSockaddr(&address_camera_rtcp, NULL, p_a->port_camera+1, &p_a->socket_rtcp_fd)!= 0) // Open port on server for rtcp 43701
        return 0;
    setSockaddr(&message_to_browser, (unsigned char* )p_a->ip_browser, p_a->port_ice_browser); // Info address browser 
    setSockaddr(&address_to_camera_rtcp, (unsigned char* )p_a->ip_camera, p_a->port_rtcp_camera); // Info address camera with rtcp port 


    /* First struct with descriptor for rtp from camera */
    fds[0].fd = p_a->socket_rtp_fd;
    fds[0].events = POLLIN;
    /* Second struct with descriptor for connection with browser(stun,srtp,srtcp) */
    fds[1].fd = p_a->socket_stream; 
    fds[1].events = POLLIN;
    /* Second struct with descriptor for rtсp from camera */
    fds[2].fd = p_a->socket_rtcp_fd; 
    fds[2].events = POLLIN;
    
    fd_count = 3;
    time(&start);
    while (busy)
    {
        if (poll(fds, fd_count, 1) < 0)
        {
            perror("Poll");
            return 0;
        }
        if (fds[0].revents & POLLIN != 0)
        {
            time(&finish);
            if((int)difftime(finish, start) >= 58)
            {
                //option_to_camera(p_a->camerafd, p_a->ip_camera, &p_a->qSec);
                parameters_to_camera(p_a->camerafd, p_a->ip_camera, p_a->session, &p_a->qSec);
                start = finish;
            }
            unsigned char rtp_sps[210] = {0};
            int l = recv(p_a->socket_rtp_fd, mes, 1500, 0);
            int sps_size = 0;
            sps_size = rtp_to_srtp(p_a, mes, rtp_sps, &l);
            if (sps_size != 0)
                sps_size = sendto(p_a->socket_stream, (char *)rtp_sps, sps_size, 0, (struct sockaddr *)&message_to_browser, sizeof(message_to_browser));
            if (sps_size == -1)
                break;
            l = sendto(p_a->socket_stream, (char *)mes, l, 0, (struct sockaddr *)&message_to_browser, sizeof(message_to_browser));
            if (l == -1)
                break;
            //printf("Camera -> RTP -> Browser\n");
        }
        if (fds[1].revents & POLLIN != 0)
        {
            time(&finish);
            if((int)difftime(finish, start) >= 58)
            {
                //option_to_camera(p_a->camerafd, p_a->ip_camera, &p_a->qSec);
                parameters_to_camera(p_a->camerafd, p_a->ip_camera, p_a->session, &p_a->qSec);
                start = finish;
            }
            struct sockaddr_in addr;
            int addr_len = sizeof(addr);
            unsigned short protocol = 0;
            int l = recvfrom(p_a->socket_stream, mes, 1500, 0, (struct sockaddr*)&addr, &addr_len);
            if  (strcmp(p_a->ip_browser, inet_ntoa(addr.sin_addr)) != 0)
                continue;
            protocol = mes[0];
            switch (protocol)
            {
            case 0x81:
                rtcp_savp_to_avp(&p_a->crypto_rtcp, mes, &l);
                l = sendto(p_a->socket_rtcp_fd, mes, l, 0, (struct sockaddr*)&address_to_camera_rtcp, sizeof(address_to_camera_rtcp));
                // printf("Camera <- RTCP <- Browser\n");
                break;
            case 0x00:
                stun_response(p_a, mes, l, &addr);
                // printf("Camera <- STUN <- Browser\n");
                break;
            }
        }
        if (fds[2].revents & POLLIN != 0)
        {
            time(&finish);
            if((int)difftime(finish, start) >= 58)
            {
                //option_to_camera(p_a->camerafd, p_a->ip_camera, &p_a->qSec);
                parameters_to_camera(p_a->camerafd, p_a->ip_camera, p_a->session, &p_a->qSec);
                start = finish;
            }
            unsigned char rtp_sps[210] = {0};
            int l = recv(p_a->socket_rtcp_fd, mes, 1500, 0);
            rtcp_avp_to_savp(&p_a->crypto_from_camera, mes, &l, &p_a->index_rtcp);
            l = sendto(p_a->socket_stream, (char *)mes, l, 0, (struct sockaddr *)&message_to_browser, sizeof(message_to_browser));
            if (l == -1)
                break;
            // printf("Camera -> RTCP -> Browser\n");
        }
    }

done:
    printf("DONE\n");
    if (!flag_teardown)
        teardown_to_camera(p_a->camerafd, p_a->ip_camera, p_a->session, &p_a->qSec);
    flag_teardown = true;
    if (p_a->socket_rtp_fd >= 0)
    {
        close(p_a->socket_rtp_fd);
        p_a->socket_rtp_fd = -1;
    }
    if (p_a->socket_rtcp_fd >= 0)
    {
        close(p_a->socket_rtcp_fd);
        p_a->socket_rtcp_fd = -1;
    }
    if (p_a->socket_stream >= 0)
    {
        close(p_a->socket_stream);
        p_a->socket_stream = -1;
    }
    free_all(arg_pthread);
    return 0;
}
static int callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    switch (reason)
    {
    case LWS_CALLBACK_PROTOCOL_INIT: /// Initial protocols
        printf("Initialization\n");
        dtls_init();
        crypto_init_main();
        break;
    case LWS_CALLBACK_ESTABLISHED: /// Connection clients
        printf("Connection established\n");
        if (busy)
        {
            busy = false;
            ice_step = sdp_step = false;
            if (!flag_teardown)
            {
                teardown_to_camera(arg_pthread->camerafd, arg_pthread->ip_camera, arg_pthread->session, &arg_pthread->qSec);
                flag_teardown = true;
            }
            free_all(arg_pthread);
        }
        memset(buf, 0, sizeof(buf));
        arg_pthread = (struct pthread_arguments *)calloc(1, sizeof(struct pthread_arguments)); /// Create struct for settings
        printf("Create Struct\n");
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
            arg_pthread->index = -1;
            for (size_t m = 0; m < 5; m++)
            {
                if (list[m] != true)
                {
                    arg_pthread->index = m;
                    list[m] = true;
                    break;
                }
            }
            if (arg_pthread->index < 0)
            {
                memcpy((char *)buf + LWS_PRE, "Error with limit connection", strlen("Error with limit connection"));
                lws_write(wsi, &buf[LWS_PRE], strlen("Error with limit connection"), LWS_WRITE_TEXT);
                busy = false;
                break;
            }
            printf("Connect\n");
        }
    case LWS_CALLBACK_RECEIVE:
        memset(buf, 0, sizeof(buf));
        strncpy((char *)buf, (char *)in, len);

        if (strcmp((char *)buf, "Connect") == 0) /// Repiet request on connection
        {
            printf("Connect\n");
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
                    arg_pthread->index = -1;
                    for (size_t m = 0; m < 5; m++)
                    {
                        if (list[m] != true)
                        {
                            arg_pthread->index = m;
                            list[m] = true;
                            break;
                        }
                    }
                    if (arg_pthread->index < 0)
                    {
                        memcpy((char *)buf + LWS_PRE, "Error with limit connection", strlen("Error with limit connection"));
                        lws_write(wsi, &buf[LWS_PRE], strlen("Error with limit connection"), LWS_WRITE_TEXT);
                        busy = false;
                        break;
                    }
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
            arg_pthread->port_ice = port_ice_start;
            arg_pthread->port_camera = port_camera_start;

            if (connect_camera(&arg_pthread->sddr,&arg_pthread->camerafd, arg_pthread->ip_camera, &arg_pthread->qSec) != 0)
            {
                memcpy((char *)buf + LWS_PRE, "Error with connect to camera", strlen("Error with connect to camera"));
                lws_write(wsi, &buf[LWS_PRE], strlen("Error with connect to camera"), LWS_WRITE_TEXT);
                busy = false;
                free(arg_pthread);
                break;
            }
            else
            {
                memcpy((char *)buf + LWS_PRE, "OK", strlen("OK")); /// Send OK, camera with status "Work"
                lws_write(wsi, &buf[LWS_PRE], strlen("OK"), LWS_WRITE_TEXT);
                printf("Next steps\n\n\n");
                if (option_to_camera(arg_pthread->camerafd, arg_pthread->ip_camera, &arg_pthread->qSec) != 0)
                {
                    printf("Error with option\n");
                    busy = false;
                    memcpy((char *)buf + LWS_PRE, "Error with option", strlen("Error with option")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with option"), LWS_WRITE_TEXT);
                    free(arg_pthread);
                    break;
                }
                if (describe_to_camera(arg_pthread->camerafd, arg_pthread->ip_camera, (char *)buf, &arg_pthread->qSec) != 0)
                {
                    printf("Error with describe\n");
                    busy = false;
                    free(arg_pthread);
                    memcpy((char *)buf + LWS_PRE, "Error with describe", strlen("Error with describe")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with describe"), LWS_WRITE_TEXT);
                    break;
                }
                strcpy(arg_pthread->sdp_camera, strstr((char *)buf, "v="));
            }
        }
        if (strncmp((char *)buf, "SDP", strlen("SDP")) == 0) /// Server receive SDP offer from browser and browser wait answer
        {
            printf("SDP Offer\n");
            memset(arg_pthread->sdp_offer, 0, sizeof(arg_pthread->sdp_offer));
            strcpy(arg_pthread->sdp_offer, (char *)buf + strlen("SDP"));
            if (setup_to_camera(arg_pthread->camerafd, arg_pthread->ip_camera, arg_pthread->port_camera, arg_pthread->session, &arg_pthread->port_rtcp_camera, &arg_pthread->qSec) != 0)
            {
                printf("Error with setup\n");
                busy = false;
                free(arg_pthread);
                memcpy((char *)buf + LWS_PRE, "Error with setup", strlen("Error with setup")); /// Send OK, camera with status "Work"
                lws_write(wsi, &buf[LWS_PRE], strlen("Error with setup"), LWS_WRITE_TEXT);
                break;
            }
            pwdParse(arg_pthread);
            sdp_step = true;          /// Marker, sdp step was finished
            if (ice_step && sdp_step) /// was finished all step
            {
                printf("Create Threads SDP\n");
                strcpy(arg_pthread->ip_server, ip_server_program);
                if (create_ice(arg_pthread) != 0)
                {
                    printf("Error with create ice\n");
                    busy = false;
                    free_all(arg_pthread);
                    memcpy((char *)buf + LWS_PRE, "Error with create ice", strlen("Error with create ice")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with create ice"), LWS_WRITE_TEXT);
                    break;
                }
                if (stun_request(arg_pthread) != 0)
                {
                    printf("Error with stun request\n");
                    busy = false;
                    free_all(arg_pthread);
                    memcpy((char *)buf + LWS_PRE, "Error with stun request", strlen("Error with stun request")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with stun request"), LWS_WRITE_TEXT);
                    break;
                }
                if (sdpParse(arg_pthread) != 0)
                {
                    printf("Error with sdp parse\n");
                    busy = false;
                    free_all(arg_pthread);
                    memcpy((char *)buf + LWS_PRE, "Error with sdp parse", strlen("Error with sdp parse")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with sdp parse"), LWS_WRITE_TEXT);
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
                if (stun_response(arg_pthread, NULL, 0, NULL) != 0)
                {
                    printf("Error with stun response\n");
                    busy = false;
                    free_all(arg_pthread);
                    memcpy((char *)buf + LWS_PRE, "Error with stun response", strlen("Error with stun response")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with stun response"), LWS_WRITE_TEXT);
                    break;
                }

                if (pthread_create(&tchilds[arg_pthread->index], 0, udp_stream, arg_pthread) < 0)
                {
                    perror("Can't create thread!");
                    free_all(arg_pthread);
                }
                // pthread_join(tchilds[arg_pthread->index], 0);
                // if (busy)
                //     free_all(arg_pthread);
                pthread_detach(tchilds[arg_pthread->index]);
                //busy = false;
                printf("After join\n");
            }
        }
        if (strncmp((char *)buf, "ICE", strlen("ICE")) == 0)
        {
            strcpy(arg_pthread->ice_browser, (char *)buf + strlen("ICE"));
            printf("ICE: \n\n%s\n", arg_pthread->ice_browser);
            iceParse(arg_pthread);
            ice_step = true; /// Marker, ice step was finished
            if (ice_step && sdp_step)
            {
                printf("Create Threads ICE\n");
                strcpy(arg_pthread->ip_server, ip_server_program);
                printf("Before create ice\n");
                if (create_ice(arg_pthread) != 0)
                {
                    printf("Error with create ice\n");
                    busy = false;
                    free_all(arg_pthread);
                    memcpy((char *)buf + LWS_PRE, "Error with create ice", strlen("Error with create ice")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with create ice"), LWS_WRITE_TEXT);
                    break;
                }
                printf("Before stun\n");
                if (stun_request(arg_pthread) != 0)
                {
                    printf("Error with stun request\n");
                    busy = false;
                    free_all(arg_pthread);
                    memcpy((char *)buf + LWS_PRE, "Error with stun request", strlen("Error with stun request")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with stun request"), LWS_WRITE_TEXT);
                    break;
                }
                printf("Before sdpParse\n");
                if (sdpParse(arg_pthread) < 0)
                {
                    printf("Error with sdp parse\n");
                    busy = false;
                    free_all(arg_pthread);
                    memcpy((char *)buf + LWS_PRE, "Error with sdp parse", strlen("Error with sdp parse")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with sdp parse"), LWS_WRITE_TEXT);
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
                if (stun_response(arg_pthread, NULL, 0, NULL) != 0)
                {
                    printf("Error with stun response\n");
                    busy = false;
                    free_all(arg_pthread);
                    memcpy((char *)buf + LWS_PRE, "Error with stun response", strlen("Error with stun response")); /// Send OK, camera with status "Work"
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with stun response"), LWS_WRITE_TEXT);
                    break;
                }
                if (pthread_create(&tchilds[arg_pthread->index], 0, udp_stream, arg_pthread) < 0)
                {
                    perror("Can't create thread!");
                    free_all(arg_pthread);
                }
                // pthread_join(tchilds[arg_pthread->index], 0);
                // if (busy)
                //     free_all(arg_pthread);
                pthread_detach(tchilds[arg_pthread->index]);
                //busy = false;
                printf("After join\n");
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
    if (busy)
    {
        busy = false;
        if (!flag_teardown)
        {
            teardown_to_camera(arg_pthread->camerafd, arg_pthread->ip_camera, arg_pthread->session, &arg_pthread->qSec);
            flag_teardown = true;
        }
        free_all(arg_pthread);
    }
    exit(1);
}
void handler_sigsegv(int signum)
{
    printf("My Segmentation fault\n");
    interrupted = 1;
    if (busy)
    {
        busy = false;
        if (!flag_teardown)
        {
            teardown_to_camera(arg_pthread->camerafd, arg_pthread->ip_camera, arg_pthread->session, &arg_pthread->qSec);
            flag_teardown = true;
        }
        free_all(arg_pthread);
    }
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
    if (argv[1] == NULL)
    {
        printf("Please enter ip address\n");
        return 0;
    }
    strcpy(ip_server_program, argv[1]);
    lws_set_log_level(logs, NULL);
    lwsl_user("WebSocket security: http://%s:9999\n", ip_server_program);
    printf("Enter Ctrl + C for exit.\n");
    memset(&info, 0, sizeof info);
    info.port = 9999; /// Server port
    info.mounts = NULL;
    info.protocols = protocols;
    info.vhost_name = ip_server_program; /// Server ip
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
