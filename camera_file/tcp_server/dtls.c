#include "dtls.h"

int cert_init(struct pthread_arguments* p_a)
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
				(unsigned char *) "danil_test", -1, -1, 0))
		goto err;

	if (!X509_set_subject_name(p_a->x509, p_a->name))
		goto err;

	if (!X509_set_issuer_name(p_a->x509, p_a->name))
		goto err;

	/* cert lifetime */

	if (!X509_gmtime_adj(X509_get_notBefore(p_a->x509), -60*60*24))
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
static unsigned int generic_func(unsigned char *o, X509 *x, const EVP_MD *md) {
	unsigned int n;
	X509_digest(x, md, o, &n);
	return n;
}

static unsigned int sha_1_func(unsigned char *o, X509 *x) {
	const EVP_MD *md;
	md = EVP_sha1();
	return generic_func(o, x, md);
}
static void dump_cert(struct pthread_arguments *cert) {
	FILE *fp;
	char *buf;
	size_t len;

	/* cert */
	fp = open_memstream(&buf, &len);
	PEM_write_X509(fp, cert->x509);
	fclose(fp);

	buf_dump_free(buf, len);

	/* key */
	fp = open_memstream(&buf, &len);
	PEM_write_PrivateKey(fp, cert->pkey, NULL, NULL, 0, 0, NULL);
	fclose(fp);

	buf_dump_free(buf, len);
}
void dtls_fingerprint_free(struct pthread_arguments *cert)
{
	BN_free(cert->exponent);
	BN_free(cert->serial_number);
	X509_NAME_free(cert->name);
	if (cert->pkey)
		EVP_PKEY_free(cert->pkey);
	if (cert->x509)
		X509_free(cert->x509);
}
static void buf_dump_free(char *buf, size_t len) {
	char *p, *f;
	int llen;

	p = buf;
	while (len) {
		f = (char* )memchr(p, '\n', len);
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
