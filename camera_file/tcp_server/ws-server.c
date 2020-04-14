#include "h264_camera.h"
#include <libwebsockets.h>

static int interrupted;

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
    uint8_t index_list = p_a->index_list;
    uint8_t i = 0;
    int fd_count = 0;
    int len;
    char* err = NULL;
    rtp_init(p_a);
    do {
        err = "Error in Dtls_connection init";
        if (dtls_connection_init(p_a) < 0)
            break;
        
        err = "Error in Dtls";
        if (dtls(p_a, NULL, 0) <= 0)
            break;
        
        while(i < 2)
        {
            err = "Error in ReadDtls";
            if((len = readDTLS(p_a->socket_stream, dtls_answers)) < 0)
                break; 
            
            err = "Error in Dtls";
            if (dtls(p_a, dtls_answers, len) <= 0)
                break;
            
            memset(dtls_answers, 0, DTLS_MESSAGES);
            err = NULL;
            i++;
        }        
        if (err)
            break;

        err = "Error in play_to_camera";
        if (play_to_camera(p_a))
            break;
        p_a->flags |= FLAG_TEARDOWN;
        err = "Error in create Sockaddr address_camera_rtp";
        if (createSockaddr(&address_camera_rtp, NULL, p_a->port_camera, &p_a->socket_rtp_fd) != 0) // Open port on server for rtp 43700
            break;

        err = "Error in create Sockaddr address_camera_rtcp";
        if (createSockaddr(&address_camera_rtcp, NULL, p_a->port_camera + 1, &p_a->socket_rtcp_fd) != 0) // Open port on server for rtcp 43701
            break;

        err = NULL;
    } while(0);
    if (err)
    {
        printf("DONE\n");
        teardown_to_camera(p_a);
        free_all(p_a);
        return 0;
    }
    
    
    setSockaddr(&message_to_browser, (unsigned char *)p_a->ip_browser, p_a->port_ice_browser);    // Info address browser
    setSockaddr(&address_to_camera_rtcp, (unsigned char *)p_a->ip_camera, p_a->port_rtcp_camera); // Info address camera with rtcp port

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
    while (list[index_list])
    {

        err = "Error in Poll";
        if (poll(fds, fd_count, 1) < 0)
            break;
        
        if ((fds[0].revents & POLLIN) != 0)
        {
            time(&finish);
            if ((int)difftime(finish, start) >= 58)
            {
                parameters_to_camera(p_a);
                start = finish;
            }
            unsigned char rtp_sps[210] = {0};
            int l = recv(p_a->socket_rtp_fd, mes, 1500, 0);
            int sps_size = 0;
            sps_size = rtp_to_srtp(p_a, mes, rtp_sps, &l);
            if (sps_size != 0)
                sps_size = sendto(p_a->socket_stream, (char *)rtp_sps, sps_size, 0, (struct sockaddr *)&message_to_browser, sizeof(message_to_browser));
            err = "Error with send sps";
            if (sps_size == -1)
                break;
            err = "Error with send rtp packets";
            l = sendto(p_a->socket_stream, (char *)mes, l, 0, (struct sockaddr *)&message_to_browser, sizeof(message_to_browser));
            if (l == -1)
                break;
            err = NULL;
        }
        if ((fds[1].revents & POLLIN) != 0)
        {
            time(&finish);
            if ((int)difftime(finish, start) >= 58)
            {
                parameters_to_camera(p_a);
                start = finish;
            }
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            unsigned short protocol = 0;
            int l = recvfrom(p_a->socket_stream, mes, 1500, 0, (struct sockaddr *)&addr, &addr_len);
            if (strcmp(p_a->ip_browser, inet_ntoa(addr.sin_addr)) != 0)
                continue;
            protocol = mes[0];
            switch (protocol)
            {
            case 0x81:
                rtcp_savp_to_avp(&p_a->crypto_rtcp, mes, &l);
                l = sendto(p_a->socket_rtcp_fd, mes, l, 0, (struct sockaddr *)&address_to_camera_rtcp, sizeof(address_to_camera_rtcp));
                break;
            case 0x00:
                stun_response(p_a, mes, l, &addr);
                break;
            }
        }
        if ((fds[2].revents & POLLIN) != 0)
        {
            time(&finish);
            if ((int)difftime(finish, start) >= 58)
            {
                parameters_to_camera(p_a);
                start = finish;
            }
            unsigned char rtp_sps[210] = {0};
            int l = recv(p_a->socket_rtcp_fd, mes, 1500, 0);
            rtcp_avp_to_savp(&p_a->crypto_from_camera, mes, &l, &p_a->index_rtcp);
            l = sendto(p_a->socket_stream, (char *)mes, l, 0, (struct sockaddr *)&message_to_browser, sizeof(message_to_browser));
            err = "Error with send srtcp"; 
            if (l == -1)
                break;
            err = NULL;
        }
    }
    if (err)
        printf("Error--------> %s\n", err);
    printf("DONE\n");
    teardown_to_camera(p_a);
    free_all(p_a);
    return 0;
}

/**
 * callback_dump_increment is callback function which swicth different events with websocket server.
 * @param wsi is pointer on struct lws for send message clients.
 * @param reason is enum for switch different events.
 * @param in is pointer on text buffer with answer from clients.
 * @param len is length text every answer.
 * @return 0 is successful.
*/

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
    {
        printf("Connection established\n");
        int index;
        for (index = 0; index < MAX_CLIENT; index++)
            if (list[index] != true)
                break;

        if (index == MAX_CLIENT)
        {
            sendSocketMessage(wsi, "Error with limit connection", MAX_CLIENT);
            break;
        }
        list[index] = true;
        pthreads[index] = (struct pthread_arguments *)calloc(1, sizeof(struct pthread_arguments)); /// Create struct for settings
        if (pthreads[index] == NULL)
            sendSocketMessage(wsi, "Error with arg_pthread.", index);
        else
            sendSocketMessage(wsi, "Connect.", index);
        pthreads[index]->flags = 0;
        pthreads[index]->index_list = index;
        printf("Connection\n");
        break;
    }
    case LWS_CALLBACK_RECEIVE:
    {
        int index_list = atoi((char *)in); // index in system 0 .. MAX_CLIENT
        struct pthread_arguments *arg_pthread = pthreads[index_list];
        if (list[index_list] == false)
            break;
        char* err = NULL;
        
        /* All commands which server can received from clients   
         * HOST - client send host camera`s  
         * SDP - client send sdp for server
         * ICE - client send ice for server
         * ERROR - client send error which was in client 
         */
        char* command = strstr((const char*)in, ":") + 1; /// Command from clients
        if (strncmp(command, "HOST", strlen("HOST")) == 0) /// Server receive host from camera
        {
            do
            {
                uint16_t size = len - (command - (char*)in) - strlen("HOST:");
                strncpy(arg_pthread->ip_camera, (char *)command + strlen("HOST:"), size);
                arg_pthread->ip_camera[size] = '\0';
                init_ports(&arg_pthread->port_ice, &arg_pthread->port_camera, index_list); // Initial ports for camera and ice
                strcpy(arg_pthread->ip_server, ip_server_program); // Save ip address servers
                err = "Error with connect to camera";
                if (connect_camera(arg_pthread))
                    break; // After Error in function with camera, free pointer of struct
                
                err = "Error with option";
                if (option_to_camera(arg_pthread))
                    break; // After Error in function with camera, free pointer of struct

                err = "Error with describe";
                if (describe_to_camera(arg_pthread))
                    break;
                
                printf("%s\n", arg_pthread->sdp_camera);
                sendSocketMessage(wsi, "OK", index_list);
                err = NULL;
            } while (0);
            
            // Error in function with camera
            if (err)
            {
                list[index_list] = false;
                printf("%s", err);
                sendSocketMessage(wsi, err, index_list);
                free_all(arg_pthread);
            }
            break;
        }
        
        if (strncmp((char *)command, "SDP", strlen("SDP")) == 0) /// Server receive SDP offer from browser and browser wait answer
        {
            printf("SDP Offer\n");
            uint16_t size = len - (command - (char*)in) - strlen("SDP:");
            memset(arg_pthread->sdp_offer, 0, sizeof(arg_pthread->sdp_offer));
            strncpy(arg_pthread->sdp_offer, (char *)command + strlen("SDP:"), size);
            arg_pthread->sdp_offer[size] = '\0';
            printf("%s\n", arg_pthread->sdp_offer);
            do
            {
                err = "Error with setup.";    
                if (setup_to_camera(arg_pthread)) 
                    break;
                err = "Error with ice-parse tag in sdp.";
                if (pwdParse(arg_pthread))
                    break;
                err = NULL;
            } while (0);
            // Error in function with camera
            if (err)
            {
                list[index_list] = false;
                sendSocketMessage(wsi, err, index_list);
                free_all(arg_pthread);
            }
            arg_pthread->flags |= FLAG_SDP;
            if ((arg_pthread->flags & FLAG_SDP) && (arg_pthread->flags & FLAG_ICE))
            {
                if (send_Stun_Sdp_Ice(arg_pthread, index_list, wsi))
                    list[index_list] = false;        
            }
            break;
        }
        
        if (strncmp((char *)command, "ICE", strlen("ICE")) == 0)
        {
            uint16_t size = len - (command - (char*)in) - strlen("ICE:");
            memset(arg_pthread->ice_browser, 0, sizeof(arg_pthread->ice_browser));
            strncpy(arg_pthread->ice_browser, (char *)command + strlen("ICE:"), size);
            arg_pthread->ice_browser[size] = '\0';
            printf("ICE: \n%s\n", arg_pthread->ice_browser);
            iceParse(arg_pthread);
            arg_pthread->flags |= FLAG_ICE;
            if ((arg_pthread->flags & FLAG_SDP) && (arg_pthread->flags & FLAG_ICE))
            {
                if (send_Stun_Sdp_Ice(arg_pthread, index_list, wsi))
                    list[index_list] = false;        
            }
            break;
        }
        if (strncmp((char *)command, "CLOSE", strlen("CLOSE")) == 0)
        {
            list[index_list] = false;
            pthread_join(tchilds[index_list], NULL);
            break;
        }
    }                         // LWS_CALLBACK_RECEIVE
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
    for (size_t index = 0; index < MAX_CLIENT; index++)
    {
        if (list[index] == true)
        {
            if (pthreads[index]->flags & FLAG_TEARDOWN)
            {
                printf("SignInt join\n");
                list[index] = false;
                pthread_join(tchilds[index], NULL);
                printf("End join\n");
            }
            else
            {
                printf("SignInt free\n");
                list[index] = false;
                free_all(pthreads[index]);    
            }
        }
    }
    interrupted = 1;
    exit(1);
}
void handler_sigsegv(int signum)
{
    printf("My Segmentation fault\n");
    for (size_t index = 0; index < MAX_CLIENT; index++)
    {
        if (list[index] == true)
        {
            if (pthreads[index]->flags & FLAG_TEARDOWN)
            {
                printf("SignInt join\n");
                list[index] = false;
                pthread_join(tchilds[index], NULL);
                printf("End join\n");
            }
            else
            {
                printf("SignInt free\n");
                list[index] = false;
                free_all(pthreads[index]);    
            }
        }
    }       
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
