// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2014
 * DENX Software Engineering
 * Heiko Schocher <hs@denx.de>
 *
 * Based on:
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 *
 * Updated-by: Prafulla Wadaskar <prafulla@marvell.com>
 *		FIT image specific code abstracted from mkimage.c
 *		some functions added to address abstraction
 *
 * All rights reserved.
 */

#include "mkimage.h"
#include "fit_common.h"
#include <bootm.h>
#include <image.h>
#include <aes_ext.h>
#include <u-boot/crc.h>

void usage(char *cmdname)
{
	fprintf(stderr, "Usage: %s -f fit file -k key file -c config name [-a aes key file]\n"
			 "          -f ==> set fit file which should be checked'\n"
			 "          -k ==> set key .dtb file which contains the key'\n"
			 "          -c ==> set the configuration name'\n"
			 "          -a ==> set raw 16-byte AES-128 key file'\n",
		cmdname);
	exit(EXIT_FAILURE);
}

static int read_aes_key(const char *filename, uint8_t *key)
{
	uint8_t data[AES_KEYLEN + 1];
	FILE *fp;
	size_t size;

	fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "Can't open AES key file '%s': %s\n",
			filename, strerror(errno));
		return -1;
	}

	size = fread(data, 1, sizeof(data), fp);
	if (ferror(fp) || size != AES_KEYLEN) {
		fprintf(stderr, "AES key file '%s' must contain exactly %d bytes\n",
			filename, AES_KEYLEN);
		fclose(fp);
		return -1;
	}

	fclose(fp);
	memcpy(key, data, AES_KEYLEN);
	return 0;
}

int main(int argc, char **argv)
{
	int ffd = -1;
	int kfd = -1;
	struct stat fsbuf;
	struct stat ksbuf;
	void *fit_blob;
	char *aeskeyfile = NULL;
	char *fdtfile = NULL;
	char *keyfile = NULL;
	char *config_name = NULL;
	char cmdname[256];
	struct bootm_host_params params = { 0 };
	uint8_t aes_key[AES_KEYLEN];
	int ret;
	void *key_blob;
	int c;

	strncpy(cmdname, *argv, sizeof(cmdname) - 1);
	cmdname[sizeof(cmdname) - 1] = '\0';
	while ((c = getopt(argc, argv, "a:f:k:c:s")) != -1)
		switch (c) {
		case 'a':
			aeskeyfile = optarg;
			break;
		case 'f':
			fdtfile = optarg;
			break;
		case 'k':
			keyfile = optarg;
			break;
		case 'c':
			config_name = optarg;
			break;
		case 's':
			params.is_spl = 1;
			break;
		default:
			usage(cmdname);
			break;
	}

	if (!fdtfile) {
		fprintf(stderr, "%s: Missing fdt file\n", *argv);
		usage(*argv);
	}
	if (!keyfile) {
		fprintf(stderr, "%s: Missing key file\n", *argv);
		usage(*argv);
	}
	if (aeskeyfile) {
		if (read_aes_key(aeskeyfile, aes_key))
			return EXIT_FAILURE;
		params.aes_key = aes_key;
	}

	ffd = mmap_fdt(cmdname, fdtfile, 0, &fit_blob, &fsbuf, false, true);
	if (ffd < 0)
		return EXIT_FAILURE;
	kfd = mmap_fdt(cmdname, keyfile, 0, &key_blob, &ksbuf, false, true);
	if (kfd < 0)
		return EXIT_FAILURE;

	image_set_host_blob(key_blob);
	ret = fit_check_sign(fit_blob, key_blob, config_name, &params);
	if (!ret) {
		ret = EXIT_SUCCESS;
		fprintf(stderr, "Signature check OK\n");
	} else {
		ret = EXIT_FAILURE;
		fprintf(stderr, "Signature check bad (error %d)\n", ret);
	}

	(void) munmap((void *)fit_blob, fsbuf.st_size);
	(void) munmap((void *)key_blob, ksbuf.st_size);

	close(ffd);
	close(kfd);
	exit(ret);
}
