#include "rtp.h"
static int flag_testing = 0;

int crypto_encrypt_rtp(struct crypto_context *c, struct str_key *payload, uint32_t ssrc, uint64_t index)
{
	return c->params.crypto_suite->encrypt_rtp(c, payload, ssrc, index);
}
int crypto_hash_rtp(struct crypto_context *c, unsigned char *payload, struct str_key *mes_all, uint64_t index)
{
	return c->params.crypto_suite->hash_rtp(c, payload, mes_all, index);
}
int crypto_encrypt_rtcp(struct crypto_context *c, unsigned char *rtcp, uint32_t ssrc, uint64_t index)
{
	return c->params.crypto_suite->encrypt_rtcp(c, rtcp, ssrc, index);
}
int crypto_init_session_key(struct crypto_context *c)
{
	return c->params.crypto_suite->session_key_init(c);
}
void rtp_init(struct pthread_arguments *p_a)
{
	p_a->sequnce_new = 0;
	p_a->sequnce_origin = 0;
}
static void aes_ctr(unsigned char *out, struct str_key *in, EVP_CIPHER_CTX *ecc, const unsigned char *iv)
{
	unsigned char ivx[16];
	unsigned char key_block[16];
	unsigned char *p, *q;
	unsigned int left;
	int outlen, i;
	uint64_t *pi, *qi, *ki;
	printf("Start aes\n");

	memcpy(ivx, iv, 16);
	printf("Ivx1: ---");
	printText(ivx, 16);
	pi = (uint64_t *)in->str;
	qi = (uint64_t *)out;
	ki = (uint64_t *)key_block;
	left = in->len;

	while (left)
	{
		EVP_EncryptUpdate(ecc, key_block, &outlen, ivx, 16);
		printf("Ivx2: ---");
		printText(ivx, 16);
		printf("Block: ---");
		printText(key_block, 16);
		if (left < 16)
		{
			p = (unsigned char *)pi;
			q = (unsigned char *)qi;
			for (i = 0; i < 16; i++)
			{
				*q++ = *p++ ^ key_block[i];
				left--;
				if (!left)
				{
					printf("Go to\n");
					goto done;
				}
			}
			printf("Error: aes_ctr and abort\n");
		}
		*qi++ = *pi++ ^ ki[0];
		*qi++ = *pi++ ^ ki[1];
		left -= 16;

		for (i = 15; i >= 0; i--)
		{
			ivx[i]++;
			if (ivx[i])
				break;
		}
	}

done:
	printf("End aes\n");
	;
}
static void aes_ctr_no_ctx(unsigned char *out, struct str_key *in, const unsigned char *key, const EVP_CIPHER *ciph,
						   const unsigned char *iv)
{
	EVP_CIPHER_CTX *ctx;
	unsigned char block[16];
	int len;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	ctx = EVP_CIPHER_CTX_new();
#else
	EVP_CIPHER_CTX ctx_s;
	ctx = &ctx_s;
	EVP_CIPHER_CTX_init(ctx);
#endif
	EVP_EncryptInit_ex(ctx, ciph, NULL, key, NULL);
	aes_ctr(out, in, ctx, iv);
	EVP_EncryptFinal_ex(ctx, block, &len);

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	EVP_CIPHER_CTX_free(ctx);
#else
	EVP_CIPHER_CTX_cleanup(ctx);
#endif
}

/* rfc 3711 section 4.3.1 and 4.3.3
 * key: 128 bits
 * x: 112 bits
 * n <= 256
 * out->len := n / 8 */
static void prf_n(struct str_key *out, const unsigned char *key, const EVP_CIPHER *ciph, const unsigned char *x)
{
	unsigned char iv[16] = {0};
	unsigned char o[32] = {0};
	unsigned char in[32] = {0};
	struct str_key in_s;

	memcpy(iv, x, 14);
	// iv[14] = iv[15] = 0;   := x << 16
	in_s.str = in;
	in_s.len = out->len > 16 ? 32 : 16;
	aes_ctr_no_ctx(o, &in_s, key, ciph, iv);
	memcpy(out->str, o, out->len);
}
int crypto_gen_session_key(struct crypto_context *c, struct str_key *out, unsigned char label, int index_len)
{
	unsigned char key_id[7] = {0}; /* [ label, 48-bit ROC || SEQ ] */
	unsigned char x[14] = {0};
	int i;
	/* key_id[1..6] := r; or 1..4 for rtcp
	 * key_derivation_rate == 0 --> r == 0 */

	key_id[0] = label;
	memcpy(x, c->params.master_salt, 14);
	for (i = 13 - index_len; i < 14; i++)
		x[i] = key_id[i - (13 - index_len)] ^ x[i];

	prf_n(out, c->params.master_key, (const EVP_CIPHER *)c->params.crypto_suite->lib_cipher_ptr, x);

	printf("Label %02x, length %i\n", label, out->len);
	printf("Generated session key: master key ");
	printText(c->params.master_key, c->params.crypto_suite->master_key_len);
	printf("Generated session key: master salt ");
	printText(c->params.master_salt, c->params.crypto_suite->master_salt_len);
	printf("Generated session key: result ");
	printText(out->str, out->len);
	return 0;
}
unsigned int rtp_sps_parse(unsigned char *rtp, unsigned char *sps, unsigned int sequnce, struct pthread_arguments *p_a)
{
	unsigned int seq_num = sequnce; // sequnce before I-Frame
	unsigned short index = 0;
	unsigned char h = rtp[2];
	unsigned char l = rtp[3];
	for (int i = 0; i < 2; i++)
	{
		sps[index++] = rtp[i];
	}
	sps[index++] = seq_num >> 8;
	sps[index++] = seq_num;
	for (int i = 0; i < 8; i++)
	{
		sps[index++] = rtp[i + 4];
	}
	sps[index++] = 56; // 0x38
	sps[index++] = (p_a->size_sps) & 0xFF >> 8;
	sps[index++] = (p_a->size_sps) & 0xFF;
	//sps[index++] = 39; // 0x27 Nal header byte with sps payload
	for (int i = 0; i < p_a->size_sps; i++)
	{
		sps[index++] = p_a->sps[i];
	}
	sps[index++] = (p_a->size_pps) & 0xFF >> 8;
	sps[index++] = (p_a->size_pps) & 0xFF;
	//sps[index++] = 40; // 0x28 Nal header byte with pps payload
	for (int i = 0; i < p_a->size_pps; i++)
	{
		sps[index++] = p_a->pps[i];
	}
	return index;
}
/** rtp_payload fill payload and sps
 * struct rtp_header save data about rtp header  
 * 
*/
unsigned int rtp_payload(struct rtp_header *rtp, struct str_key *payload, uint32_t *sequnce_origin, uint32_t *sequnce_new,
						 unsigned char *all_mess, struct str_key *rtp_sps, struct pthread_arguments *p_a, int l)
{
	unsigned char v = (all_mess[0] & 0xC0) >> 6;
	unsigned char p = (all_mess[0] & 0x20) >> 5;
	unsigned char x = (all_mess[0] & 0x10) >> 4;
	unsigned char cc = (all_mess[0] & 0x0F);
	unsigned char m = (all_mess[1] & 0x80) >> 7;
	unsigned char payload_type = all_mess[1] & 0x7F;
	unsigned int seq_num = (all_mess[2] & 0xFF) << 8 | (all_mess[3] & 0xFF); // 1054i // 1055 // 1056 // 1057i // 1058
	unsigned long timestamp = (all_mess[4] & 0xFF) << 24 | (all_mess[5] & 0xFF) << 16 | (all_mess[6] & 0xFF) << 8 | (all_mess[7] & 0xFF);
	uint32_t ssrc = (all_mess[8] & 0xFF) << 24 | (all_mess[9] & 0xFF) << 16 | (all_mess[10] & 0xFF) << 8 | (all_mess[11] & 0xFF);
	unsigned int sq_copy = *sequnce_new;
	unsigned short indicator_type = all_mess[12] & 0x1F;
	unsigned short header_type = all_mess[13] & 0x1F;
	unsigned short header_start = (all_mess[13] & 0x80) >> 7;
	all_mess[1] = (all_mess[1] & 0x80) | 102; /** Change payload in rtp header */
	rtp->all_mark[0] = all_mess[0];
	rtp->all_mark[1] = all_mess[1];
	rtp->seq_num = seq_num;
	rtp->ssrc = ssrc;
	rtp->timestamp = timestamp;
	if (flag_testing)
	{
		printf("V = %u\n", v);
		printf("P = %u\n", p);
		printf("X = %u\n", x);
		printf("CC = %u\n", cc);
		/* 1 byte */
		printf("M = %u\n", m);
		printf("Payload tupe = %u\n", payload_type);
		/* 2-3 bytes */
		printf("Sequence number = %u\n", seq_num);
		/* 4-7 bytes */
		printf("Timestamp = %lu\n", timestamp);
		/* 8-11 bytes */
		printf("SSRC = %u\n", ssrc);
	}
	payload->str = all_mess + 12;

	//printf("Old sequnce: %d\n", *sequnce_new);

	if (*sequnce_origin == 0)
	{
		//printf("Start rtp_sequnce\n");
		*sequnce_origin = rtp->seq_num;
		sq_copy = *sequnce_new = 1;
	}
	if (indicator_type == 28 && header_type == 5 && header_start == 1)
	{
		//printf("I-frame\n");
		rtp_sps->len = rtp_sps_parse(all_mess, rtp_sps->str, sq_copy, p_a);
		sq_copy++;
	}
	all_mess[2] = (sq_copy & 0xFF00) >> 8;
	all_mess[3] = sq_copy & 0xFF;
	//printf("New sequnce: %d\n", *sequnce_new);
	return l - 12;
}
int check_session_keys(struct crypto_context *c)
{
	if (c->have_session_key)
		return 0;

	struct str_key s;

	s.str = (unsigned char *)c->session_key;
	s.len = c->params.crypto_suite->session_key_len;
	if (crypto_gen_session_key(c, &s, 0x00, 6))
		goto error;
	if (flag_testing)
	{
		printf("session_key: ");
		for (size_t k = 0; k < 16; k++)
		{
			printf("%02x", (unsigned char)c->session_key[k]);
		}
		printf("\n");
	}
	s.str = (unsigned char *)c->session_auth_key;
	s.len = c->params.crypto_suite->srtp_auth_key_len;
	if (crypto_gen_session_key(c, &s, 0x01, 6))
		goto error;
	if (flag_testing)
	{
		printf("session_auth_key: ");
		for (size_t k = 0; k < 20; k++)
		{
			printf("%02x", (unsigned char)c->session_auth_key[k]);
		}
		printf("\n");
	}
	s.str = (unsigned char *)c->session_salt;
	s.len = c->params.crypto_suite->session_salt_len;
	if (crypto_gen_session_key(c, &s, 0x02, 6))
		goto error;
	if (flag_testing)
	{
		printf("session_salt: ");
		for (size_t k = 0; k < 14; k++)
		{
			printf("%02x", (unsigned char)c->session_salt[k]);
		}
		printf("\n");
	}
	c->have_session_key = 1;
	crypto_init_session_key(c);

	return 0;

error:
	printf("Error in check_session_keys\n");
	return -1;
}
static u_int64_t packet_index(uint32_t seq_num, uint64_t srtp_index)
{
	u_int16_t seq = seq_num;
	if (srtp_index == 0)
		srtp_index = seq;
	/* rfc 3711 appendix A, modified, and sections 3.3 and 3.3.1 */
	u_int16_t s_l = (srtp_index & 0x00000000ffffULL);
	u_int32_t roc = (srtp_index & 0xffffffff0000ULL) >> 16;
	u_int32_t v = 0;

	if (s_l < 0x8000)
	{
		if (((seq - s_l) > 0x8000) && roc > 0)
			v = (roc - 1) % 0x10000;
		else
			v = roc;
	}
	else
	{
		if ((s_l - 0x8000) > seq)
			v = (roc + 1) % 0x10000;
		else
			v = roc;
	}

	return (u_int64_t)(((v << 16) | seq) & 0xffffffffffffULL);
}
/**
 * rtp_to_srtp --> do all work with rtp packets
 * p_a -->struct with all information
 * rtp --> array data with rtp packet
 * l --> size 0f array data
*/
int rtp_to_srtp(struct pthread_arguments *p_a, unsigned char *rtp, unsigned char *rtp_sps, int *l)
{
	printf("rtp_to_srtp\n");
	uint64_t index = 0;
	struct str_key payload; /** Payload for save payload from camera and crypt his and send to browser */
	struct str_key sps_mes;
	struct str_key sps_payload;
	struct rtp_header rtp_h; /** Rtp_h struct for save rtp header */
	struct str_key mes_all;  /** mes_all save all rtp message */
	int socket_rtp;
	struct sockaddr_in servaddr;
	sps_mes.str = rtp_sps; /** Struct with all sps data*/
	sps_mes.len = 0;

	payload.len = rtp_payload(&rtp_h, &payload, &p_a->sequnce_origin, &p_a->sequnce_new, rtp, &sps_mes, p_a, *l); /** Fill rtp_h and payload structs from array bytes from camera */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;				// IPv4
	inet_aton("127.0.0.1", &servaddr.sin_addr); /// Address browser
	servaddr.sin_port = htons(40800);

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
	if (sps_mes.len != 0)
	{
		//printf("Ok\n");
		sps_payload.str = sps_mes.str + 12;
		sps_payload.len = sps_mes.len - 12;
		sendto(p_a->socket_stream, (char *)rtp_sps, sps_mes.len, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		check_session_keys(&p_a->crypto);
		index = packet_index(p_a->sequnce_new, index); /// 1 ...
		crypto_encrypt_rtp(&p_a->crypto, &sps_payload, rtp_h.ssrc, index);
		crypto_hash_rtp(&p_a->crypto, (unsigned char *)(sps_mes.str + sps_mes.len), &sps_mes, index);
		sps_mes.len += 10;
		p_a->sequnce_new += 1; /// 2 ...
							   //printf("Nice\n");
	}

	sendto(p_a->socket_stream, (char *)rtp, *l, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
	mes_all.str = rtp;
	mes_all.len = *l;

	//printf("All message: \n\tto_tag_size: %d\n", mes_all.len);

	check_session_keys(&p_a->crypto);
	index = packet_index(p_a->sequnce_new, index);

	crypto_encrypt_rtp(&p_a->crypto, &payload, rtp_h.ssrc, index);
	/** Function Hash all mess and add    
	 * payload->str + payload->len --- place where will save 10 bytes
	 * mes_all --- all message
	 * 
	*/
	crypto_hash_rtp(&p_a->crypto, (unsigned char *)(mes_all.str + mes_all.len), &mes_all, index);
	payload.len += p_a->crypto.params.crypto_suite->srtp_auth_tag;
	mes_all.len += p_a->crypto.params.crypto_suite->srtp_auth_tag;
	p_a->sequnce_new += 1;
	p_a->sequnce_new = p_a->sequnce_new % 0x01FFFF;
	*l = mes_all.len;
	close(socket_rtp);
	return sps_mes.len;
}
