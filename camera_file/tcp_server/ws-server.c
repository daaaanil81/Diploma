#include "dtls.h"
#include <libwebsockets.h>
static int interrupted;
static int index_arr;
unsigned char buf[4100] = {0};
char timing[4300] = {0};
static struct pthread_arguments *arg_pthread = NULL; /// Arguments for settings connection
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
                if (option_to_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera) != 0)
                {
                    printf("Error with option\n");
                    busy = false;
                    break;
                }
                if (describe_to_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera, (char *)buf) != 0)
                {
                    printf("Error with describe\n");
                    busy = false;
                    break;
                }
                strcpy(arg_pthread->sdp_camera, strstr((char *)buf, "v=")); 
                create_ice(arg_pthread);
                if (sdpParse(arg_pthread) < 0)
                {
                    printf("Error with sdp parse\n");
                    busy = false;
                    break;
                }

            }
        }
        if (strncmp((char *)buf, "SDP", strlen("SDP")) == 0) /// Server receive SDP offer from browser and browser wait answer
        {
            printf("SDP Offer\n");
            memset(arg_pthread->sdp_offer, 0, sizeof(arg_pthread->sdp_offer));
  
            strcpy(arg_pthread->sdp_offer, (char *)buf + strlen("SDP"));
            if (setup_to_camera(arg_pthread->sddr, arg_pthread->camerafd, arg_pthread->ip_camera, arg_pthread->port_camera, arg_pthread->session, arg_pthread->port_udp_camera) != 0)
            {
                printf("Error with setup\n");
                busy = false;
                break;
            }
            pwdParse(arg_pthread);
            sdp_step = true; /// Marker, sdp step was finished
            if (ice_step && sdp_step) /// was finished all step
            {
                printf("Create Threads SDP\n");
                generationSTUN(arg_pthread);
                dtls_fingerprint_free(arg_pthread);
                free(arg_pthread);
                /*printf("Send answer: \n");
                memset(buf, 0, sizeof(buf));
                sprintf((char *)buf + LWS_PRE, "%s%s", type_sdp, arg_pthread->sdp_answer);
                lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->sdp_answer) + strlen(type_sdp), LWS_WRITE_TEXT); /// Send answer into browser

                printf("Send Ice: \n");
                memset(buf, 0, sizeof(buf));
                sprintf((char *)buf + LWS_PRE, "%s%s", type_ice, arg_pthread->ice_server);
                lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->ice_server) + strlen(type_ice), LWS_WRITE_TEXT); /// Send ice candidate into browser
                */
                busy = false;
                sleep(1);
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
                generationSTUN(arg_pthread);
                dtls_fingerprint_free(arg_pthread);
                free(arg_pthread);
                //printf("Offer: \n\n%s\n", arg_pthread->sdp_offer);
                /*
                printf("Send answer: \n");
                memset(buf, 0, sizeof(buf));
                sprintf((char *)buf + LWS_PRE, "%s%s", type_sdp, arg_pthread->sdp_answer);
                lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->sdp_answer) + strlen(type_sdp), LWS_WRITE_TEXT); /// Send answer into browser

                printf("Send Ice: \n");
                memset(buf, 0, sizeof(buf));    
                sprintf((char *)buf + LWS_PRE, "%s%s", type_ice, arg_pthread->ice_server);
                lws_write(wsi, &buf[LWS_PRE], strlen(arg_pthread->ice_server) + strlen(type_ice), LWS_WRITE_TEXT); /// Send ice candidate into browser
                */

                busy = false;
                sleep(1);
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
    lwsl_user("WebSocket security: http://10.168.166.132:8666\n");
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
