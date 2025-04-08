/*
 *SPDX-License-Identifier: GPL-2.0+
 */
#ifndef __SUNXI_CERT_INTERFACE__
#define __SUNXI_CERT_INTERFACE__

#define RSA_2048_BIT 2048
#define RSA_3072_BIT 3072
#define RSA_4096_BIT 4096

#define ALIGN_BYTE 8
#define KEY_STRING_LEN (RSA_4096_BIT >> 3)
#define RSA_MOD_MAX_LEN_BYTE (RSA_4096_BIT >> 3)

#define ECC_P521_BIT 521
#define ECC_P384_BIT 384
#define ECC_P256_BIT 256

#define ECC_MAX_BIT (521 + 7) //maye be padding
#define RSA_MAX_BIT 4096
#define RSA_MAX_BYTE (RSA_MAX_BIT >> 3)
#define ECC_MAX_BYTE (ECC_MAX_BIT >> 3)

#define RSA_PADING_PKCSV1_5 0
#define RSA_PADING_PSS 1
#define RSA_PADING_OAEP 2

typedef enum KeyFileType {
	KEYFILE_TYPE_BIN,
	KEYFILE_TYPE_PEM,
} KeyFileType_t;

typedef enum FlagType {
	FLAG_TYPE_ARRAY,
	FLAG_TYPE_STRING,
} FlagType_t;

typedef enum SignType {
	SIGN_TYPE_RSA,
	SIGN_TYPE_ECC,
	SIGN_TYPE_MAX,
} SignType_t;

#define AW_CERT_MAX_SIZE	(4096)

#define AW_CERT_PK_INFO_SIZE	(1036)

#define AW_CERT_NAME_SIZE	16
#define DEFAULT_ISSUER_NAME		"allwinner"
#define DEFAULT_SUBJECT_NAME	"allwinner"

#define AW_CERT_TIME_SIZE		12

#define AW_CERT_FORMAT_SIGN		"AW_SIGN!"
#define AW_CERT_MAGIC_SIZE	8
#define AW_CERT_HEAD_VERSION		"V1.0.0"
#define AW_CERT_HEAD_VERSION_SIZE		4

#define AW_CERT_VALIDITY		(30)

typedef struct _aw_cert_rsa_pk_info_t {
	u32 rsa_padding_mode;			//PSS/NOPANDING
	u32 n_len;						//byte
	u32 e_len;						//byte
	u8 n[RSA_MAX_BYTE];				//512
	u8 e[RSA_MAX_BYTE];				//512
} aw_cert_rsa_pk_info_t;			//1036

typedef struct _aw_cert_ecc_pk_info_t {
	u32 p_len;
	u32 a_len;
	u32 b_len;
	u32 n_len;
	u32 k_len;
	u32 gx_len;
	u32 gy_len;
	u32 qx_len;
	u32 qy_len;
	u8 p[ECC_MAX_BYTE];
	u8 a[ECC_MAX_BYTE];
	u8 b[ECC_MAX_BYTE];
	u8 n[ECC_MAX_BYTE];
	u8 k[ECC_MAX_BYTE];
	u8 gx[ECC_MAX_BYTE];
	u8 gy[ECC_MAX_BYTE];
	u8 qx[ECC_MAX_BYTE];
	u8 qy[ECC_MAX_BYTE];
} aw_cert_ecc_pk_info_t; //598

typedef struct _aw_cert_ecc_sign_t {
	u32 r_len;
	u32 s_len;
	u8 r[ECC_MAX_BYTE];
	u8 s[ECC_MAX_BYTE];
} aw_cert_ecc_sign_t;

typedef struct _aw_cert_t {
	u8 magic[AW_CERT_MAGIC_SIZE];				//”AW_SIGN!”  2
	u8 head_version[AW_CERT_HEAD_VERSION_SIZE];		//“V1.0.0”
	u32 head_size;									//total size of cert

	u32 algorithm_type;								//rsa/ecc
	u32 sign_hash_size;								//sha256/sha384/sha512
	u32 nvc;										//rollback index 3
	u32 serial_num;

	u8	issuer_name[AW_CERT_NAME_SIZE];			//the name of issuer 4

	u8	subject_name[AW_CERT_NAME_SIZE];		//the name of subject 5

	u8	validity_start[AW_CERT_TIME_SIZE];			//the date of create cert
    u8	validity_end[AW_CERT_TIME_SIZE];				//the validity date of cert 6 + 8bytes
	u32 public_key_size;							//public key size(bit)   5 + 8bytes
	u8	public_key_info[AW_CERT_PK_INFO_SIZE];	//the info of public key
	u32	sboot_hash_size;							//sha256/sha384/sha512
	u8	sboot_hash[64];								//the hash of sboot
	u8	reserve[512];
	u32 verify_block_offset;						//the offset of some block must be verify
	u32 verify_block_size;							//block size
	u32 sign_size;									//the sign size
	u8	sign[RSA_MAX_BYTE];
} aw_cert_t;

/* Be sure to be consistent with the sign tool */
typedef struct __attribute__((packed)) _aw_cert_extern_t {
	u8 name[7];
	u8 value[32];
} aw_cert_extern_t;

#define RSA_BIT_WITDH 2048

#define SUNXI_EXTENSION_ITEM_MAX (8)

typedef struct {
	int extension_num;
	u8 *name[SUNXI_EXTENSION_ITEM_MAX];
	uint name_len[SUNXI_EXTENSION_ITEM_MAX];
	u8 *value[SUNXI_EXTENSION_ITEM_MAX];
	uint value_len[SUNXI_EXTENSION_ITEM_MAX];
} sunxi_extension_t;

typedef struct {
	u8 *n;
	u32 n_len;
	u8 *e;
	u32 e_len;
} sunxi_key_t;

typedef struct {
	u32 public_key_size;
	u8 *public_key_info;
} aw_cert_info_t;

typedef struct {
	long version;
	long serial_num;
	sunxi_key_t pubkey; //x509
	aw_cert_info_t aw_cert_info;
	sunxi_extension_t extension;
} sunxi_certif_info_t;

#define SUNXI_EXTENSION_ITEM_MAX (8)
#define SUNXI_X509_CERTIFF_MAX_LEN 4096

int sunxi_certif_verify_itself(sunxi_certif_info_t *sunxi_certif, u8 *buf,
			       u32 len);
int sunxi_certif_pubkey_hash_cal(sunxi_certif_info_t *sunxi_certif, u8 *hash_buf);

int sunxi_certif_verify_subcert_pubkey(sunxi_certif_info_t *root_certif,
					sunxi_certif_info_t *sub_certif,
					const char *name);

int aw_certif_probe_extern(sunxi_certif_info_t *sunxi_certif,
				aw_cert_t *p_toc1_cert);
#endif //__SUNXI_CERT_INTERFACE__
