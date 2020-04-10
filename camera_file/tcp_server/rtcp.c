#include "rtcp.h"
int crypto_init_session_key_rtcp(struct crypto_context *c)
{
	return c->params.crypto_suite->session_key_init(c);
}
int crypto_decrypt_rtcp(struct crypto_context *c, struct str_key *payload, uint32_t ssrc, uint64_t index)
{
	return c->params.crypto_suite->decrypt_rtcp(c, payload, ssrc, index);
}
int crypto_encrypt_rtcp(struct crypto_context *c, struct str_key *payload, uint32_t ssrc, uint64_t index)
{
	return c->params.crypto_suite->encrypt_rtcp(c, payload, ssrc, index);
}
int check_session_keys_rtcp(struct crypto_context *c)
{
	if (c->have_session_key)
		return 0;

	struct str_key s;

	s.str = (unsigned char *)c->session_key;
	s.len = c->params.crypto_suite->session_key_len;
	if (crypto_gen_session_key(c, &s, 0x03, 6))
		goto error;
	s.str = (unsigned char *)c->session_auth_key;
	s.len = c->params.crypto_suite->srtp_auth_key_len;
	if (crypto_gen_session_key(c, &s, 0x04, 6))
		goto error;
	s.str = (unsigned char *)c->session_salt;
	s.len = c->params.crypto_suite->session_salt_len;
	if (crypto_gen_session_key(c, &s, 0x05, 6))
		goto error;
	c->have_session_key = 1;
	crypto_init_session_key_rtcp(c);
	return 0;

error:
	printf("Error in check_session_keys\n");
	return -1;
}
void rtcp_payload(struct rtcp_header* rtcp_h, struct str_key* payload, unsigned char* s, int length)
{
    rtcp_h->length = (s[2] & 0xFF) << 8 | (s[3] & 0xFF);
    rtcp_h->ssrc = (s[4] & 0xFF) << 24 | (s[5] & 0xFF) << 16 | (s[6] & 0xFF) << 8 | (s[7] & 0xFF);
	printf("SSRC result RTCP: %u\n", rtcp_h->ssrc);
    payload->len = length - sizeof(struct rtcp_header);
    payload->str = s + sizeof(struct rtcp_header);
}
int rtcp_savp_to_avp(struct crypto_context *crypto_rtcp, unsigned char *rtcp, int* length) 
{
	struct rtcp_header rtcp_h;
	u_int32_t idx, *idx_p;
	struct str_key to_auth, payload;

	rtcp_payload(&rtcp_h, &payload, rtcp, *length);
	if (check_session_keys_rtcp(crypto_rtcp))
		return -1;

    to_auth.str = rtcp;
    to_auth.len = *length - crypto_rtcp->params.crypto_suite->srtp_auth_tag; // hash
    payload.len -= crypto_rtcp->params.crypto_suite->srtp_auth_tag; //hash

	payload.len -= sizeof(idx);
	idx_p = (void *) payload.str + payload.len;
	idx = ntohl(*idx_p);
	if ((idx & 0x80000000ULL)) {
		if (crypto_decrypt_rtcp(crypto_rtcp, &payload, rtcp_h.ssrc, idx & 0x7fffffffULL))
			return -1;
	}
	to_auth.len -= sizeof(idx);
    *length = to_auth.len;
	return 0;

error:
	printf("Discarded invalid SRTCP packet");
	return -1;
}
int rtcp_avp_to_savp(struct crypto_context *crypto_from_camera, unsigned char *rtcp, int* length, uint32_t* index_rtcp) 
{
    struct rtcp_header rtcp_h;
	u_int32_t *idx;
	struct str_key to_auth, payload;
	rtcp_payload(&rtcp_h, &payload, rtcp, *length);
    if (check_session_keys_rtcp(crypto_from_camera))
		return -1;
    to_auth.str = rtcp;
    to_auth.len = *length;
    printf("SRTCP index -------------->%u\n", *index_rtcp);
    printf("RTCP avp_savp SSRC: -------------------> %u\n", rtcp_h.ssrc);
    if (crypto_encrypt_rtcp(crypto_from_camera, &payload, rtcp_h.ssrc, *index_rtcp))
        return -1;
    
    idx = (void *) to_auth.str + to_auth.len;
    *idx = htonl((0x80000000ULL | *index_rtcp));
    to_auth.len += sizeof(*idx);
    crypto_from_camera->params.crypto_suite->hash_rtcp(crypto_from_camera, to_auth.str + to_auth.len, &to_auth);
    to_auth.len += crypto_from_camera->params.crypto_suite->srtcp_auth_tag;
    *length = to_auth.len;
    (*index_rtcp)++;
	return 1;
error:
	printf("Discarded invalid SRTCP packet");
    return -1;
}


