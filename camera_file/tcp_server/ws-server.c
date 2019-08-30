#include "h264_camera.h"
static int interrupted;
static int index_arr;
unsigned char buf[1024] = {0};
bool busy = false;
bool other_user = false;
bool ice_step = false;
bool sdp_step = false;
struct lws *test;
struct pthread_arguments
{
    struct sockaddr_in sddr;
    char host[20];
    int camerafd;
    char sdp_offer[1024];
    char sdp_camera[1024];
    char sdp_answer[1024];
    unsigned int port_ice;
    unsigned int port_camera;
    char ice_browser[300];
    char ice_server[300];
    char session[20];
    char uflag_server[10];
};
static struct pthread_arguments *arg_phtread = NULL;
static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    return 0;
}
static int callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    switch (reason) {
        case LWS_CALLBACK_PROTOCOL_INIT:
            index_arr = 0;
            while(index_arr < SIZE_CAMERA)
            {
                afc[index_arr].port_ice = 0;
                afc[index_arr].port_camera = 0;
                afc[index_arr].ip[0] = '\0';
                index_arr++;
            }
            break;
        case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting
            printf("connection established\n");
            if(!busy)
            {
                memset(buf, 0, sizeof(buf));
                arg_phtread = (pthread_arguments*)calloc(1, sizeof(pthread_arguments));
                if(arg_phtread == NULL)
                {
                    printf("Error with arg_pthread");
                    memcpy((char *)buf + LWS_PRE, "Error", strlen("Error"));
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error"), LWS_WRITE_TEXT);
                }
                else
                {
                    memcpy((char *)buf + LWS_PRE, "Connect", strlen("Connect"));
                    lws_write(wsi, &buf[LWS_PRE], strlen("Connect"), LWS_WRITE_TEXT);
                    busy = true;
                    test = wsi;
                    printf("Connect\n");
                }
            }
            else
            {
                memset(buf, 0, sizeof(buf));
                memcpy((char *)buf + LWS_PRE, "Busy", strlen("Busy"));
                lws_write(wsi, &buf[LWS_PRE], strlen("Busy"), LWS_WRITE_TEXT);
            }
        case LWS_CALLBACK_RECEIVE:
            printf("RECEIVE\n");
            memset(buf, 0, sizeof(buf));
            if(len <= 1)
            {
                break;
            }
            strncpy((char*)buf, (char*)in, len);
            buf[len] = '\0';
            printf("RECEIVE:\n");
            
            if(strcmp((char*)buf, "Connect") == 0)
            {
                //printf("Connect\n");
                if(!busy)
                {
                    memset(buf, 0, sizeof(buf));
                    arg_phtread = (pthread_arguments*)calloc(1, sizeof(pthread_arguments));
                    if(arg_phtread == NULL)
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
            if(strncmp((char*)buf, "host", strlen("host")) == 0)
            {
                strcpy(arg_phtread->host, (char*)buf + strlen("host"));
                memset(buf, 0, sizeof(buf));
                index_arr = 0;
                while( afc[index_arr].port_ice != 0 && index_arr < SIZE_CAMERA )
                {
                    if(strncmp(afc[index_arr].ip, arg_phtread->host, strlen(arg_phtread->host)) == 0)
                    {
                        other_user = true;
                        break;
                    }
                    index_arr++;
                }
                if(other_user)
                {
                    printf("Camera used other people.\n\n");
                    memcpy((char *)buf + LWS_PRE, "Error: Camera used other people.", strlen("Error: Camera used other people."));
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error: Camera used other people."), LWS_WRITE_TEXT);
                    busy = false;
                    break;
                }
                if(index_arr == 0)
                {
                    arg_phtread->port_ice = afc[index_arr].port_ice = port_ice_start;
                    arg_phtread->port_camera = afc[index_arr].port_camera = port_camera_start;
                    strcpy(afc[index_arr].ip, arg_phtread->host);
                }
                else
                {
                    arg_phtread->port_ice = afc[index_arr].port_ice = afc[index_arr-1].port_ice+1000;
                    arg_phtread->port_camera = afc[index_arr].port_camera = afc[index_arr-1].port_camera+1000;
                    strcpy(afc[index_arr].ip, arg_phtread->host);
                }
                if(connect_camera(arg_phtread->sddr, arg_phtread->camerafd, arg_phtread->host) != 0)
                {
                    memcpy((char *)buf + LWS_PRE, "Error with connect to camera", strlen("Error with connect to camera"));
                    lws_write(wsi, &buf[LWS_PRE], strlen("Error with connect to camera"), LWS_WRITE_TEXT);
                    busy = false;
                    break;
                }
                else
                {
                    memcpy((char *)buf + LWS_PRE, "OK", strlen("OK"));
                    lws_write(test, &buf[LWS_PRE], strlen("OK"), LWS_WRITE_TEXT);
                    printf("Next steps\n\n\n");
                    if(option_to_camera(arg_phtread->sddr, arg_phtread->camerafd, arg_phtread->host) != 0)
                    {
                        printf("Error with option\n");
                        busy = false;
                        break;
                    }
                    if(describe_to_camera(arg_phtread->sddr, arg_phtread->camerafd, arg_phtread->host, (char*)buf) != 0)
                    {
                        printf("Error with describe\n");
                        busy = false;
                        break;
                    }
                    strcpy(arg_phtread->sdp_camera, strstr((char*)buf,"v="));
                    sdpParse(arg_phtread->sdp_camera, arg_phtread->uflag_server, arg_phtread->sdp_offer);
                    sprintf((char*)buf + LWS_PRE, "%s%s", type_sdp, arg_phtread->sdp_offer);
                    lws_write(wsi, &buf[LWS_PRE], strlen(arg_phtread->sdp_offer)+strlen(type_sdp), LWS_WRITE_TEXT);
                    create_ice(arg_phtread->ice_server, arg_phtread->port_ice);
                    sprintf((char*)buf + LWS_PRE, "%s%s", type_ice, arg_phtread->ice_server);
                    lws_write(wsi, &buf[LWS_PRE], strlen(arg_phtread->ice_server)+strlen(type_ice), LWS_WRITE_TEXT);
    
                }
            }
            if(strncmp((char*)buf, "SDP", strlen("SDP")) == 0)
            {
                strcpy(arg_phtread->sdp_answer, (char*)buf + strlen("SDP"));
                printf("Answer: \n\n%s\n", arg_phtread->sdp_answer);
                if(setup_to_camera(arg_phtread->sddr, arg_phtread->camerafd, arg_phtread->host, arg_phtread->port_camera, arg_phtread->session) != 0)
                {
                    printf("Error with setup\n");
                    busy = false;
                    break;
                }
                sdp_step = true;
                if(ice_step && sdp_step)
                {
                    printf("Create Threads\n");
                    //free(arg_phtread);
                }
            }
            if(strncmp((char*)buf, "ICE", strlen("ICE")) == 0)
            {
                strcpy(arg_phtread->ice_browser, (char*)buf + strlen("ICE"));
                printf("ICE: \n\n%s\n", arg_phtread->ice_browser);
                ice_step = true;
                if(ice_step && sdp_step)
                {
                    printf("Create Threads\n");
                    
                }
                
            }
            
            
            //lws_write(wsi, &buf[LWS_PRE], strlen("Busy"), LWS_WRITE_TEXT);
            break;
        default:
            break;
    }
    
    return 0;
}
static struct lws_protocols protocols[] = {
    { "http", callback_http, 0, 0 },
    { "lws-minimal", callback_dumb_increment, 0, 0, 0, NULL, 0 },
    { NULL, NULL, 0, 0 } /* terminator */
};

void sigint_handler(int sig)
{
    interrupted = 1;
    free(arg_phtread);
}

int main(int argc, const char **argv)
{
    struct lws_context_creation_info info;
    struct lws_context *context;
    const char *p;
    int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE;
    
    signal(SIGINT, sigint_handler);
    
    lws_set_log_level(logs, NULL);
    lwsl_user("WebSocket security: http://10.168.75.95:8666\n");
    printf("Enter Ctrl + C for exit.\n");
    memset(&info, 0, sizeof info);
    info.port = 8666;
    info.mounts = NULL;
    info.protocols = protocols;
    info.vhost_name = "10.168.75.95";
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


