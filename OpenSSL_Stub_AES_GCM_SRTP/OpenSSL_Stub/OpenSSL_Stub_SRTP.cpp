//Triel-1
//Case-1 : RTCP 27 - Able to Decrypt 
//case 2 : RTCP 28 - Not able to Decrypt - Auth Fail

//Triel-2
//RTCP 28 Auth Tag Check Failing

/* Init libsrtp */

#include <srtp.h>
#include <crypto_kernel.h>
#include <iostream>
#include <crypto\include\cipher_types.h>
#include <crypto\cipher\aes_gcm_ossl.c>

#define KN_MAX_KEY_LEN 128
#define KN_MKI_VAL_SIZE 4

struct str_st
{
	/** Buffer pointer, which is by convention NOT null terminated. */
	char ptr[64];

	/** The length of the string. */
	int  slen;
};

//KN_TP_I_MSG_TYPE enum also needs to be updated on below enum changes.
typedef enum _MSI_I_MESSAGE_TYPE {
	MSI_I_MESSAGE_TYPE_INVALID,
	MSI_I_MESSAGE_TYPE_CSK,
	MSI_I_MESSAGE_TYPE_PCK,
	MSI_I_MESSAGE_TYPE_T_GMK,
	MSI_I_MESSAGE_TYPE_GMK,
	MSI_I_MESSAGE_TYPE_MAX
}MSI_I_MESSAGE_TYPE;

typedef struct srtp_policy_neg
{
	/** Optional key. If empty, a random key will be autogenerated. */
	str_st	key;
	/** Crypto name.   */
	str_st	name;
	/** Flags, bitmask from #pjmedia_srtp_crypto_option */
	unsigned	flags;
	/*Need this as part of ROC changes , to distinguish between Keys*/
	MSI_I_MESSAGE_TYPE eKeyType;
} srtp_policy_neg;


typedef struct srtp_crypto_info
{
	srtp_policy_neg  tx_policy;
	srtp_policy_neg  rx_policy;

	/* Temporary policy for negotiation */
	srtp_policy_neg tx_policy_neg;
	srtp_policy_neg rx_policy_neg;

	unsigned char		tx_key[KN_MAX_KEY_LEN];
	unsigned char		rx_key[KN_MAX_KEY_LEN];
	unsigned char		tx_rx_key[KN_MAX_KEY_LEN]; //Media RTP Master Key Only

	unsigned char	mki_idVal[2 * KN_MKI_VAL_SIZE];
	unsigned int	mki_idSize;
	unsigned char	csk_mki_idVal[KN_MKI_VAL_SIZE];
	unsigned int	csk_mki_idSize;

	/* libSRTP contexts */
	void* srtcp_tx_rx_ctx;	/*RTCP transmission context*/

} srtp_crypto_info;

typedef void(*crypto_method_t)(srtp_crypto_policy_t* policy);

typedef struct crypto_suite
{
	const char* name;
	srtp_cipher_type_id_t cipher_type;
	unsigned		 cipher_key_len;    /* key + salt length    */
	unsigned		 cipher_salt_len;   /* salt only length	    */
	srtp_auth_type_id_t	 auth_type;
	unsigned		 auth_key_len;
	unsigned		 srtp_auth_tag_len;
	unsigned		 srtcp_auth_tag_len;
	srtp_sec_serv_t	 service;
	/* This is an attempt to validate crypto support by libsrtp, i.e: it should
	* raise linking error if the libsrtp does not support the crypto.
	*/
	const srtp_cipher_type_t* ext_cipher_type;
	crypto_method_t      ext_crypto_method;
} crypto_suite;

extern const srtp_cipher_type_t srtp_aes_gcm_128_openssl;

/* https://www.iana.org/assignments/sdp-security-descriptions/sdp-security-descriptions.xhtml */
static crypto_suite crypto_suites[] = {
	/* plain RTP/RTCP (no cipher & no auth) */
	{ "NULL", SRTP_NULL_CIPHER, 0, SRTP_NULL_AUTH, 0, 0, 0, sec_serv_none },
	/* cipher AES_GCM, NULL auth, auth tag len = 16 octets */
	{ "AEAD_AES_128_GCM", SRTP_AES_GCM_128, 28, 12,
	SRTP_NULL_AUTH, 0, 16, 16, sec_serv_conf_and_auth,
	&srtp_aes_gcm_128_openssl },

	/* cipher AES_GCM, NULL auth, auth tag len = 16 octets */
	{ "AEAD_AES_128_GCM", SRTP_AES_GCM_128, 28, 12,
	SRTP_NULL_AUTH, 0, 16, 16, sec_serv_conf_and_auth,
	&srtp_aes_gcm_128_openssl },

	/* cipher AES_GCM, NULL auth, auth tag len = 8 octets */
	/*{ "AEAD_AES_128_GCM_8", SRTP_AES_GCM_128, 28, 12,
	SRTP_NULL_AUTH, 0, 8, 8, sec_serv_conf_and_auth,
	&srtp_aes_gcm_128_openssl },*/
};

int srtp_lib_init();

int srtp_start(srtp_crypto_info* srtp_info, unsigned int client_ssrc);

void print_buffer(const char* name, unsigned char* data, int len);

int main()
{

	srtp_crypto_info srtp_cinfo = { 0 };
	srtp_crypto_info* srtp_info = &srtp_cinfo;
	int pkt_len = 0;
	int use_mki = 1;
	/*
	//Triel1
	//RTCP 27/28 
	unsigned int client_ssrc = 1800959702;
	//RTCP 27/28
	char MasterKey[] = { 0x04, 0xb8, 0x7b, 0xd0, 0xc4, 0xba, 0x0c, 0x79, 0xbc, 0xf8, 0xe9, 0xcc, 0xce, 0x19, 0x67, 0x45, 0xaf, 0x37, 0xa9, 0x19, 0xa5, 0x65, 0xe1, 0x57, 0x79, 0xc5, 0xeb, 0xe3 };
	//RTCP 27-116 BYTES
	//char pkt[] = { 0x9b, 0xcc, 0x00, 0x16, 0x6b, 0x58, 0x76, 0xd6, 0x61, 0x9a, 0x80, 0xf9, 0x5b, 0x16, 0xad, 0x7b, 0xed, 0x17, 0xd7, 0x4b, 0x49, 0xa0, 0x40, 0x11, 0xdc, 0xa5, 0x3b, 0xe5, 0xc3, 0x22, 0xc6, 0xb5, 0x07, 0x07, 0x4c, 0xea, 0x04, 0xe7, 0x46, 0x65, 0xb2, 0x58, 0x0b, 0x84, 0x6d, 0x7d, 0x38, 0x30, 0xba, 0xcb, 0x25, 0x77, 0x09, 0x58, 0x6d, 0xf5, 0xf0, 0x22, 0x8c, 0x95, 0x92, 0x3e, 0x02, 0x1b, 0xba, 0x92, 0x18, 0xcc, 0x42, 0xa3, 0x91, 0x4a, 0xb2, 0x1b, 0x73, 0x88, 0x0a, 0x2e, 0x9c, 0x4f, 0xe2, 0xc4, 0x17, 0x08, 0x2c, 0xef, 0xce, 0x6c, 0x6b, 0x8c, 0x1a, 0x7a, 0x3f, 0xb3, 0x06, 0x8e, 0xb9, 0xac, 0x60, 0x56, 0x70, 0x15, 0x10, 0x76, 0x50, 0xe6, 0x8f, 0x81, 0x80, 0x00, 0x00, 0x01, 0x24, 0x9b, 0x6f, 0xb7 };
	//RTCP 28-68 BYTES - FAILING
	char pkt[] = { 0x9C, 0xCC, 0x00, 0x0A, 0x6B, 0x58, 0x76, 0xD6, 0xB9, 0x72, 0x59, 0xA5, 0xB2, 0x0A, 0x02, 0xD2, 0xF9, 0x31, 0xED, 0xF2, 0x48, 0x6D, 0x42, 0xBB, 0x1A, 0x52, 0x43, 0x8E, 0x34, 0x92, 0x4E, 0xAE, 0x43, 0xC4, 0x72, 0x40, 0x4D, 0x75, 0xF5, 0xEA, 0x8B, 0x89, 0x32, 0xBA, 0x29, 0xFC, 0xD0, 0x48, 0x3C, 0x20, 0x76, 0x4D, 0x89, 0xF1, 0xD6, 0x98, 0xF2, 0x3D, 0xE2, 0xDD, 0x80, 0x00, 0x00, 0x02, 0x24, 0x9B, 0x6F, 0xB7 };
	//RTCP 27/28
	unsigned char CSKID[5] = { 0x24, 0x9b, 0x6f, 0xb7 };
	//pkt_len = 116;	//RTCP 27
	pkt_len = 68;		//RTCP 28
	*/



	//Triel-2 RTCP 28 Failing Test
	unsigned int client_ssrc = 2801858373;
	char MasterKey[] = { 0x0b, 0xc9, 0x12, 0xb7, 0x16, 0x0e, 0x70, 0x98, 0xc9, 0xfe, 0xc5, 0xfc, 0xf9, 0xbc, 0x8a, 0x01, 0xf5, 0x2c, 0x86, 0x28, 0x2d, 0x60, 0xf7, 0xe3, 0xda, 0x86, 0x09, 0xd0 };
	//RTCP 28 - 68 BYTES
	unsigned char pkt[] = { 0x9C, 0xCC, 0x00, 0x0A, 0xA7, 0x00, 0xF7, 0x45, 0xFD, 0xE5, 0xAE, 0xDA, 0xB9, 0xBD, 0x05, 0x7C, 0x6B, 0x1C, 0xD1, 0x24, 0x11, 0xF5, 0xFA, 0x7C, 0x8A, 0x4B, 0x4B, 0xEE, 0xDF, 0x03, 0x41, 0x6D, 0x09, 0xE7, 0x38, 0x4A, 0x67, 0x21, 0xBF, 0x9C, 0xCC, 0xB8, 0x17, 0x76, 0xBB, 0xEB, 0x3F, 0xA0, 0x80, 0xBA, 0xC7, 0x46, 0xFC, 0xCD, 0x61, 0x40, 0x4C, 0xEC, 0x46, 0x0C, 0x80, 0x00, 0x00, 0x02, 0x2C, 0xFE, 0x38, 0x54 };
	unsigned char CSKID[5] = { 0x2c, 0xfe, 0x38, 0x54 };
	pkt_len = 68;


	//Setting the Properties to create SRTP Context
	strncpy((char*)srtp_info->csk_mki_idVal, (const char*)CSKID, 4);
	srtp_info->csk_mki_idSize = 4;

	memset(&srtp_info->tx_policy_neg, 0, sizeof(srtp_info->tx_policy_neg));
	memset(&srtp_info->rx_policy_neg, 0, sizeof(srtp_info->rx_policy_neg));

	srtp_info->tx_policy_neg.flags = 0;
	strcpy(srtp_info->tx_policy_neg.name.ptr, "AEAD_AES_128_GCM");
	srtp_info->tx_policy_neg.name.slen = 16;
	strcpy(srtp_info->tx_policy_neg.key.ptr, MasterKey);
	srtp_info->tx_policy_neg.key.slen = 28;

	srtp_info->rx_policy_neg.flags = 0;
	strcpy(srtp_info->rx_policy_neg.name.ptr, "AEAD_AES_128_GCM");
	srtp_info->rx_policy_neg.name.slen = 16;
	strcpy(srtp_info->rx_policy_neg.key.ptr, MasterKey);
	srtp_info->rx_policy_neg.key.slen = 28;

	srtp_info->tx_policy_neg.eKeyType = srtp_info->rx_policy_neg.eKeyType = MSI_I_MESSAGE_TYPE_CSK;

	//Init SRTP & Creating Context
	srtp_lib_init();

	srtp_start(srtp_info, client_ssrc);

	srtp_err_status_t srtp_lib_err = srtp_err_status_ok;

	print_buffer("\nPacket Data (Encrypted by WolfSSL):", pkt, pkt_len);

	//Decrypting Packet
	srtp_lib_err = srtp_unprotect_rtcp_mki((srtp_t)srtp_info->srtcp_tx_rx_ctx, pkt, &pkt_len, use_mki);
	if (srtp_lib_err != srtp_err_status_ok)
	{
		printf("\nPacket Decrypt Fail\n");
		return -1;
	}

	print_buffer("\nPacket Data (Decrypted by OpenSSL):", pkt, pkt_len);
}


int srtp_lib_init()
{
	srtp_err_status_t err;
	srtp_crypto_info srtp_info;

	err = srtp_init();
	if (err != srtp_err_status_ok) {
		printf("\n\nFailed to initialize libsrtp");
		return -1;
	}

}

int srtp_start(srtp_crypto_info* srtp_info, unsigned int client_ssrc)
{
	srtp_policy_t    tx_;
	srtp_policy_t    rx_;
	srtp_master_key_t mKeys[2] = { 0 }, * pmKeys = NULL;
	srtp_err_status_t err;

	int		     cr_tx_idx = 0;
	int		     au_tx_idx = 0;
	int		     cr_rx_idx = 0;
	int		     au_rx_idx = 0;

	cr_tx_idx = au_tx_idx = 1; // 1 for AES GCM 128
	cr_rx_idx = au_rx_idx = 1; // 1 for AES GCM 128

	/* Init transmit direction */
	memset(&tx_, 0, sizeof(srtp_policy_t));
	memmove(srtp_info->tx_key, srtp_info->tx_policy_neg.key.ptr, srtp_info->tx_policy_neg.key.slen);

	if (cr_tx_idx && au_tx_idx)
		tx_.rtp.sec_serv = sec_serv_conf_and_auth;
	else if (cr_tx_idx)
		tx_.rtp.sec_serv = sec_serv_conf;
	else if (au_tx_idx)
		tx_.rtp.sec_serv = sec_serv_auth;
	else
		tx_.rtp.sec_serv = sec_serv_none;

	/*MKI changes START*/
	mKeys[0].key = (uint8_t*)srtp_info->tx_key;
	mKeys[0].mki_id = srtp_info->csk_mki_idVal;
	mKeys[0].mki_size = srtp_info->csk_mki_idSize;
	pmKeys = &mKeys[0];
	tx_.keys = &pmKeys; // &mKeys[0];
	tx_.num_master_keys = 1;
	/*MKI changes END*/

	tx_.ssrc.type = ssrc_specific;
	tx_.ssrc.value = client_ssrc;

	tx_.rtp.cipher_type = crypto_suites[cr_tx_idx].cipher_type;
	tx_.rtp.cipher_key_len = crypto_suites[cr_tx_idx].cipher_key_len;
	tx_.rtp.auth_type = crypto_suites[au_tx_idx].auth_type;
	tx_.rtp.auth_key_len = crypto_suites[au_tx_idx].auth_key_len;
	tx_.rtp.auth_tag_len = crypto_suites[au_tx_idx].srtp_auth_tag_len;
	tx_.rtcp = tx_.rtp;
	tx_.rtcp.auth_tag_len = crypto_suites[au_tx_idx].srtcp_auth_tag_len;
	tx_.next = &rx_;

	srtp_info->tx_policy = srtp_info->tx_policy_neg;

	//printf("\nTx => tx_policy.name [%.*s]", srtp_info->tx_policy.name.slen, srtp_info->tx_policy.name.ptr);
	//printf("\nTx => auth_type [%d]", tx_.rtp.auth_type);
	//printf("\nTx => auth_key_len [%d]", tx_.rtp.auth_key_len);
	//printf("\nTx => auth_tag_len [%d]", tx_.rtp.auth_tag_len);

	/* Init receive direction */
	memset(&rx_, 0, sizeof(srtp_policy_t));
	memmove(srtp_info->rx_key, srtp_info->rx_policy_neg.key.ptr, srtp_info->rx_policy_neg.key.slen);

	if (cr_rx_idx && au_rx_idx)
		rx_.rtp.sec_serv = sec_serv_conf_and_auth;
	else if (cr_rx_idx)
		rx_.rtp.sec_serv = sec_serv_conf;
	else if (au_rx_idx)
		rx_.rtp.sec_serv = sec_serv_auth;
	else
		rx_.rtp.sec_serv = sec_serv_none;

	/*MKI changes START*/
	mKeys[0].key = (uint8_t*)srtp_info->rx_key;
	mKeys[0].mki_id = srtp_info->csk_mki_idVal;
	mKeys[0].mki_size = srtp_info->csk_mki_idSize;
	pmKeys = &mKeys[0];
	rx_.keys = &pmKeys; // &mKeys[0];
	rx_.num_master_keys = 1;
	/*MKI changes END*/

	rx_.ssrc.type = ssrc_any_inbound;
	rx_.ssrc.value = 0;
	rx_.rtp.sec_serv = crypto_suites[cr_rx_idx].service;
	rx_.rtp.cipher_type = crypto_suites[cr_rx_idx].cipher_type;
	rx_.rtp.cipher_key_len = crypto_suites[cr_rx_idx].cipher_key_len;
	rx_.rtp.auth_type = crypto_suites[au_rx_idx].auth_type;
	rx_.rtp.auth_key_len = crypto_suites[au_rx_idx].auth_key_len;
	rx_.rtp.auth_tag_len = crypto_suites[au_rx_idx].srtp_auth_tag_len;
	rx_.rtcp = rx_.rtp;
	rx_.rtcp.auth_tag_len = crypto_suites[au_rx_idx].srtcp_auth_tag_len;
	rx_.next = NULL;

	err = srtp_create((srtp_t*)&srtp_info->srtcp_tx_rx_ctx, &tx_);
	if (err != srtp_err_status_ok) {
		return -1;
	}

	srtp_info->rx_policy = srtp_info->rx_policy_neg;

	//printf("\nRx => rx_policy.name [%.*s]", srtp_info->rx_policy.name.slen, srtp_info->rx_policy.name.ptr);
	//printf("\nRx => auth_type [%d]", rx_.rtp.auth_type);
	//printf("\nRx => auth_key_len [%d]", rx_.rtp.auth_key_len);
	//printf("\nRx => auth_tag_len [%d]", rx_.rtp.auth_tag_len);
}

//Print Buffer in HEX
void print_buffer(const char* name, unsigned char* data, int len)
{
	int i; printf("%s", name); for (i = 0; i < len; i++) { if ((i % 8) == 0) printf("\n "); printf("0x%02x, ", data[i]); } printf("\n");
}