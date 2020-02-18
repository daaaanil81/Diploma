#include "crypto.h"
void aes_ctr(unsigned char *out, struct str_key *in, EVP_CIPHER_CTX *ecc, const unsigned char *iv)
{
	unsigned char ivx[16];
	unsigned char key_block[16];
	unsigned char *p, *q;
	unsigned int left;
	int outlen, i;
	uint64_t *pi, *qi, *ki;
	printf("Start aes\n");
	printf("Size: %d\n", in->len);
	memcpy(ivx, iv, 16);
	pi = (uint64_t *)in->str;
	qi = (uint64_t *)out;
	ki = (uint64_t *)key_block;
	left = in->len;

	while (left)
	{
		EVP_EncryptUpdate(ecc, key_block, &outlen, ivx, 16);
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
void aes_ctr_no_ctx(unsigned char *out, struct str_key *in, const unsigned char *key, const EVP_CIPHER *ciph,
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
void prf_n(struct str_key *out, const unsigned char *key, const EVP_CIPHER *ciph, const unsigned char *x)
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