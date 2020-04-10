#include "stun.h"
unsigned int stun_header(struct msghdr *mh, struct iovec *iov, struct header *hdr, unsigned char *buf, unsigned int *index, unsigned char* transaction)
{
    memset(mh, 0, sizeof(*mh));
    unsigned int size = *index;
    mh->msg_iov = iov;
    mh->msg_iovlen = 1;
    iov->iov_base = hdr;
    iov->iov_len = sizeof(*hdr); // 20
    hdr->msg_type = htons(HEADER_TYPE);
    hdr->magick = htonl(MAGICK);
    hdr->data_len = 0;

    buf[size++] = HEADER_TYPE >> 8;
    buf[size++] = HEADER_TYPE;
    buf[size++] = 0x0;
    buf[size++] = 0x0;
    short t = 3;
    while (t >= 0)
    {
        buf[size++] = MAGICK >> (8 * t);
        t--;
    }
    if(transaction == NULL)
    {
        gen_random((unsigned char *)(buf + size), 12);
        for(int k = 0; k < 12; k++)
        {
            printf("%X.", buf[8 + k]);
        }
        printf("\n");
        memcpy(hdr->id, (unsigned char *)(buf + size), 12);
    }
    else
    {
        hdr->msg_type = htons(0x0101);
        buf[0] = 0x01;
        buf[1] = 0x01;
        memcpy((unsigned char *)(buf + size), transaction, 12);
        for(int k = 0; k < 12; k++)
        {
            printf("%X.", buf[8 + k]);
        }
        printf("\n");
        memcpy(hdr->id, (unsigned char *)transaction, 12);
    }
    size += 12;
    *index = size;
    return size;
}
/**
 * buf -> Message with all text for network
 * index -> index for message 
*/
unsigned int stun_software(struct msghdr *mh, struct software *sw, unsigned char *buf, unsigned int *index)
{
    size_t len = sprintf(sw->str, "webrtpstream-1.0.0"); // Text in software attribute, his len is 18
    struct iovec *iov = &mh->msg_iov[mh->msg_iovlen++];
    struct header *hdr;
    unsigned int size = *index;
    unsigned int size_all;
    //
    iov->iov_len = sizeof(sw->t); // Size struct = 4
    iov->iov_base = &(sw->t);     // iov_base = tlv
    sw->t.type = htons(SOFTWARE); /// Code SOFTWARE
    sw->t.len = htons(len);       // len message without padding = 18
    //
    hdr = (struct header *)(mh->msg_iov->iov_base); /// Struct header for add size message
    size_t time = 4 + ((len + 3) & 0xfffc);         /// Size struct + len message + padding; 18 + 3 = 21 & 0xfffc = 20 + 4 = 24
    hdr->data_len += time;                          /// Add size message; len message = 18 + padding(2) = 20 + size struct tlv(4) = 24
    size_all = hdr->data_len;
    //
    iov = &mh->msg_iov[mh->msg_iovlen++]; /// Add text message
    iov->iov_len = time - 4;              //Only message text but with padding = 20
    iov->iov_base = sw->str; // 18 + 2
    memset(sw->str + len, 0, 2);
    //
    buf[2] = size_all >> 8;
    buf[3] = size_all;
    buf[size++] = (SOFTWARE & 0xFF00) >> 8;
    buf[size++] = (SOFTWARE & 0xFF);
    buf[size++] = len >> 8;
    buf[size++] = len;
    strncpy((char *)buf + size, sw->str, len);
    size += len;
    buf[size++] = 0x0;
    buf[size++] = 0x0;
    //

    printf("\n");
    printf("Size iov_len = %zu\n", sizeof(sw->t));
    printf("Size all message with padding = %zu\n", time);
    printf("Size in header = %u\n", size_all);
    *index = size;
    return time;
}
unsigned int stun_username(struct msghdr *mh, struct username *user, unsigned char *buf, unsigned int *index, char *buf_name, size_t len)
{
    //char buf_name[15];
    struct iovec *iov = &mh->msg_iov[mh->msg_iovlen++];
    struct header *hdr;
    unsigned int i = *index;
    unsigned int size_all;
    iov->iov_base = &user->t;
    iov->iov_len = sizeof(user->t); /// Size of struct = 4
    user->t.type = htons(USERNAME); /// Code USERNAME = 0x0006
    user->t.len = htons(len);       /// Length message
    hdr = (struct header *)mh->msg_iov->iov_base;
    size_t time = sizeof(user->t) + len + 3;
    hdr->data_len += time;    /// Size struct + len message + padding
    size_all = hdr->data_len; /// Size of all stun message with all attributes without header
    //
    iov = &mh->msg_iov[mh->msg_iovlen++]; /// Add text message
    iov->iov_len = time - 4;              /// Only message text but with padding
    iov->iov_base = buf_name;
    memset(buf_name + len, 0, 3);
    //
    buf[2] = size_all >> 8;
    buf[3] = size_all;
    buf[i++] = USERNAME >> 8;
    buf[i++] = USERNAME;
    buf[i++] = len >> 8;
    buf[i++] = len;
    strncpy((char *)buf + i, buf_name, len);
    i += len;
    buf[i++] = 0x0;
    buf[i++] = 0x0;
    buf[i++] = 0x0;

    printf("\n");
    printf("Size iov_len = %zu\n", sizeof(user->t));
    printf("Size all message with padding = %zu\n", time);
    printf("Size in header = %u\n", size_all);
    *index = i;
    return time;
}
unsigned int stun_controlled(struct msghdr *mh, struct ice_controlling *contr, unsigned char *buf, unsigned int *index)
{
    struct iovec *iov = &mh->msg_iov[mh->msg_iovlen++];
    struct header *hdr;
    unsigned int i = *index;
    unsigned int size_all;
    uint64_t test = 0;
    iov->iov_base = &contr->t;
    iov->iov_len = sizeof(*contr);              /// 12
    contr->t.type = htons(ICE_CONTROLLED);      /// Code ICE_CONTROLLED = 0x8029
    contr->t.len = htons(ICE_CONTROLLING_LENGTH); /// 8
    hdr = (struct header *)mh->msg_iov->iov_base;
    size_t time = 4 + ICE_CONTROLLING_LENGTH;
    hdr->data_len += time;    /// Size struct + len message + padding
    size_all = hdr->data_len; /// Size of all stun message with all attributes without header
    //
    buf[2] = size_all >> 8;
    buf[3] = size_all;
    buf[i++] = (ICE_CONTROLLED & 0xFF00) >> 8;
    buf[i++] = (ICE_CONTROLLED & 0xFF);
    buf[i++] = ICE_CONTROLLING_LENGTH >> 8;
    buf[i++] = ICE_CONTROLLING_LENGTH;

    gen_random((unsigned char *)(buf + i), 8);
    memcpy(contr->value, (unsigned char *)(buf + i), 8);
    //contr->value[7] = '\0';
    /*size_t j = i;
    for (int k = 0; k < 8 ; k++)
    {
        contr->value += buf[j] << k * 8;
        
        printf("%X.", buf[j]);
        j++;
    }*/
    i += 8;
    printf("\n");
    printf("Size iov_len = %zu\n", sizeof(*contr));
    printf("Size all message = %zu\n", time);
    printf("Size in header = %u\n", size_all);
    *index = i;
    return time;
}
unsigned int stun_priority(struct msghdr *mh, struct priority *prior, unsigned char *buf, unsigned int *index)
{
    struct iovec *iov = &mh->msg_iov[mh->msg_iovlen++];
    struct header *hdr;
    unsigned int i = *index;
    unsigned int size_all;
    uint32_t priority_value = 1853817087;
    iov->iov_base = &prior->t;
    iov->iov_len = sizeof(*prior);                 /// 4 + 4 = 8
    prior->t.type = htons(PRIORITY);               /// Code PRIORITY = 0x0024
    prior->t.len = htons(PRIORITY_LENGTH); /// 4
    hdr = (struct header *)mh->msg_iov->iov_base;
    size_t time = 4 + PRIORITY_LENGTH; /// 4 + 4 = 8
    prior->priority = htonl(priority_value);
    hdr->data_len += time;    /// Size struct + len message + padding
    size_all = hdr->data_len; /// Size of all stun message with all attributes without header
    //
    buf[2] = size_all >> 8;
    buf[3] = size_all;
    buf[i++] = PRIORITY >> 8;
    buf[i++] = PRIORITY;
    buf[i++] = PRIORITY_LENGTH >> 8;
    buf[i++] = PRIORITY_LENGTH;

    for (int k = 3; k >= 0; k--)
    {
        buf[i++] = 0xFF & (priority_value >> k * 8);
    }
    printf("\n");
    printf("Size iov_len = %zu\n", sizeof(*prior));
    printf("Size all message = %zu\n", time);
    printf("Size in header = %u\n", size_all);
    *index = i;
    return time;
}
unsigned int stun_integrity(struct msghdr *mh, struct message_integrity *mi, struct str *browser_pwd, unsigned char *buf, unsigned int *index)
{
    struct iovec *iov = &mh->msg_iov[mh->msg_iovlen++];
    struct header *hdr;
    unsigned int i = *index;
    unsigned int size_all;
    int integ;
    HMAC_CTX *ctx;

    iov->iov_base = &mi->t;
    iov->iov_len = 4 + MESSAGE_INTEGRITY_LENGTH; /// 4 + 20 = 24
    mi->t.len = htons(MESSAGE_INTEGRITY_LENGTH);
    mi->t.type = htons(MESSAGE_INTEGRITY);
    //
    hdr = (struct header *)mh->msg_iov->iov_base;
    size_t time = 4 + MESSAGE_INTEGRITY_LENGTH; /// 4 + 20 = 24
    hdr->data_len += time;                       /// Size struct + len message + padding
    size_all = hdr->data_len;                    /// Size of all stun message with all attributes without header
    //
    iov = mh->msg_iov;
    hdr->data_len = htons(hdr->data_len);
    /// Calculate integrity ///
    int iov_cnt = mh->msg_iovlen - 1;
    
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    ctx = HMAC_CTX_new();
#else
    HMAC_CTX ctx_s;
    HMAC_CTX_init(&ctx_s);
    ctx = &ctx_s;
#endif
    /* do we need to SASLprep here? */
    HMAC_Init_ex(ctx, browser_pwd->s, browser_pwd->len, EVP_sha1(), NULL);

    for (integ = 0; integ < iov_cnt; integ++)
        HMAC_Update(ctx, (const unsigned char*)iov[integ].iov_base, iov[integ].iov_len);
    
    HMAC_Final(ctx, (unsigned char* )mi->digest, NULL);

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    HMAC_CTX_free(ctx);
#else
    HMAC_CTX_cleanup(ctx);
#endif

    buf[2] = size_all >> 8;
    buf[3] = size_all;
    buf[i++] = MESSAGE_INTEGRITY >> 8;
    buf[i++] = MESSAGE_INTEGRITY;
    buf[i++] = MESSAGE_INTEGRITY_LENGTH >> 8;
    buf[i++] = MESSAGE_INTEGRITY_LENGTH;

    for(int k = 0; k < MESSAGE_INTEGRITY_LENGTH; k++)
    {
        buf[i++] = mi->digest[k];
        printf("%X.", buf[i-1]);
    } 

    hdr->data_len = ntohs(hdr->data_len);
    printf("\n");
    printf("Size iov_len = %d\n", 4 + MESSAGE_INTEGRITY_LENGTH);
    printf("Size all message = %zu\n", time);
    printf("Size in header = %u\n", size_all);
    *index = i;
    return time;
}
unsigned int stun_fingerprint(struct msghdr *mh, struct fingerprint *fp, unsigned char *buf, unsigned int *index)
{
    struct iovec *iov = &mh->msg_iov[mh->msg_iovlen++];
    struct header *hdr;
    unsigned int i = *index;
    unsigned int size_all;

    iov->iov_base = &fp->t;
    iov->iov_len = sizeof(*fp);                 /// 4 + 4 = 8
    fp->t.type = htons(FINGERPRINT);               /// Code FINGERPRINT = 0x8028
    fp->t.len = htons(FINGERPRINT_LENGTH); /// 4

    hdr = (struct header *)mh->msg_iov->iov_base;
    size_t time = 4 + FINGERPRINT_LENGTH; /// 4 + 4 = 8
    hdr->data_len += time;    /// Size struct + len message
    size_all = hdr->data_len; /// Size of all stun message with all attributes without header
    //

    hdr->data_len = htons(hdr->data_len);
    iov = mh->msg_iov;
	fp->crc = crc32(0, NULL, 0);
	for (size_t j = 0; j < mh->msg_iovlen - 1; j++)
    {
		fp->crc = crc32(fp->crc, (const Bytef* )iov[j].iov_base, iov[j].iov_len);
    }

	fp->crc = htonl(fp->crc ^ STUN_CRC_XOR);
	//hdr->data_len = ntohs(hdr->data_len);
    //
    buf[2] = size_all >> 8;
    buf[3] = size_all;
    buf[i++] = (FINGERPRINT & 0xFF00) >> 8;
    buf[i++] = (FINGERPRINT & 0xFF);

    buf[i++] = FINGERPRINT_LENGTH >> 8;
    buf[i++] = FINGERPRINT_LENGTH;

    for (int k = 0; k < 4; k++)
    {
        buf[i++] = 0xFF & (fp->crc >> k * 8);
        printf("%X.", buf[i-1]);
    }
    printf("\n");
    for(int k = 0; k < 12; k++)
    {
        printf("%X.", buf[8 + k]);
    }
    printf("\n");
    printf("Size iov_len = %zu\n", sizeof(*fp));
    printf("Size all message = %zu\n", time);
    printf("Size in header = %u\n", size_all);
    *index = i;
    return time;
}
int stun_xor_mapped(struct msghdr *mh, struct xor_mapped_address *x_m_a, unsigned char *buf, unsigned int *index)
{
    struct iovec *iov = &mh->msg_iov[mh->msg_iovlen++];
    struct header *hdr;
    unsigned int i = *index;
    unsigned int size_all;
    iov->iov_base = &x_m_a->t;
    iov->iov_len = XOR_MAPPED_ADDRESS_LENGTH + 4; /// 4 + 8 = 12
    x_m_a->t.type = htons(XOR_MAPPED_ADDRESS);               /// Code 
    x_m_a->t.len = htons(XOR_MAPPED_ADDRESS_LENGTH); /// 4

    hdr = (struct header *)mh->msg_iov->iov_base;
    size_t time = 4 + XOR_MAPPED_ADDRESS_LENGTH; /// 4 + 8 = 12
    hdr->data_len += time;    /// Size struct + len message
    size_all = hdr->data_len; /// Size of all stun message with all attributes without header
    buf[2] = size_all >> 8;
    buf[3] = size_all;
    buf[i++] = (XOR_MAPPED_ADDRESS & 0xFF00) >> 8;
    buf[i++] = (XOR_MAPPED_ADDRESS & 0xFF);

    buf[i++] = XOR_MAPPED_ADDRESS_LENGTH >> 8;
    buf[i++] = XOR_MAPPED_ADDRESS_LENGTH;
    buf[i++] = x_m_a->family;
    buf[i++] = x_m_a->family >> 8;
    buf[i++] = x_m_a->port;
    buf[i++] = x_m_a->port >> 8;
    memcpy(buf + i, x_m_a->address, 4);
    i += 4;
    *index = i;
    return time;
}
int stun_response(struct pthread_arguments *p_a, unsigned char* stun_req, unsigned int s, struct sockaddr_in* addr)
{
    struct header hdr;
    struct msghdr mh;
    struct iovec iov[4];
    struct message_integrity m_i;
    struct fingerprint fp;
    struct xor_mapped_address x_m_a;
    unsigned char stun_request[150];
    unsigned char stun_response[120];
    unsigned char transaction[12];
    unsigned int size_stun_message = 0;
    unsigned char ip[20] = {0};
    unsigned int port;
    struct sockaddr_in stun_to_browser;
    int size_sockaddr_in = sizeof(stun_to_browser);
    int answer = 0;
    if(stun_req != NULL)
    {
        memcpy(stun_request, stun_req, s);
        memcpy(&stun_to_browser, addr, size_sockaddr_in);
    }
    else
    {
        answer = recvfrom(p_a->socket_stream, stun_request, sizeof(stun_request), 0, (struct sockaddr *)&stun_to_browser, (socklen_t*)&size_sockaddr_in);
        if(answer <= 0)
            return 1;
    }
    strcpy((char* )ip, inet_ntoa(stun_to_browser.sin_addr));
    port = ntohs(stun_to_browser.sin_port);
    printf("Recv from %s:%d\n", ip, port);
    
    memcpy(transaction, stun_request + 8, 12); /// In header start from 8 index  
    unsigned int i = 0; /// Index for last change in buffer
    size_stun_message = stun_header(&mh, iov, &hdr, stun_response, &i, transaction);
    x_m_a.family = htons(0x0001);
    printf("PORT = %X\n", stun_to_browser.sin_port);
    x_m_a.port = (port ^ (MAGICK >> 16));
    x_m_a.port = htons(x_m_a.port);
    x_m_a.address[0] = stun_to_browser.sin_addr.s_addr ^ htonl(MAGICK);
    size_stun_message += stun_xor_mapped(&mh, &x_m_a, stun_response, &i);
    size_stun_message += stun_integrity(&mh, &m_i, &p_a->pwd_server, stun_response, &i);
    size_stun_message += stun_fingerprint(&mh, &fp, stun_response, &i);
    
    mh.msg_name = &stun_to_browser;
    mh.msg_namelen = sizeof(struct sockaddr_in);
    
    // if (sendmsg(p_a->socket_stream, &mh, 0) < 0)
    // {
    //     perror("Sendmsg failed");
    //     return -1;
    // }
    int k = sendto(p_a->socket_stream, stun_response, size_stun_message, 0, (struct sockaddr *)&stun_to_browser, sizeof(stun_to_browser));
    printf("Size send stun: %d\n", k);
    return 0;    
}
int stun_request(struct pthread_arguments *p_a)
{
    struct msghdr mh;
    struct msghdr mhdr;
    struct iovec iov[10];
    struct header hdr;
    struct software sw;
    struct username user;
    struct priority pr;
    struct message_integrity m_i;
    struct ice_controlling ice_contr;
    struct sockaddr_in stun_to_browser;
    struct fingerprint fp;
    char buf_name[20] = {0};
    unsigned int size_stun_message = 0;
    unsigned char stun_request[150];
    unsigned char stun_answer[120];

    memset(&stun_to_browser, 0, sizeof(stun_to_browser)); 
    stun_to_browser.sin_family = AF_INET; // IPv4
    inet_aton(p_a->ip_browser, &stun_to_browser.sin_addr); /// Address browser
    stun_to_browser.sin_port = htons(p_a->port_ice_browser); /// Port browser
    
    unsigned int i = 0; /// Index for last change in buffer
    size_t len = sprintf(buf_name, "%s:sEMT", p_a->uflag_browser);
    size_stun_message = stun_header(&mh, iov, &hdr, stun_request, &i, NULL);
    size_stun_message += stun_software(&mh, &sw, stun_request, &i);
    size_stun_message += stun_username(&mh, &user, stun_request, &i, buf_name, len);
    size_stun_message += stun_controlled(&mh, &ice_contr, stun_request, &i);
    size_stun_message += stun_priority(&mh, &pr, stun_request, &i);
    size_stun_message += stun_integrity(&mh, &m_i, &p_a->pwd_browser, stun_request, &i);
    size_stun_message += stun_fingerprint(&mh, &fp, stun_request, &i);
    int response = sendto(p_a->socket_stream, stun_request, size_stun_message, 0, (struct sockaddr *)&stun_to_browser, sizeof(stun_to_browser));
    printf("Sendto ----> %d\n", response);
    if(response <= 0)
    {
        perror("Sendto failed");
        return 1;
    } 
    int answer = recvfrom(p_a->socket_stream, stun_answer, sizeof(stun_answer), 0, NULL, 0);
    if(answer <= 0)
    {
        perror("Recvfrom failed");
        return 1;
    } 
    
    /*
    mh.msg_name = &stun_to_browser;
    mh.msg_namelen = sizeof(struct sockaddr_in);
    mhdr.msg_name = &stun_to_browser;
    mhdr.msg_namelen = sizeof(struct sockaddr_in);
    
    if (sendmsg(p_a->socket_stream, &mh, 0) < 0)
    {
        perror("Sendmsg failed");
        return -1;
    }
    
    if (recvmsg(p_a->socket_stream, &mhdr, 0) < 0)
    {
        perror("Recvmsg failed");
        return -1;
    }*/
    return 0;
}
