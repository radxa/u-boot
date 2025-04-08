
/*
 *SPDX-License-Identifier: GPL-2.0+
 */
#include <openssl_ext.h>
#include <asm/arch/ce.h>
#include <sunxi_board.h>
#include <sunxi_cert_interface.h>
#include <memalign.h>
#ifdef CONFIG_CRYPTO
#include <crypto/ecc_dsa.h>
#endif

#ifndef ALG_ECC
#define ALG_ECC	(0x21)
#endif

typedef struct _sunxi_certif_desc {
	int (*pubkey_hash_cal_from_sunxi_cert)(
		u8 *sub_pubkey_hash, u32 sub_pubkey_hash_size,
		sunxi_certif_info_t *sunxi_certif);
	int (*verify_itself)(sunxi_certif_info_t *sunxi_certif, u8 *buf,
			     u32 len);
	int (*aw_certif_verify_sign)(aw_cert_t *p_toc1_cert, u8 *hash_of_certif,
				     u8 hash_of_certif_len);
	int (*verify_subcert_pubkey)(sunxi_certif_info_t *root_certif,
				     u8 ext_seq,
				     sunxi_certif_info_t *sub_certif);
	// int (*aw_certif_pubkey_hash_cal)(u8 *out_buf, u32 hash_size,
	// 			u8 *public_key_info, u32 public_key_size);
} sunxi_certif_desc_t;
sunxi_certif_desc_t toc_certif_desc;
int sunxi_certif_desc_register(u8 *buf);

__weak s32 sunxi_ecc_sign_check(struct ecc_verify_params *params, u32 key_byte_size,
				u8 *hash, u32 hash_byte_len)
{
#ifdef CONFIG_CRYPTO
	u8 *pubkey, *sign;
	int ret;

	pubkey = (u8 *)malloc(params->qx_len * 2);
	memset(pubkey, 0, params->qx_len * 2);
	memcpy(pubkey, params->qx, params->qx_len);
	memcpy(pubkey + params->qx_len, params->qy, params->qy_len);

	sign = (u8 *)malloc(params->r_len * 2);
	memset(sign, 0, params->r_len * 2);
	memcpy(sign, params->r, params->r_len);
	memcpy(sign + params->r_len, params->s, params->s_len);
#ifdef SUNXI_SOFTWARE_ECC_SECP256R1
	/* use software ecc secp256r1 */
	if (uECC_verify(pubkey, hash, hash_byte_len, sign, &curve_secp256r1)) {
		ret = 0;
	} else
#elif defined(SUNXI_SOFTWARE_ECC_SECP384R1)
	/* use software ecc secp384r1 */
	if (uECC_verify(pubkey, hash, hash_byte_len, sign, &curve_secp384r1)) {
		ret = 0;
	} else
#endif  /* SUNXI_SOFTWARE_ECC_SECP256R1 */
	{
		printf("uECC_verify failed\n");
		ret = -1;
	}

	free(sign);
	free(pubkey);
	return ret;
#else
	printf("__weak sunxi_params_check: not support uECC_verify\n");
	return -1;
#endif  /* CONFIG_CRYPTO */
}

__weak s32 sunxi_rsa_sign_check(u32 mod_bit_size, u8 *key_addr, u32 key_len,
			 u8 *src_addr, u32 src_len, u8 *dest_addr, u32 want_len,
			 u8 *mod_addr)
{
	printf("__weak %s: not support rsa verify\n", __func__);
	return -1;
}

int aw_certif_probe_sunxi_certif(sunxi_certif_info_t *sunxi_certif,
				 aw_cert_t *aw_cert)
{
	memcpy((void *)&sunxi_certif->version, aw_cert->head_version,
	       sizeof(aw_cert->head_version));
	sunxi_certif->serial_num = (long)aw_cert->serial_num;
	sunxi_certif->aw_cert_info.public_key_info =
		malloc(AW_CERT_PK_INFO_SIZE);
	if (!sunxi_certif->aw_cert_info.public_key_info) {
		pr_err("malloc failed\n");
		return -1;
	}
	memcpy(sunxi_certif->aw_cert_info.public_key_info,
	       aw_cert->public_key_info, AW_CERT_PK_INFO_SIZE);
	sunxi_certif->aw_cert_info.public_key_size = aw_cert->public_key_size;

	return 0;
}
#define SELF_STRING "self"
int aw_certif_probe_extern(sunxi_certif_info_t *sunxi_certif,
			   aw_cert_t *p_toc1_cert)
{
	int count = 0;
	char all_zero[8] = { 0 };
	aw_cert_extern_t *aw_cert_extern =
		(aw_cert_extern_t *)p_toc1_cert->reserve;
	sunxi_certif->extension.name[count] = malloc(sizeof(SELF_STRING));
	if (!sunxi_certif->extension.name[count]) {
		return -1;
	}
	memcpy(sunxi_certif->extension.name[count], SELF_STRING,
	       sizeof(SELF_STRING));
	sunxi_certif->extension.value[count] =
		malloc(sizeof(p_toc1_cert->sboot_hash));
	if (!sunxi_certif->extension.value[count]) {
		return -1;
	}
	memcpy(sunxi_certif->extension.value[count], p_toc1_cert->sboot_hash,
	       sizeof(p_toc1_cert->sboot_hash));
	// memcpy(sunxi_certif->extension.name[count], aw_cert_extern->name, sizeof(aw_cert_extern->name));
	sunxi_certif->extension.name_len[count] = sizeof(aw_cert_extern->name);
	// memcpy(sunxi_certif->extension.value[count], aw_cert_extern->value, sizeof(aw_cert_extern->value));
	sunxi_certif->extension.value_len[count] = sizeof(aw_cert_extern->value);
	for (count = 1;
	     count < sizeof(p_toc1_cert->reserve) / sizeof(aw_cert_extern_t);
	     count++) {
		if (memcmp(all_zero, aw_cert_extern->name,
			   sizeof(aw_cert_extern->name))) {
			// printf("dump %c%c%c%c%c%c in  rootcert\n",
			//        *((char *)(aw_cert_extern->name)),
			//        *((char *)(aw_cert_extern->name) + 1),
			//        *((char *)(aw_cert_extern->name) + 2),
			//        *((char *)(aw_cert_extern->name) + 3),
			//        *((char *)(aw_cert_extern->name) + 4),
			//        *((char *)(aw_cert_extern->name) + 5),
			// 	   *((char *)(aw_cert_extern->name) + 6));
			// sunxi_dump(aw_cert_extern->value,
			//       sizeof(aw_cert_extern->value));
			sunxi_certif->extension.name[count] =
				malloc(sizeof(aw_cert_extern->name));
			if (!sunxi_certif->extension.name[count]) {
				return -1;
			}
			memcpy(sunxi_certif->extension.name[count],
			       aw_cert_extern->name,
			       sizeof(aw_cert_extern->name));
			sunxi_certif->extension.value[count] =
				malloc(sizeof(aw_cert_extern->value));
			if (!sunxi_certif->extension.value[count]) {
				return -1;
			}
			memcpy(sunxi_certif->extension.value[count],
			       aw_cert_extern->value,
			       sizeof(aw_cert_extern->value));
			// memcpy(sunxi_certif->extension.name[count], aw_cert_extern->name, sizeof(aw_cert_extern->name));
			sunxi_certif->extension.name_len[count] =
				sizeof(aw_cert_extern->name);
			// memcpy(sunxi_certif->extension.value[count], aw_cert_extern->value, sizeof(aw_cert_extern->value));
			sunxi_certif->extension.value_len[count] =
				sizeof(aw_cert_extern->value);
			aw_cert_extern++;
		} else {
			//all zero
			break;
		}
	}
	sunxi_certif->extension.extension_num = count;
	// printf("count = %d\n", count);
	return 0;
}
int aw_certif_probe_signature(aw_cert_t *aw_cert, u8 *sign)
{
	memcpy(sign, aw_cert->sign, aw_cert->sign_size);
	return 0;
}
static void parse_aw_cert_publickey(aw_cert_t *p_toc0_cert,
				    struct ecc_verify_params *params)
{
	aw_cert_ecc_pk_info_t *ecc_pk_info =
		(aw_cert_ecc_pk_info_t *)p_toc0_cert->public_key_info;
	params->p = ecc_pk_info->p;
	params->a = ecc_pk_info->a;
	params->n = ecc_pk_info->n;
	params->gx = ecc_pk_info->gx;
	params->gy = ecc_pk_info->gy;
	params->qx = ecc_pk_info->qx;
	params->qy = ecc_pk_info->qy;
	params->p_len = ecc_pk_info->p_len;
	params->a_len = ecc_pk_info->a_len;
	params->n_len = ecc_pk_info->n_len;
	params->gx_len = ecc_pk_info->gx_len;
	params->gy_len = ecc_pk_info->gy_len;
	params->qx_len = ecc_pk_info->qx_len;
	params->qy_len = ecc_pk_info->qy_len;
	return;
}
static void parse_aw_cert_sign(aw_cert_ecc_sign_t *ecc_sign,
			       struct ecc_verify_params *params)
{
	params->r = ecc_sign->r;
	params->s = ecc_sign->s;
	params->r_len = ecc_sign->r_len;
	params->s_len = ecc_sign->s_len;
	return;
}

static int aw_certif_rsa_verify_sign(aw_cert_t *p_toc1_cert,
				 u8 *hash_of_certif, u8 hash_of_certif_len)
{
	int ret = -1;
	aw_cert_rsa_pk_info_t *rsa_params =
		(aw_cert_rsa_pk_info_t *)p_toc1_cert->public_key_info;
	u8 hash_of_sign[32];
	memset(hash_of_sign, 0, sizeof(hash_of_sign));

	ret = sunxi_rsa_sign_check(p_toc1_cert->public_key_size, rsa_params->e,
				   rsa_params->e_len, p_toc1_cert->sign,
				   p_toc1_cert->sign_size, hash_of_sign,
				   sizeof(hash_of_sign), rsa_params->n);
	if (ret) {
		printf("rsa sign calc: calc rsa %d with hardware err\n", p_toc1_cert->public_key_size);
		return -1;
	}
	if (memcmp(hash_of_certif, hash_of_sign, 32)) {
		printf("rsa %d certif verify failed\n", p_toc1_cert->public_key_size);
		printf(">>>>>>>>>>>>>>hash_of_certif\n");
		sunxi_dump(hash_of_certif, 32);
		printf("<<<<<<<<<<<<<<\n");
		printf(">>>>>>>>>>>>>>hash_of_sign\n");
		sunxi_dump(hash_of_sign, 32);
		printf("<<<<<<<<<<<<<<\n");
		return -1;
	}
	return 0;
}

static int aw_certif_ecc_verify_sign(aw_cert_t *p_toc1_cert, u8 *hash_of_certif,
				     u8 hash_of_certif_len)
{
	struct ecc_verify_params *params = (struct ecc_verify_params *)malloc(
		sizeof(struct ecc_verify_params));
	int ret = -1;
	parse_aw_cert_publickey(p_toc1_cert, params);
	parse_aw_cert_sign((aw_cert_ecc_sign_t *)(p_toc1_cert->sign), params);

	params->sign_type = ALG_ECC;

	// pr_err("dump hash_of_certif(%d)\n", hash_of_certif_len);
	// sunxi_dump(hash_of_certif, hash_of_certif_len);
	if (!sunxi_ecc_sign_check(params, 68,
				 hash_of_certif, hash_of_certif_len)) {
		ret = 0;
	} else {
		pr_err("ecc sign check failed\n");
		ret = -1;
	}
	if (params)
		free(params);
	return ret;
}

int aw_certif_verify_itself(sunxi_certif_info_t *sunxi_certif, u8 *buf, u32 len)
{
	int ret = -1;
	u32 toc1_cert_align_len;
	aw_cert_t *p_toc1_cert = (aw_cert_t *)buf;
	toc1_cert_align_len = ALIGN((p_toc1_cert->head_size - RSA_MAX_BYTE), CACHE_LINE_SIZE);
	ALLOC_CACHE_ALIGN_BUFFER(u8, hash_of_certif, CACHE_LINE_SIZE);
	//get cert
	if (aw_certif_probe_sunxi_certif(sunxi_certif,
					 (aw_cert_t *)buf) < 0) {
		return -1;
	}
	if (aw_certif_probe_extern(sunxi_certif, p_toc1_cert) < 0) {
		return -1;
	}

	//get  sign data hash
	memset(hash_of_certif, 0, CACHE_LINE_SIZE);
	if ((unsigned long)p_toc1_cert & (CACHE_LINE_SIZE - 1)) {
		ALLOC_CACHE_ALIGN_BUFFER(u8, p_toc1_cert_align, toc1_cert_align_len);
		memset(p_toc1_cert_align, 0, toc1_cert_align_len);
		memcpy(p_toc1_cert_align, p_toc1_cert, (p_toc1_cert->head_size - RSA_MAX_BYTE));
		ret = sunxi_sha_calc(hash_of_certif, 32, p_toc1_cert_align,
			     (p_toc1_cert->head_size - RSA_MAX_BYTE));
		if (ret) {
			printf("sunxi_sha_calc: calc sha256 with hardware err\n");
			return -1;
		}
	} else {
		ret = sunxi_sha_calc(hash_of_certif, 32, (void *)p_toc1_cert,
				     (p_toc1_cert->head_size - RSA_MAX_BYTE));
		if (ret) {
			printf("sunxi_sha_calc: calc sha256 with hardware err\n");
			return -1;
		}
	}
	ret = toc_certif_desc.aw_certif_verify_sign(p_toc1_cert, hash_of_certif, 32);

	return ret;
}
int sunxi_certif_verify_itself(sunxi_certif_info_t *sunxi_certif, u8 *buf,
			       u32 len)
{
	if (sunxi_certif_desc_register(buf) < 0) {
		return -1;
	}
	return toc_certif_desc.verify_itself(sunxi_certif, buf, len);
}


int aw_certif_rsa_pubkey_hash_cal(u8 *out_buf, u32 hash_size,
				  u8 *public_key_info, u32 public_key_size)
{
	int ret = -1;
	aw_cert_rsa_pk_info_t *rsa_pk_info =
		(aw_cert_rsa_pk_info_t *)public_key_info;
	u32 pkey_size = public_key_size / 8 * 2;
	ALLOC_CACHE_ALIGN_BUFFER(u8, pkey, ALIGN(pkey_size + 31, CACHE_LINE_SIZE));
	if (!pkey) {
		goto rsapk_err;
	}
	memset(pkey, 0x91, ALIGN(pkey_size + 31, CACHE_LINE_SIZE));
	memcpy(pkey, rsa_pk_info->n, rsa_pk_info->n_len);
	memcpy(pkey + rsa_pk_info->n_len, rsa_pk_info->e, rsa_pk_info->e_len);
	if (sunxi_sha_calc(out_buf, hash_size, pkey, pkey_size)) {
		goto rsapk_err;
	}
	ret = 0;
	// printf("dump public hash(%d)\n", pkey_size);
	// sunxi_dump(align, pkey_size);
rsapk_err:
	return ret;
}

int aw_certif_rsa_pubkey_hash_cal_from_sunxi_cert(
	u8 *out_buf, u32 hash_size, sunxi_certif_info_t *sunxi_certif)
{
	return aw_certif_rsa_pubkey_hash_cal(
		out_buf, hash_size,
		(u8 *)(sunxi_certif->aw_cert_info.public_key_info),
		sunxi_certif->aw_cert_info.public_key_size);
}

int aw_certif_ecc_pubkey_hash_cal(u8 *out_buf, u32 hash_size,
				  u8 *public_key_info, u32 public_key_size)
{
	ALLOC_CACHE_ALIGN_BUFFER(u8, pkey, ALIGN(ECC_MAX_BYTE * 2 + 32, CACHE_LINE_SIZE)); //px + py

	aw_cert_ecc_pk_info_t *ecc_pk_info =
		(aw_cert_ecc_pk_info_t *)public_key_info;

	memset(pkey, 0x0, ALIGN(ECC_MAX_BYTE * 2 + 32, CACHE_LINE_SIZE));
	memcpy(pkey, ecc_pk_info->qx, ecc_pk_info->qx_len);
	memcpy(pkey + ecc_pk_info->qx_len, ecc_pk_info->qy,
	       ecc_pk_info->qy_len);

	if (sunxi_sha_calc(out_buf, hash_size, pkey,
			   ecc_pk_info->qx_len + ecc_pk_info->qy_len)) {
		return -1;
	}
	return 0;
}

int aw_certif_ecc_pubkey_hash_cal_from_sunxi_cert(
	u8 *out_buf, u32 hash_size, sunxi_certif_info_t *sunxi_certif)
{
	return aw_certif_ecc_pubkey_hash_cal(
		out_buf, hash_size,
		(u8 *)(sunxi_certif->aw_cert_info.public_key_info),
		sunxi_certif->aw_cert_info.public_key_size);
}

int sunxi_rsa_pubkey_hash_cal(u8 *out_buf, u32 hash_size,
			      sunxi_certif_info_t *sunxi_certif);

int sunxi_certif_pubkey_hash_cal(sunxi_certif_info_t *sunxi_certif,
				 u8 *hash_buf)
{
	if (toc_certif_desc.pubkey_hash_cal_from_sunxi_cert == NULL) {
		//printf("sunxi_certif_desc_register isn't register, use sunxi certif default\n");
		toc_certif_desc.pubkey_hash_cal_from_sunxi_cert = sunxi_rsa_pubkey_hash_cal;
	}
	return toc_certif_desc.pubkey_hash_cal_from_sunxi_cert(hash_buf, 32, sunxi_certif);
}

int aw_certif_verify_subcert_pubkey(sunxi_certif_info_t *root_certif,
				    u8 ext_seq, sunxi_certif_info_t *sub_certif)
{
	u8 sub_pubkey_hash[32] = { 0 };

	if (toc_certif_desc.pubkey_hash_cal_from_sunxi_cert(sub_pubkey_hash, 32,
							    sub_certif) < 0) {
		return -1;
	}
	if (memcmp(root_certif->extension.value[ext_seq], sub_pubkey_hash,
		   32)) {
		printf("root cert exten hash(%d)\n", 32);
		sunxi_dump((void *)root_certif->extension.value[ext_seq], 32);
		printf("sub cert pubkey hash(%d)\n", 32);
		sunxi_dump((void *)sub_pubkey_hash, 32);
		return -1;
	}
	return 0;
}

int sunxi_certif_verify_subcert_pubkey(sunxi_certif_info_t *root_certif,
				       sunxi_certif_info_t *sub_certif,
				       const char *name)
{
	u8 i;

	for (i = 0; i < root_certif->extension.extension_num; i++) {
		if (!strncmp((const char *)root_certif->extension.name[i],
			     name,
			     root_certif->extension.name_len[i])) {
			printf("find %s key stored in root certif\n", name);

			if (toc_certif_desc.verify_subcert_pubkey(
				    root_certif, i, sub_certif) < 0) {
				return -1;
			}
			break;
		}
	}
	if (i == root_certif->extension.extension_num) {
		printf("cant find %s key stored in root certif\n", name);
		return -1;
	}
	return 0;
}

int sunxi_certif_desc_register(u8 *buf)
{
	int ret = 0;
	aw_cert_t *p_toc1_cert = (aw_cert_t *)buf;

	if (!memcmp(buf, AW_CERT_FORMAT_SIGN, AW_CERT_MAGIC_SIZE)) {
		// printf("aw ceritf %s %d\n",
		//        p_toc1_cert->algorithm_type ? "ecc" : "rsa",
		//        p_toc1_cert->public_key_size);
		toc_certif_desc.verify_itself = aw_certif_verify_itself;
		toc_certif_desc.verify_subcert_pubkey = aw_certif_verify_subcert_pubkey;

		if (p_toc1_cert->algorithm_type == SIGN_TYPE_ECC) {
			toc_certif_desc.pubkey_hash_cal_from_sunxi_cert =
				aw_certif_ecc_pubkey_hash_cal_from_sunxi_cert;
			toc_certif_desc.aw_certif_verify_sign =
				aw_certif_ecc_verify_sign;
			// toc_certif_desc.aw_certif_pubkey_hash_cal =
			// 	aw_certif_ecc_pubkey_hash_cal;
		} else if (p_toc1_cert->algorithm_type == SIGN_TYPE_RSA) {
			toc_certif_desc.pubkey_hash_cal_from_sunxi_cert =
				aw_certif_rsa_pubkey_hash_cal_from_sunxi_cert;
			toc_certif_desc.aw_certif_verify_sign =
				aw_certif_rsa_verify_sign;
			// toc_certif_desc.aw_certif_pubkey_hash_cal =
			// 	aw_certif_rsa_pubkey_hash_cal;
		} else {
			printf("unsupport asymmetric\n");
			return -1;
		}
	} else {
		toc_certif_desc.pubkey_hash_cal_from_sunxi_cert =
			sunxi_rsa_pubkey_hash_cal;
		toc_certif_desc.verify_itself = sunxi_X509_certif_verify_itself;
		//toc_certif_desc.verify_subcert_pubkey =
		//	sunxi_X509_certif_verify_subcert_pubkey;
	}
	return ret;
}
