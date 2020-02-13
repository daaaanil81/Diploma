#include "dtls.h"

static int flag_testing = 0;
struct crypto_suite c_suites[] = {
	{
		.name = "AES_CM_128_HMAC_SHA1_80",
		.dtls_name = "SRTP_AES128_CM_SHA1_80",
		.master_key_len = 16,
		.master_salt_len = 14,
		.session_key_len = 16,
		.session_salt_len = 14,
		.srtp_auth_tag = 10,
		.srtcp_auth_tag = 10,
		.srtp_auth_key_len = 20,
		.srtcp_auth_key_len = 20,
		.srtp_lifetime = 1ULL << 48,
		.srtcp_lifetime = 1ULL << 31,
		.kernel_cipher = 2,
		.kernel_hmac = 2,
		.encrypt_rtp = aes_cm_encrypt_rtp,
		.decrypt_rtp = aes_cm_encrypt_rtp,
		.encrypt_rtcp = aes_cm_encrypt_rtcp,
		.decrypt_rtcp = aes_cm_encrypt_rtcp,
		.hash_rtp = hmac_sha1_rtp,
		.hash_rtcp = hmac_sha1_rtcp,
		.session_key_init = aes_cm_session_key_init,
		.session_key_cleanup = evp_session_key_cleanup,
		.lib_cipher_ptr = NULL,
		.idx = 0,
	},
	{
		.name = "AES_CM_128_HMAC_SHA1_32",
		.dtls_name = "SRTP_AES128_CM_SHA1_32",
		.master_key_len = 16,
		.master_salt_len = 14,
		.session_key_len = 16,
		.session_salt_len = 14,
		.srtp_auth_tag = 4,
		.srtcp_auth_tag = 10,
		.srtp_auth_key_len = 20,
		.srtcp_auth_key_len = 20,
		.srtp_lifetime = 1ULL << 48,
		.srtcp_lifetime = 1ULL << 31,
		.kernel_cipher = 2,
		.kernel_hmac = 2,
		.encrypt_rtp = aes_cm_encrypt_rtp,
		.decrypt_rtp = aes_cm_encrypt_rtp,
		.encrypt_rtcp = aes_cm_encrypt_rtcp,
		.decrypt_rtcp = aes_cm_encrypt_rtcp,
		.hash_rtp = hmac_sha1_rtp,
		.hash_rtcp = hmac_sha1_rtcp,
		.session_key_init = aes_cm_session_key_init,
		.session_key_cleanup = evp_session_key_cleanup,
		.lib_cipher_ptr = NULL,
		.idx = 0,
	},
};

int cert_init(struct pthread_arguments *p_a)
{
	p_a->pkey = EVP_PKEY_new();
	p_a->exponent = BN_new();
	p_a->rsa = RSA_new();
	p_a->serial_number = BN_new();
	p_a->name = X509_NAME_new();
	p_a->x509 = X509_new();
	if (!p_a->exponent || !p_a->pkey || !p_a->rsa || !p_a->serial_number || !p_a->name || !p_a->x509)
		goto err;

	/* key */

	if (!BN_set_word(p_a->exponent, 0x10001))
		goto err;

	if (!RSA_generate_key_ex(p_a->rsa, 1024, p_a->exponent, NULL))
		goto err;

	if (!EVP_PKEY_assign_RSA(p_a->pkey, p_a->rsa))
		goto err;

	/* x509 cert */

	if (!X509_set_pubkey(p_a->x509, p_a->pkey))
		goto err;

	/* serial */

	if (!BN_pseudo_rand(p_a->serial_number, 64, 0, 0))
		goto err;

	p_a->asn1_serial_number = X509_get_serialNumber(p_a->x509);
	if (!p_a->asn1_serial_number)
		goto err;

	if (!BN_to_ASN1_INTEGER(p_a->serial_number, p_a->asn1_serial_number))
		goto err;

	/* version 1 */

	if (!X509_set_version(p_a->x509, 0L))
		goto err;

	/* common name */

	if (!X509_NAME_add_entry_by_NID(p_a->name, NID_commonName, MBSTRING_UTF8,
									(unsigned char *)"danil_test", -1, -1, 0))
		goto err;

	if (!X509_set_subject_name(p_a->x509, p_a->name))
		goto err;

	if (!X509_set_issuer_name(p_a->x509, p_a->name))
		goto err;

	/* cert lifetime */

	if (!X509_gmtime_adj(X509_get_notBefore(p_a->x509), -60 * 60 * 24))
		goto err;

	if (!X509_gmtime_adj(X509_get_notAfter(p_a->x509), CERT_EXPIRY_TIME))
		goto err;

	/* sign it */

	if (!X509_sign(p_a->x509, p_a->pkey, EVP_sha1()))
		goto err;

	/* digest */

	p_a->attr_fingerprint.size = sha_1_func(p_a->attr_fingerprint.digest_fingerprint, p_a->x509);

	dump_cert(p_a);

	return 0;
err:
	printf("Error with dtls fingerprint for sdp\n");
	if (p_a->pkey)
		EVP_PKEY_free(p_a->pkey);
	if (p_a->exponent)
		BN_free(p_a->exponent);
	if (p_a->rsa)
		RSA_free(p_a->rsa);
	if (p_a->x509)
		X509_free(p_a->x509);
	if (p_a->serial_number)
		BN_free(p_a->serial_number);

	return -1;
}
static unsigned int generic_func(unsigned char *o, X509 *x, const EVP_MD *md)
{
	unsigned int n;
	X509_digest(x, md, o, &n);
	return n;
}

static unsigned int sha_1_func(unsigned char *o, X509 *x)
{
	const EVP_MD *md;
	md = EVP_sha1();
	return generic_func(o, x, md);
}
static void dump_cert(struct pthread_arguments *cert)
{
	FILE *fp;
	char *buf;
	size_t len;

	/* cert */
	fp = open_memstream(&buf, &len);
	PEM_write_X509(fp, cert->x509);
	fclose(fp);
	printf("Certificate\n");
	buf_dump_free(buf, len);

	/* key */
	fp = open_memstream(&buf, &len);
	PEM_write_PrivateKey(fp, cert->pkey, NULL, NULL, 0, 0, NULL);
	fclose(fp);

	buf_dump_free(buf, len);
}
int try_connect(struct dtls_connection *d)
{
	int ret = 0, code;

	printf("Try_connect\n");
	ret = SSL_connect(d->ssl);
	code = SSL_get_error(d->ssl, ret);
	ret = 0;
	switch (code)
	{
	case SSL_ERROR_NONE:
		ret = 1;
		break;

	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
		break;

	default:
		ret = ERR_peek_last_error();
		printf("DTLS error: %i (%s)\n", code, ERR_reason_error_string(ret));
		ret = -1;
		break;
	}
	return ret;
}
int dtls_connection_init(struct pthread_arguments *p_a)
{
	struct dtls_connection *d = &p_a->dtls_cert;
	EC_KEY *ecdh = NULL;
	printf("Dtls_connection_init\n");

#if OPENSSL_VERSION_NUMBER >= 0x10002000L
	d->ssl_ctx = SSL_CTX_new(DTLS_client_method());
#else
	d->ssl_ctx = SSL_CTX_new(active ? DTLSv1_client_method() : DTLSv1_server_method());
#endif
	if (!d->ssl_ctx)
	{
		printf("LINE:%d:Error in SSL_CTX_new\n", __LINE__);
		goto error;
	}
	if (SSL_CTX_use_certificate(d->ssl_ctx, p_a->x509) != 1)
	{
		printf("LINE:%d:Error in SSL_CTX_use_certificate\n", __LINE__);
		goto error;
	}
	if (SSL_CTX_use_PrivateKey(d->ssl_ctx, p_a->pkey) != 1)
	{
		printf("LINE:%d:Error in SSL_CTX_use_PrivateKey\n", __LINE__);
		goto error;
	}
	SSL_CTX_set_cipher_list(d->ssl_ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
	if (SSL_CTX_set_tlsext_use_srtp(d->ssl_ctx, ciphers_str))
	{
		printf("LINE:%d:Error in SSL_CTX_set_tlsext_use_srtp\n", __LINE__);
		goto error;
	}
	if (SSL_CTX_set_read_ahead(d->ssl_ctx, 1))
	{
		printf("LINE:%d:Error in SSL_CTX_set_read_ahead\n", __LINE__);
		goto error;
	}
	d->ssl = SSL_new(d->ssl_ctx);
	if (!d->ssl)
	{
		printf("LINE:%d:Error in SSL_new\n", __LINE__);
		goto error;
	}
	d->r_bio = BIO_new(BIO_s_mem());
	d->w_bio = BIO_new(BIO_s_mem());
	if (!d->r_bio || !d->w_bio)
	{
		printf("LINE:%d:Error in BIO_new\n", __LINE__);
		goto error;
	}
	SSL_set_app_data(d->ssl, d);
	SSL_set_bio(d->ssl, d->r_bio, d->w_bio);

	SSL_set_mode(d->ssl, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
	if (ecdh == NULL)
	{
		printf("LINE:%d:Error in EC_KEY_new_by_curve_name\n", __LINE__);
		goto error;
	}
	SSL_set_options(d->ssl, SSL_OP_SINGLE_ECDH_USE);
	SSL_set_tmp_ecdh(d->ssl, ecdh);
	EC_KEY_free(ecdh);
#if defined(SSL_OP_NO_QUERY_MTU)
	SSL_CTX_set_options(d->ssl_ctx, SSL_OP_NO_QUERY_MTU);
	SSL_set_mtu(d->ssl, 1500);
#if defined(DTLS_set_link_mtu) || defined(DTLS_CTRL_SET_LINK_MTU) || OPENSSL_VERSION_NUMBER >= 0x10100000L
	DTLS_set_link_mtu(d->ssl, 1500);
#endif
#endif
	return 0;

error:
	if (p_a->dtls_cert.r_bio)
		BIO_free(d->r_bio);
	if (d->w_bio)
		BIO_free(d->w_bio);
	if (d->ssl)
		SSL_free(d->ssl);
	if (d->ssl_ctx)
		SSL_CTX_free(d->ssl_ctx);
	bzero(d, sizeof(struct dtls_connection));
	printf("Failed to init DTLS connection\n");
	return -1;
}
void dtls_connection_cleanup(struct dtls_connection *c)
{
	printf("dtls_connection_cleanup\n");
	if (c->r_bio)
		BIO_free(c->r_bio);
	if (c->w_bio)
		BIO_free(c->w_bio);
	if (c->ssl)
		SSL_free(c->ssl);
	if (c->ssl_ctx)
		SSL_CTX_free(c->ssl_ctx);
	bzero(c, sizeof(struct dtls_connection));
}
void dtls_fingerprint_free(struct pthread_arguments *cert)
{
	printf("dtls_fingerprint_free\n");
	if (cert->name)
		X509_NAME_free(cert->name);
	if (cert->exponent)
		BN_free(cert->exponent);
	if (cert->rsa)
		RSA_free(cert->rsa);
	if (cert->serial_number)
		BN_free(cert->serial_number);
	// if (cert->pkey)
	// 	EVP_PKEY_free(cert->pkey);
	// if (cert->x509)
	// 	X509_free(cert->x509);
}
static void buf_dump_free(char *buf, size_t len)
{
	char *p, *f;
	int llen;
	p = buf;
	while (len)
	{
		f = (char *)memchr(p, '\n', len);
		if (f)
			llen = f - p;
		else
			llen = len;

		printf("--- %.*s", llen, p);

		if (!f)
			break;
		len -= llen + 1;
		p = f + 1;
	}
	printf("\n");
	free(buf);
}
void dtls_init()
{
	char *p = ciphers_str;
	for (size_t i = 0; i < num_crypto_suites; i++)
	{
		if (!c_suites[i].dtls_name)
			continue;
		p += sprintf(p, "%s:", c_suites[i].dtls_name);
	}
	p[-1] = '\0';
}
int dtls_setup_crypto(struct dtls_connection *d, struct crypto_context *crypto)
{
	printf("Dtls_setup_crypto\n");
	const char *err;
	SRTP_PROTECTION_PROFILE *spp;
	int i;
	const struct crypto_suite *cs;
	unsigned char keys[2 * (SRTP_MAX_MASTER_KEY_LEN + SRTP_MAX_MASTER_SALT_LEN)];
	struct crypto_params server;
	struct crypto_params *client = &crypto->params;
	crypto->have_session_key = 0;
	err = "no SRTP protection profile negotiated";
	spp = SSL_get_selected_srtp_profile(d->ssl);
	if (!spp)
		goto error;

	for (i = 0; i < num_crypto_suites; i++)
	{
		cs = &c_suites[i];
		if (!cs->dtls_name)
			continue;
		if (!strcmp(cs->dtls_name, spp->name))
		{
			printf("Found dtls index %d\n", i);
			printf("Found dtls name: %s\n", cs->dtls_name);
			goto found;
		}
	}

	err = "unknown SRTP protection profile negotiated";
	goto error;

found:
	i = SSL_export_keying_material(d->ssl, keys, sizeof(keys), "EXTRACTOR-dtls_srtp",
								   strlen("EXTRACTOR-dtls_srtp"), NULL, 0, 0);
	err = "failed to export keying material";
	if (i != 1)
		goto error;

	/* got everything XXX except MKI */
	bzero(client, sizeof(struct crypto_params));
	bzero(&server, sizeof(struct crypto_params));
	i = 0;

	client->crypto_suite = server.crypto_suite = cs;

	memcpy(client->master_key, &keys[i], cs->master_key_len);
	i += cs->master_key_len;
	memcpy(server.master_key, &keys[i], cs->master_key_len);
	i += cs->master_key_len;
	memcpy(client->master_salt, &keys[i], cs->master_salt_len);
	i += cs->master_salt_len;
	memcpy(server.master_salt, &keys[i], cs->master_salt_len);

	printf("SRTP keys negotiated: "
		   "c-m: %02x%02x%02x%02x%02x%02x%02x%02x "
		   "c-s: %02x%02x%02x%02x%02x%02x%02x%02x "
		   "s-m: %02x%02x%02x%02x%02x%02x%02x%02x "
		   "s-s: %02x%02x%02x%02x%02x%02x%02x%02x\n",
		   client->master_key[0], client->master_key[1], client->master_key[2], client->master_key[3],
		   client->master_key[4], client->master_key[5], client->master_key[6], client->master_key[7],
		   client->master_salt[0], client->master_salt[1], client->master_salt[2], client->master_salt[3],
		   client->master_salt[4], client->master_salt[5], client->master_salt[6], client->master_salt[7],
		   server.master_key[0], server.master_key[1], server.master_key[2], server.master_key[3],
		   server.master_key[4], server.master_key[5], server.master_key[6], server.master_key[7],
		   server.master_salt[0], server.master_salt[1], server.master_salt[2], server.master_salt[3],
		   server.master_salt[4], server.master_salt[5], server.master_salt[6], server.master_salt[7]);
	if (flag_testing)
	{
		for (size_t k = 0; k < 16; k++)
		{
			client->master_key[k] = master_key[k];
		}
		for (size_t k = 0; k < 14; k++)
		{
			client->master_salt[k] = master_salt[k];
		}
	}
	printf("DTLS-SRTP successfully negotiated\n");
	return 0;

error:
	return -1;
}
int dtls(struct pthread_arguments *p_a, unsigned char *s, unsigned int len)
{
	//struct stream_fd *sfd, const str *s, const endpoint_t *fsin
	printf("Dtls\n");
	struct sockaddr_in stun_to_browser;
	int ret;
	unsigned char buf[0x10000];

	struct dtls_connection *d = &p_a->dtls_cert;
	if (!d)
		return 0;
	if (s)
		printf("dtls packet input: len %u %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			   len,
			   (unsigned char)s[0], (unsigned char)s[1], (unsigned char)s[2], (unsigned char)s[3],
			   (unsigned char)s[4], (unsigned char)s[5], (unsigned char)s[6], (unsigned char)s[7],
			   (unsigned char)s[8], (unsigned char)s[9], (unsigned char)s[10], (unsigned char)s[11],
			   (unsigned char)s[12], (unsigned char)s[13], (unsigned char)s[14], (unsigned char)s[15]);

	if (s)
	{
		printf("Processing incoming DTLS packet\n");
		BIO_write(d->r_bio, s, len);
	}

	ret = try_connect(d);
	if (ret == -1)
	{
		printf("%s:%d:Error with try_connection\n", __func__, __LINE__);
		dtls_connection_cleanup(d);
		return 0;
	}
	else if (ret == 1)
	{
		printf("Try_connection: Successful\n");
		dtls_setup_crypto(d, &p_a->crypto);
	}

	while (1)
	{
		ret = BIO_ctrl_pending(d->w_bio);
		if (ret <= 0)
			break;

		if (ret > sizeof(buf))
		{
			printf("BIO buffer overflow\n");
			(void)BIO_reset(d->w_bio);
			break;
		}

		ret = BIO_read(d->w_bio, buf, ret);
		if (ret <= 0)
			break;

		printf("dtls packet output: len %u %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			   ret,
			   buf[0], buf[1], buf[2], buf[3],
			   buf[4], buf[5], buf[6], buf[7],
			   buf[8], buf[9], buf[10], buf[11],
			   buf[12], buf[13], buf[14], buf[15]);

		printf("Sending DTLS packet\n");

		memset(&stun_to_browser, 0, sizeof(stun_to_browser));
		stun_to_browser.sin_family = AF_INET;					 // IPv4
		inet_aton(p_a->ip_browser, &stun_to_browser.sin_addr);   /// Address browser
		stun_to_browser.sin_port = htons(p_a->port_ice_browser); /// Port browser

		sendto(p_a->socket_stream, (char *)buf, ret, 0, (struct sockaddr *)&stun_to_browser, sizeof(stun_to_browser));
	}

	return 1;
}
static void aes_ctr(unsigned char *out, struct str_key *in, EVP_CIPHER_CTX *ecc, const unsigned char *iv)
{
	unsigned char ivx[16];
	unsigned char key_block[16];
	unsigned char *p, *q;
	unsigned int left;
	int outlen, i;
	u_int64_t *pi, *qi, *ki;
	//printf("Start aes\n");

	memcpy(ivx, iv, 16);
	pi = (uint64_t *)in->str;
	qi = (uint64_t *)out;
	ki = (uint64_t *)key_block;
	left = in->len;
	if(flag_testing)
	{
		printf("IVX1 --- ");
		printText(ivx, 16);
	}
	while (left)
	{
		EVP_EncryptUpdate(ecc, key_block, &outlen, ivx, 16);
		if(flag_testing)
		{
			printf("IVX2 --- ");
			printText(ivx, 16);
			printf("Key_block --- ");
			printText(key_block, 16);
		}
		if (left < 16)
		{
			p = (unsigned char *)pi;
			q = (unsigned char *)qi;
			for (i = 0; i < 16; i++)
			{
				*q++ = *p++ ^ key_block[i];
				left--;
				if (!left)
					goto done;
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
	//printf("End aes\n");
	;
}
/* rfc 3711 section 4.1 */
static int aes_cm_encrypt_rtp(struct crypto_context *c, struct str_key *payload, uint32_t ssrc, uint64_t idx)
{
	unsigned char iv[16];
	u_int32_t *ivi;
	u_int32_t idxh, idxl;

	memcpy(iv, c->session_salt, 14);
	iv[14] = iv[15] = '\0';
	ivi = (u_int32_t *)iv;
	idx <<= 16;
	idxh = htonl((idx & 0xffffffff00000000ULL) >> 32);
	idxl = htonl(idx & 0xffffffffULL);

	ivi[1] ^= htonl(ssrc);
	ivi[2] ^= idxh;
	ivi[3] ^= idxl;
	if(flag_testing)
		printf("SSRC = %u --- IvI[1] = %02x --- IVI[2] = %02x --- IvI[3] = %02x INDEX ---> %lu\n", htonl(ssrc), ivi[1], ivi[2], ivi[3], idx);
	aes_ctr((unsigned char *)payload->str, payload, (EVP_CIPHER_CTX *)c->session_key_ctx[0], iv);
}

/* rfc 3711 sections 3.4 and 4.1 */
static int aes_cm_encrypt_rtcp(struct crypto_context *c, unsigned char *buf, uint32_t ssrc, uint64_t index)
{
	/*unsigned char iv[16];
	u_int32_t *ivi;
	u_int32_t idxh, idxl;

	memcpy(iv, c->session_salt, 14);
	iv[14] = iv[15] = '\0';
	ivi = (void *) iv;
	idx <<= 16;
	idxh = htonl((idx & 0xffffffff00000000ULL) >> 32);
	idxl = htonl(idx & 0xffffffffULL);

	ivi[1] ^= ssrc;
	ivi[2] ^= idxh;
	ivi[3] ^= idxl;

	aes_ctr((void *) s->s, s, c->session_key_ctx[0], iv);*/
	printf("aes_cm_encrypt_rtcp\n");
}

/* rfc 3711, sections 4.2 and 4.2.1 */
static int hmac_sha1_rtp(struct crypto_context *c, unsigned char *payload, struct str_key *in, uint64_t index)
{
	unsigned char hmac[20];
	u_int32_t roc;
	HMAC_CTX *hc;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	hc = HMAC_CTX_new();
#else
	HMAC_CTX hc_s;
	HMAC_CTX_init(&hc_s);
	hc = &hc_s;
#endif

	HMAC_Init_ex(hc, c->session_auth_key, c->params.crypto_suite->srtp_auth_key_len, EVP_sha1(), NULL);
	HMAC_Update(hc, (unsigned char *)in->str, in->len);
	roc = htonl((index & 0xffffffff0000ULL) >> 16);
	HMAC_Update(hc, (unsigned char *)&roc, sizeof(roc));
	HMAC_Final(hc, hmac, NULL);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	HMAC_CTX_free(hc);
#else
	HMAC_CTX_cleanup(hc);
#endif

	memcpy(payload, hmac, c->params.crypto_suite->srtp_auth_tag);
	printf("hmac_sha1_rtp\n");
	return 0;
}

/* rfc 3711, sections 4.2 and 4.2.1 */
static int hmac_sha1_rtcp(struct crypto_context *c, unsigned char *buf)
{
	/*
	unsigned char hmac[20];

	HMAC(EVP_sha1(), c->session_auth_key, c->params.crypto_suite->srtcp_auth_key_len,
			(unsigned char *) in->s, in->len, hmac, NULL);

	assert(sizeof(hmac) >= c->params.crypto_suite->srtcp_auth_tag);
	memcpy(out, hmac, c->params.crypto_suite->srtcp_auth_tag);
	*/
	printf("hmac_sha1_rtcp\n");
	return 0;
}

static int aes_cm_session_key_init(struct crypto_context *c)
{
	evp_session_key_cleanup(c);

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	c->session_key_ctx[0] = EVP_CIPHER_CTX_new();
#else
	c->session_key_ctx[0] = g_slice_alloc(sizeof(EVP_CIPHER_CTX));
	EVP_CIPHER_CTX_init(c->session_key_ctx[0]);
#endif
	EVP_EncryptInit_ex((EVP_CIPHER_CTX *)c->session_key_ctx[0], (const EVP_CIPHER *)c->params.crypto_suite->lib_cipher_ptr, NULL,
					   (unsigned char *)c->session_key, NULL);
	return 0;
}

static int evp_session_key_cleanup(struct crypto_context *c)
{
	unsigned char block[16];
	int len, i;

	for (i = 0; i < 2; i++)
	{
		if (!c->session_key_ctx[i])
			continue;

		EVP_EncryptFinal_ex((EVP_CIPHER_CTX *)c->session_key_ctx[i], block, &len);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
		EVP_CIPHER_CTX_free((EVP_CIPHER_CTX *)c->session_key_ctx[i]);
#else
		EVP_CIPHER_CTX_cleanup(c->session_key_ctx[i]);
		g_slice_free1(sizeof(EVP_CIPHER_CTX), c->session_key_ctx[i]);
#endif
		c->session_key_ctx[i] = NULL;
	}
	return 0;
}

void crypto_init_main()
{
	struct crypto_suite *cs;
	for (unsigned int i = 0; i < num_crypto_suites; i++)
	{
		cs = &c_suites[i];
		cs->idx = i;
		switch (cs->master_key_len)
		{
		case 16:
			cs->lib_cipher_ptr = EVP_aes_128_ecb();
			break;
		case 24:
			cs->lib_cipher_ptr = EVP_aes_192_ecb();
			break;
		case 32:
			cs->lib_cipher_ptr = EVP_aes_256_ecb();
			break;
		}
	}
}
int readDTLS(int fd, unsigned char *dtls_answers)
{
	struct msghdr msg;
	struct iovec iov;
	char ctrl[64];
	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	iov.iov_base = dtls_answers;
	iov.iov_len = DTLS_MESSAGES;
	int answer = recvmsg(fd, &msg, 0);
	return answer;
}
