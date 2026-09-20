#!/bin/bash
#
# Copyright (c) 2022 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier: GPL-2.0
#
set -e

FIT_DIR="fit"
IMG_UBOOT="uboot.img"
IMG_BOOT="boot.img"
IMG_RECOVERY="recovery.img"
ITB_UBOOT="${FIT_DIR}/uboot.itb"
ITB_BOOT="${FIT_DIR}/boot.itb"
ITB_RECOVERY="${FIT_DIR}/recovery.itb"
SIG_BIN="data2sign.bin"
SIG_UBOOT="${FIT_DIR}/uboot.data2sign"
SIG_BOOT="${FIT_DIR}/boot.data2sign"
SIG_RECOVERY="${FIT_DIR}/recovery.data2sign"
SIG_CFG_DIR="${FIT_DIR}/fit_signcfg"
SIG_CONFIG="${SIG_CFG_DIR}/sign.readonly_config"
MINIALL_INI="${SIG_CFG_DIR}/MINIALL.ini"
DTC_TOOL="${SIG_CFG_DIR}/dtc"
# offs
OFFS_DATA="0x1400"
# placeholder address
FDT_ADDR_PLACEHOLDER="0xffffff00"
KERNEL_ADDR_PLACEHOLDER="0xffffff01"
RAMDISK_ADDR_PLACEHOLDER="0xffffff02"
# tools
MKIMAGE="./tools/mkimage"
RK_SIGN_TOOL="../rkbin/tools/rk_sign_tool"
BOOT_MERGER="../rkbin/tools/boot_merger"
FIT_UNPACK="./scripts/fit-unpack.sh"
CHECK_SIGN="./tools/fit_check_sign"
# key
KEY_DIR="keys/"
AES_KEY="keys/aes128.key"
RSA_PRI_KEY="keys/dev.key"
RSA_PUB_KEY="keys/dev.pubkey"
RSA_CRT_KEY="keys/dev.crt"
LEGACY_RSA_PRI_KEY="legacy_keys/dev.key"
LEGACY_RSA_PUB_KEY="legacy_keys/dev.pubkey"
LEGACY_RSA_CRT_KEY="legacy_keys/dev.crt"
SIGNATURE_KEY_NODE="/signature/key-dev"
SPL_DTB="spl/u-boot-spl.dtb"
UBOOT_DTB="u-boot.dtb"
# its
ITS_UBOOT="u-boot.its"
ITS_BOOT="boot.its"
ITS_RECOVERY="recovery.its"
ARG_VER_UBOOT="0"
ARG_VER_BOOT="0"
ARG_VER_RECOVERY="0"

function help()
{
	echo
	echo "usage:"
	echo "    $0 [args]"
	echo
	echo "args:"
	echo "    --rollback-index-recovery  <decimal integer>"
	echo "    --rollback-index-boot      <decimal integer>"
	echo "    --rollback-index-uboot     <decimal integer>"
	echo "    --rollback-index-loader    <decimal integer>"
	echo "    --version-recovery         <decimal integer>"
	echo "    --version-boot             <decimal integer>"
	echo "    --version-uboot            <decimal integer>"
	echo "    --boot_img                 <boot image>"
	echo "    --recovery_img             <recovery image>"
	echo "    --args                     <arg>"
	echo "    --ini-loader               <loader ini file>"
	echo "    --ini-trust                <trust ini file>"
	echo "    --no-check"
	echo "    --no-sign"
	echo "    --spl-new"
	echo
}

function arg_check_decimal()
{
	if [ -z $1 ]; then
		help
		exit 1
	fi

	decimal=`echo $1 |sed 's/[0-9]//g'`
	if [ ! -z ${decimal} ]; then
		echo "ERROR: $1 is not decimal integer"
		help
		exit 1
	fi
}

function check_its()
{
	cat $1 | while read line
	do
		file=`echo ${line} | sed -n "/incbin/p" | awk -F '"' '{ printf $2 }' | tr -d ' '`
		if [ ! -f ${file} ]; then
			echo "ERROR: No ${file}"
			exit 1
		fi
	done
}

function check_rsa_algo()
{
	if grep -q '^CONFIG_FIT_ENABLE_RSA4096_SUPPORT=y' .config ; then
		rsa_algo="rsa4096"
	else
		rsa_algo="rsa2048"
	fi
	if ! grep -qr ${rsa_algo} $1 ; then
		echo "ERROR: Wrong rsa_algo in its file. It should be ${rsa_algo}."
		exit 1
	fi
}

function check_rsa_keys()
{
	if [ ! -f ${RSA_PRI_KEY} ]; then
		echo "ERROR: No ${RSA_PRI_KEY} "
		exit 1
	elif [ ! -f ${RSA_PUB_KEY} ]; then
		echo "ERROR: No ${RSA_PUB_KEY} "
		exit 1
	elif [ ! -f ${RSA_CRT_KEY} ]; then
		echo "ERROR: No ${RSA_CRT_KEY} "
		exit 1
	fi
}

function check_aes_key()
{
	if [ ! -f "${AES_KEY}" ] || [ "$(wc -c < "${AES_KEY}")" -ne 16 ]; then
		echo "ERROR: ${AES_KEY} must be a 16-byte AES-128 key"
		exit 1
	fi
}

function unpack_fit()
{
	if [ "${ARG_NO_CIPHER}" != "y" ] && grep -q '^CONFIG_FIT_CIPHER=y' .config ; then
		check_aes_key
		${FIT_UNPACK} -f $1 -o $2 -k ${AES_KEY} -encrypt
	else
		${FIT_UNPACK} -f $1 -o $2
	fi
}

function insert_aes_key()
{
	local DTB=$1

	check_aes_key
	fdtput -p -t bx ${DTB} /aes_key data $(xxd -p -c 1 ${AES_KEY})
}

function insert_spl_aes_key()
{
	if grep -q '^CONFIG_SPL_FIT_CIPHER_SW=y' .config ; then
		insert_aes_key ${SPL_DTB}
	else
		check_aes_key
	fi
}

function insert_uboot_aes_key()
{
	insert_aes_key ${UBOOT_DTB}
}

function rebuild_uboot_fdt()
{
	FDT_DATA=${UBOOT_DTB}

	if grep -q '^CONFIG_SPL_FIT_CIPHER=y' .config ; then
		insert_spl_aes_key
		openssl rand -out ${FDT_DATA}.iv 16
		openssl dgst -sha256 -binary -out ${FDT_DATA}.digest ${FDT_DATA}
		openssl enc -aes-128-ctr -in ${FDT_DATA} -out ${FDT_DATA}.aes128 \
			-K "$(xxd -p -c 256 ${AES_KEY})" -iv "$(xxd -p -c 256 ${FDT_DATA}.iv)"
	fi
}

function check_uboot_fdt_uncompressed()
{
	COMPRESSION=`fdtget -ts ${ITB_UBOOT} /images/fdt compression 2>/dev/null`
	if [ "${COMPRESSION}" != "none" ]; then
		echo "ERROR: U-Boot FDT compression must be none"
		exit 1
	fi
}

function validate_arg()
{
	case $1 in
		--no-check|--no-sign|--spl-new|--burn-key-hash)
			shift=1
			;;
		--ini-trust|--ini-loader|--rollback-index-boot|--rollback-index-recovery|--rollback-index-uboot|--rollback-index-loader|--boot_img|--recovery_img|--version-uboot|--version-boot|--version-recovery|--chip)
			shift=2
			;;
		*)
			shift=0
			;;
	esac
	echo ${shift}
}

function fit_process_args()
{
	if [ $# -eq 0 ]; then
		help
		exit 0
	fi

	if grep -q '^CONFIG_FIT_SIGNATURE=y' .config ; then
		ARG_SIGN="y"
	fi

	while [ $# -gt 0 ]; do
		case $1 in
			--args)
				ARG_VALIDATE=$2
				shift 2
				;;
			--boot_img)     # boot.img
				ARG_BOOT_IMG=$2
				shift 2
				;;
			--chip)
				ARG_CHIP=$2
				shift 2
				;;
			--recovery_img) # recovery.img
				ARG_RECOVERY_IMG=$2
				shift 2
				;;
			--boot_img_dir) # boot.img components directory
				ARG_BOOT_IMG_DIR=$2
				shift 2
				;;
			--no-check)     # No hostcc fit signature check
				ARG_NO_CHECK="y"
				shift 1
				;;
			--no-sign)
				ARG_NO_SIGN="y"
				ARG_NO_CIPHER="y"
				ARG_SIGN="n"
				shift 1
				;;
			--ini-trust)    # Assign trust ini file
				ARG_INI_TRUST=$2
				shift 2
				;;
			--ini-loader)   # Assign loader ini file
				ARG_INI_LOADER=$2
				shift 2
				;;
			--spl-new)      # Use current build u-boot-spl.bin to pack loader
				ARG_SPL_NEW="y"
				# Whether aarch32 or not, spl only support 64 bits version.
				if grep -q '^CONFIG_ARM64_BOOT_AARCH32=y' .config ; then
					echo "ERROR: SPL doesn't support 32-bit. Please build 64-bit defconfig and update u-boot-spl.bin to rkbin first."
					exit 1
				fi
				shift 1
				;;
			--rollback-index-boot)
				ARG_ROLLBACK_IDX_BOOT=$2
				arg_check_decimal $2
				shift 2
				;;
			--rollback-index-recovery)
				ARG_ROLLBACK_IDX_RECOVERY=$2
				arg_check_decimal $2
				shift 2
				;;
			--rollback-index-uboot)
				ARG_ROLLBACK_IDX_UBOOT=$2
				arg_check_decimal $2
				shift 2
				;;
			--rollback-index-loader)
				ARG_ROLLBACK_IDX_LOADER=$2
				arg_check_decimal $2
				if [ "$2" -gt 65535 ]; then
					echo "Error: --rollback-index-loader ($2) exceeds 16-bit maximum (65535)" >&2
					exit 1
				fi
				shift 2
				;;
			--version-uboot)
				ARG_VER_UBOOT=$2
				arg_check_decimal $2
				shift 2
				;;
			--version-boot)
				ARG_VER_BOOT=$2
				arg_check_decimal $2
				shift 2
				;;
			--version-recovery)
				ARG_VER_RECOVERY=$2
				arg_check_decimal $2
				shift 2
				;;
			--burn-key-hash)
				ARG_BURN_KEY_HASH="y"
				shift 1
				;;
			--spl-fwver)
				ARG_FIT_FWVER="${ARG_FIT_FWVER} --spl-fwver $2"
				shift 2
				;;
			--fwver)
				ARG_FIT_FWVER="${ARG_FIT_FWVER} --fwver $2"
				shift 2
				;;
			*)
				help
				exit 1
				;;
		esac
	done
}

function fit_raw_compile()
{
	# Verified-boot: should rebuild code but don't need to repack images.
	if [ "${ARG_SIGN}" == "y" ]; then
		./make.sh --raw-compile ${ARG_FIT_FWVER}
	fi
	rm ${FIT_DIR} -rf && mkdir -p ${FIT_DIR} && mkdir -p ${SIG_CFG_DIR}
}

function fit_gen_uboot_itb()
{
	if [ "${ARG_NO_CIPHER}" != "y" ] && grep -q '^CONFIG_FIT_CIPHER_SW=y' .config ; then
		insert_uboot_aes_key
	fi

	# generate u-boot.its file
	FIT_NO_CIPHER=${ARG_NO_CIPHER} ./make.sh itb ${ARG_INI_TRUST}

	# check existance of file in its
	check_its ${ITS_UBOOT}

	if [ "${ARG_SIGN}" != "y" ]; then
		if [ "${ARG_NO_CIPHER}" != "y" ] && grep -q '^CONFIG_SPL_FIT_CIPHER=y' .config ; then
			insert_spl_aes_key
		fi
		${MKIMAGE} -f ${ITS_UBOOT} -E -p ${OFFS_DATA} ${ITB_UBOOT} -v ${ARG_VER_UBOOT}
		check_uboot_fdt_uncompressed
		if [ "${ARG_SPL_NEW}" == "y" ]; then
			./make.sh --spl ${ARG_INI_LOADER}
			echo "pack loader with new: spl/u-boot-spl.bin"
		else
			./make.sh loader ${ARG_INI_LOADER}
		fi
	else
		check_rsa_keys

		if ! grep -q '^CONFIG_SPL_FIT_SIGNATURE=y' .config ; then
			echo "ERROR: CONFIG_SPL_FIT_SIGNATURE is disabled"
			exit 1
		fi

		# rollback-index
		if grep -q '^CONFIG_SPL_FIT_ROLLBACK_PROTECT=y' .config ; then
			ARG_SPL_ROLLBACK_PROTECT="y"
			if [ -z ${ARG_ROLLBACK_IDX_UBOOT} ]; then
				echo "ERROR: No arg \"--rollback-index-uboot <n>\""
				exit 1
			fi
		fi

		if [ "${ARG_SPL_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`grep 'rollback-index' ${ITS_UBOOT} | awk -F '=' '{ printf $2 }' | tr -d ' '`
			sed -i "s/rollback-index = ${VERSION}/rollback-index = <${ARG_ROLLBACK_IDX_UBOOT}>;/g" ${ITS_UBOOT}
		fi

		# Generally, boot.img is signed before uboot.img, so the ras key can be found
		# in u-boot.dtb. If not found, let's insert rsa key anyway.
		if ! fdtget -l ${UBOOT_DTB} /signature >/dev/null 2>&1 ; then
			${MKIMAGE} -f ${ITS_UBOOT} -k ${KEY_DIR} -K ${UBOOT_DTB} -E -p ${OFFS_DATA} -r ${ITB_UBOOT} -v ${ARG_VER_UBOOT}
			echo "## Adding RSA public key into ${UBOOT_DTB}"
		fi
		rebuild_uboot_fdt

		# Pack
		${MKIMAGE} -f ${ITS_UBOOT} -k ${KEY_DIR} -K ${SPL_DTB} -E -p ${OFFS_DATA} -r ${ITB_UBOOT} -v ${ARG_VER_UBOOT}
		check_uboot_fdt_uncompressed
		mv ${SIG_BIN} ${SIG_UBOOT}

		# burn-key-hash
		if [ "${ARG_BURN_KEY_HASH}" == "y" ]; then
			if grep -q '^CONFIG_SPL_FIT_HW_CRYPTO=y' .config ; then
				fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} burn-key-hash 0x1
			else
				echo "ERROR: --burn-key-hash requires CONFIG_SPL_FIT_HW_CRYPTO=y"
				exit 1
			fi
		fi

		# rollback-index read back check
		if [ "${ARG_SPL_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`fdtget -ti ${ITB_UBOOT} /configurations/conf rollback-index`
			if [ "${VERSION}" != "${ARG_ROLLBACK_IDX_UBOOT}" ]; then
				echo "ERROR: Failed to set rollback-index for ${ITB_UBOOT}";
				exit 1
			fi
		fi

		# burn-key-hash read back check
		if [ "${ARG_BURN_KEY_HASH}" == "y" ]; then
			if [ "`fdtget -ti ${SPL_DTB} ${SIGNATURE_KEY_NODE} burn-key-hash`" != "1" ]; then
				echo "ERROR: Failed to set burn-key-hash for ${SPL_DTB}";
				exit 1
			fi
		fi

		# host check signature
		if [ "${ARG_NO_CHECK}" != "y" ]; then
			CHECK_SIGN_AES=
			if grep -q '^CONFIG_SPL_FIT_CIPHER=y' .config ; then
				CHECK_SIGN_AES="-a ${AES_KEY}"
			fi
			if [ "${ARG_SPL_NEW}" == "y" ]; then
				 ${CHECK_SIGN} -f ${ITB_UBOOT} -k ${SPL_DTB} ${CHECK_SIGN_AES} -s
			else
				spl_file="../rkbin/"`sed -n "/FlashBoot=/s/FlashBoot=//p" ${ARG_INI_LOADER}  |tr -d '\r'`
				offs=`fdtdump -s ${spl_file} | head -1 | awk -F ":" '{ print $2 }' | sed "s/ found fdt at offset //g" | tr -d " "`
				if [ -z ${offs}  ]; then
					echo "ERROR: invalid ${spl_file} , unable to find fdt blob"
				fi
				offs=`printf %d ${offs} ` # hex -> dec
				dd if=${spl_file} of=spl/u-boot-spl-old.dtb bs=${offs} skip=1 >/dev/null 2>&1
				${CHECK_SIGN} -f ${ITB_UBOOT} -k spl/u-boot-spl-old.dtb ${CHECK_SIGN_AES} -s
			fi
		fi

		# minimize u-boot-spl.dtb: clear as 0 but not remove property.
		if grep -q '^CONFIG_SPL_FIT_HW_CRYPTO=y' .config ; then
			fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,r-squared 0x0
			if grep -q '^CONFIG_SPL_ROCKCHIP_CRYPTO_V1=y' .config ; then
				fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
				fdtput -r ${SPL_DTB} ${SIGNATURE_KEY_NODE}/hash@np
			else
				fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
				fdtput -r ${SPL_DTB} ${SIGNATURE_KEY_NODE}/hash@c
			fi
		else
			fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
			fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
			fdtput -tx ${SPL_DTB} ${SIGNATURE_KEY_NODE} rsa,exponent-BN 0x0
			fdtput -r ${SPL_DTB} ${SIGNATURE_KEY_NODE}/hash@c
			fdtput -r ${SPL_DTB} ${SIGNATURE_KEY_NODE}/hash@np
		fi

		# repack spl
		if [ "${ARG_SPL_NEW}" == "y" ]; then
			cat spl/u-boot-spl-nodtb.bin > spl/u-boot-spl.bin
			if ! grep -q '^CONFIG_SPL_SEPARATE_BSS=y' .config ; then
				cat spl/u-boot-spl-pad.bin >> spl/u-boot-spl.bin
			fi
			cat ${SPL_DTB} >> spl/u-boot-spl.bin

			./make.sh --spl ${ARG_INI_LOADER}
			echo "## pack loader with new: spl/u-boot-spl.bin"
		else
			./make.sh loader ${ARG_INI_LOADER}
		fi

		if [ "${ARG_BURN_KEY_HASH}" == "y" ]; then
			echo "## ${SPL_DTB}: burn-key-hash=1"
		fi
	fi

	rm -f u-boot.itb u-boot.img u-boot-dtb.img
	mv ${ITS_UBOOT} ${FIT_DIR}
}

function fit_fixup_address()
{
	local its=$1
	local placeholder=$2
	local addr=$3
	local cells="<${addr}>"

	# Use two cells when any of the upper 32 address bits are set.
	if (( addr >> 32 )); then
		cells="/bits/ 64 <${addr}>"
	fi

	sed -i "s|<[[:space:]]*${placeholder}[[:space:]]*>|${cells}|g" "${its}"
}

function fit_gen_boot_itb()
{
	if [ ! -z ${ARG_BOOT_IMG} ]; then
		unpack_fit ${ARG_BOOT_IMG} ${FIT_DIR}/unpack
		ITS_BOOT="${FIT_DIR}/unpack/image.its"
	else
		compression=`awk -F"," '/COMPRESSION=/  { printf $1 }' ${ARG_INI_TRUST} | tr -d ' ' | cut -c 13-`
		if [ -z "${compression}" ]; then
			compression="none"
		fi
		./arch/arm/mach-rockchip/make_fit_boot.sh -c ${compression} > ${ITS_BOOT}
		check_its ${ITS_BOOT}
		if [ "${ARG_NO_CIPHER}" != "y" ] && grep -q '^CONFIG_FIT_CIPHER=y' .config ; then
			${MKIMAGE} -f ${ITS_BOOT} -E -p ${OFFS_DATA} ${ITB_BOOT} -v ${ARG_VER_BOOT}
			unpack_fit ${ITB_BOOT} ${FIT_DIR}/unpack
			ITS_BOOT="${FIT_DIR}/unpack/image.its"
		fi
	fi

	if [ "${ARG_SIGN}" != "y" ]; then
		${MKIMAGE} -f ${ITS_BOOT} -E -p ${OFFS_DATA} ${ITB_BOOT} -v ${ARG_VER_BOOT}
	else
		check_rsa_keys

		check_rsa_algo ${ITS_BOOT}

		if [ "${ARG_SIGN}" != "y" ]; then
			echo "ERROR: CONFIG_FIT_SIGNATURE is disabled"
			exit 1
		fi

		if grep -q '^CONFIG_FIT_ROLLBACK_PROTECT=y' .config ; then
			ARG_ROLLBACK_PROTECT="y"
			if [ -z ${ARG_ROLLBACK_IDX_BOOT} ]; then
				echo "ERROR: No arg \"--rollback-index-boot <n>\""
				exit 1
			fi
			if ! grep -q '^CONFIG_OPTEE=y' .config ; then
				echo "ERROR: Don't support \"--rollback-index-boot <n>\""
				exit 1
			fi
		fi

		# fixup
		FDT_ADDR_R=`strings env/common.o | grep 'fdt_addr_r=' | awk -F "=" '{ print $2 }'`
		KERNEL_ADDR_R=`strings env/common.o | grep 'kernel_addr_r=' | awk -F "=" '{ print $2 }'`
		RMADISK_ADDR_R=`strings env/common.o | grep 'ramdisk_addr_r=' | awk -F "=" '{ print $2 }'`
		fit_fixup_address ${ITS_BOOT} ${FDT_ADDR_PLACEHOLDER} ${FDT_ADDR_R}
		fit_fixup_address ${ITS_BOOT} ${KERNEL_ADDR_PLACEHOLDER} ${KERNEL_ADDR_R}
		fit_fixup_address ${ITS_BOOT} ${RAMDISK_ADDR_PLACEHOLDER} ${RMADISK_ADDR_R}

		if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`grep 'rollback-index' ${ITS_BOOT} | awk -F '=' '{ printf $2 }' | tr -d ' '`
			sed -i "s/rollback-index = ${VERSION}/rollback-index = <${ARG_ROLLBACK_IDX_BOOT}>;/g" ${ITS_BOOT}
		fi

		${MKIMAGE} -f ${ITS_BOOT} -k ${KEY_DIR} -K ${UBOOT_DTB} -E -p ${OFFS_DATA} -r ${ITB_BOOT} -v ${ARG_VER_BOOT}
		mv ${SIG_BIN} ${SIG_BOOT}

		# rollback-index read back check
		if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`fdtget -ti ${ITB_BOOT} /configurations/conf rollback-index`
			if [ "${VERSION}" != "${ARG_ROLLBACK_IDX_BOOT}" ]; then
				echo "ERROR: Failed to set rollback-index for ${ITB_BOOT}";
				exit 1
			fi
		fi

		# host check signature
		if [ "${ARG_NO_CHECK}" != "y" ]; then
			CHECK_SIGN_AES=
			if grep -q '^CONFIG_FIT_CIPHER=y' .config ; then
				CHECK_SIGN_AES="-a ${AES_KEY}"
			fi
			${CHECK_SIGN} -f ${ITB_BOOT} -k ${UBOOT_DTB} ${CHECK_SIGN_AES}
		fi

		# minimize u-boot.dtb: clearn as 0 but not remove property.
		if grep -q '^CONFIG_FIT_HW_CRYPTO=y' .config ; then
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,r-squared 0x0
			if grep -q '^CONFIG_ROCKCHIP_CRYPTO_V1=y' .config ; then
				fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
			else
				fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
			fi
		else
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,exponent-BN 0x0
		fi
		fdtput -r ${UBOOT_DTB} ${SIGNATURE_KEY_NODE}/hash@c
		fdtput -r ${UBOOT_DTB} ${SIGNATURE_KEY_NODE}/hash@np
	fi

	mv ${ITS_BOOT} ${FIT_DIR}
}

function fit_gen_recovery_itb()
{
	if [ ! -z ${ARG_RECOVERY_IMG} ]; then
		unpack_fit ${ARG_RECOVERY_IMG} ${FIT_DIR}/unpack
		ITS_RECOVERY="${FIT_DIR}/unpack/image.its"
	else
		echo "ERROR: No recovery.img"
		exit 1
	fi

	if [ "${ARG_SIGN}" != "y" ]; then
		${MKIMAGE} -f ${ITS_RECOVERY} -E -p ${OFFS_DATA} ${ITB_RECOVERY} -v ${ARG_VER_RECOVERY}
	else
		check_rsa_keys

		check_rsa_algo ${ITS_RECOVERY}

		if [ "${ARG_SIGN}" != "y" ]; then
			echo "ERROR: CONFIG_FIT_SIGNATURE is disabled"
			exit 1
		fi

		if grep -q '^CONFIG_FIT_ROLLBACK_PROTECT=y' .config ; then
			ARG_ROLLBACK_PROTECT="y"
			if [ -z ${ARG_ROLLBACK_IDX_RECOVERY} ]; then
				echo "ERROR: No arg \"--rollback-index-recovery <n>\""
				exit 1
			fi
			if ! grep -q '^CONFIG_OPTEE=y' .config ; then
				echo "ERROR: Don't support \"--rollback-index-recovery <n>\""
				exit 1
			fi
		fi

		# fixup
		FDT_ADDR_R=`strings env/common.o | grep 'fdt_addr_r=' | awk -F "=" '{ print $2 }'`
		KERNEL_ADDR_R=`strings env/common.o | grep 'kernel_addr_r=' | awk -F "=" '{ print $2 }'`
		RMADISK_ADDR_R=`strings env/common.o | grep 'ramdisk_addr_r=' | awk -F "=" '{ print $2 }'`
		fit_fixup_address ${ITS_RECOVERY} ${FDT_ADDR_PLACEHOLDER} ${FDT_ADDR_R}
		fit_fixup_address ${ITS_RECOVERY} ${KERNEL_ADDR_PLACEHOLDER} ${KERNEL_ADDR_R}
		fit_fixup_address ${ITS_RECOVERY} ${RAMDISK_ADDR_PLACEHOLDER} ${RMADISK_ADDR_R}

		if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`grep 'rollback-index' ${ITS_RECOVERY} | awk -F '=' '{ printf $2 }' | tr -d ' '`
			sed -i "s/rollback-index = ${VERSION}/rollback-index = <${ARG_ROLLBACK_IDX_RECOVERY}>;/g" ${ITS_RECOVERY}
		fi

		${MKIMAGE} -f ${ITS_RECOVERY} -k ${KEY_DIR} -K ${UBOOT_DTB} -E -p ${OFFS_DATA} -r ${ITB_RECOVERY} -v ${ARG_VER_RECOVERY}
		mv ${SIG_BIN} ${SIG_RECOVERY}

		# rollback-index read back check
		if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
			VERSION=`fdtget -ti ${ITB_RECOVERY} /configurations/conf rollback-index`
			if [ "${VERSION}" != "${ARG_ROLLBACK_IDX_RECOVERY}" ]; then
				echo "ERROR: Failed to set rollback-index for ${ITB_RECOVERY}";
				exit 1
			fi
		fi

		# host check signature
		if [ "${ARG_NO_CHECK}" != "y" ]; then
			CHECK_SIGN_AES=
			if grep -q '^CONFIG_FIT_CIPHER=y' .config ; then
				CHECK_SIGN_AES="-a ${AES_KEY}"
			fi
			${CHECK_SIGN} -f ${ITB_RECOVERY} -k ${UBOOT_DTB} ${CHECK_SIGN_AES}
		fi

		# minimize u-boot.dtb: clearn as 0 but not remove property.
		if grep -q '^CONFIG_FIT_HW_CRYPTO=y' .config ; then
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,r-squared 0x0
			if grep -q '^CONFIG_ROCKCHIP_CRYPTO_V1=y' .config ; then
				fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
			else
				fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
			fi
		else
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,c 0x0
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,np 0x0
			fdtput -tx ${UBOOT_DTB} ${SIGNATURE_KEY_NODE} rsa,exponent-BN 0x0
		fi
		fdtput -r ${UBOOT_DTB} ${SIGNATURE_KEY_NODE}/hash@c
		fdtput -r ${UBOOT_DTB} ${SIGNATURE_KEY_NODE}/hash@np
	fi

	mv ${ITS_RECOVERY} ${FIT_DIR}
}

function fit_gen_uboot_img()
{
	ITB=$1

	if [ -z ${ITB} ]; then
		ITB=${ITB_UBOOT}
	fi

	ITB_MAX_NUM=`sed -n "/SPL_FIT_IMAGE_MULTIPLE/p" .config | awk -F "=" '{ print $2 }'`
	ITB_MAX_KB=`sed  -n "/SPL_FIT_IMAGE_KB/p" .config | awk -F "=" '{ print $2 }'`
	ITB_MAX_BS=$((ITB_MAX_KB*1024))
	ITB_BS=`ls -l ${ITB} | awk '{ print $5 }'`

	if [ ${ITB_BS} -gt ${ITB_MAX_BS} ]; then
		echo "ERROR: pack ${IMG_UBOOT} failed! ${ITB} actual: ${ITB_BS} bytes, max limit: ${ITB_MAX_BS} bytes"
		exit 1
	fi

	rm -f ${IMG_UBOOT}
	for ((i = 0; i < ${ITB_MAX_NUM}; i++));
	do
		cat ${ITB} >> ${IMG_UBOOT}
		truncate -s %${ITB_MAX_KB}K ${IMG_UBOOT}
	done
}

function fit_gen_boot_img()
{
	ITB=$1

	if [ -z ${ITB} ]; then
		ITB=${ITB_BOOT}
	fi

	if [ "${ITB}" != "${IMG_BOOT}" ]; then
		cp ${ITB} ${IMG_BOOT} -f
	fi
}

function fit_gen_recovery_img()
{
	ITB=$1

	if [ -z ${ITB} ]; then
		ITB=${ITB_RECOVERY}
	fi

	if [ "${ITB}" != "${IMG_RECOVERY}" ]; then
		cp ${ITB} ${IMG_RECOVERY} -f
	fi
}

function fit_gen_loader()
{
	if [ "${ARG_SIGN}" == "y" ]; then
		if grep -q '^CONFIG_SPL_FIT_CIPHER=y' .config ; then
			check_aes_key
			LOADER=
			if ls *loader*.bin >/dev/null 2>&1 ; then
				LOADER=`ls *loader*.bin | head -n 1`
			elif ls *download*.bin >/dev/null 2>&1 ; then
				LOADER=`ls *download*.bin | head -n 1`
			fi
			if [ -z "${LOADER}" ]; then
				echo "ERROR: No loader found"
				exit 1
			fi

			LOADER_CIPHER_DIR=fit/unpack_loader/
			rm  ${LOADER_CIPHER_DIR} -rf && mkdir -p ${LOADER_CIPHER_DIR}
			LOADER_IV=${LOADER_CIPHER_DIR}/nonce.bin
			openssl rand -out ${LOADER_IV} 8
			if [ -n "${LOADER}" ]; then
				${BOOT_MERGER} unpack -i ${LOADER} -o ${LOADER_CIPHER_DIR}
				cp ${ARG_INI_LOADER} ${LOADER_CIPHER_DIR}/MINIALL.ini
				cp ${ARG_INI_LOADER} ${LOADER_CIPHER_DIR}/MINIALL_BACKUP.ini

				DDR=`grep "Path1=bin/[^ ]*_ddr_" ${LOADER_CIPHER_DIR}/MINIALL.ini | tr -d ' '`
				if [ -n "${DDR}" ]; then
					DDR=${DDR/*=/}
					NEW_DDR=`find ${LOADER_CIPHER_DIR} -name '*ddr*bin' | head -n 1`
					${RK_SIGN_TOOL} aes --key ${AES_KEY} --nonce ${LOADER_IV} --init 1 --align 4 --bin ${NEW_DDR} ${NEW_DDR}.aes128
					sed -i "s|Path1=${DDR}|Path1=${NEW_DDR}.aes128|g" ${LOADER_CIPHER_DIR}/MINIALL.ini
				fi

				USBPLUG=`grep "Path1=bin/[^ ]*_usbplug_" ${LOADER_CIPHER_DIR}/MINIALL.ini | tr -d ' '`
				if [ -n "${USBPLUG}" ]; then
					USBPLUG=${USBPLUG/*=/}
					NEW_USBPLUG=`find ${LOADER_CIPHER_DIR} -name '*usbplug*bin' | head -n 1`
					${RK_SIGN_TOOL} aes --key ${AES_KEY} --nonce ${LOADER_IV} --init 2 --align 4 --bin ${NEW_USBPLUG} ${NEW_USBPLUG}.aes128
					sed -i "s|Path1=${USBPLUG}|Path1=${NEW_USBPLUG}.aes128|g" ${LOADER_CIPHER_DIR}/MINIALL.ini
				fi

				FlashData=`grep "FlashData=bin/[^ ]*" ${LOADER_CIPHER_DIR}/MINIALL.ini | tr -d ' '`
				if [ -n "${FlashData}" ]; then
					FlashData=${FlashData/*=/}
					NEW_FlashData=`find ${LOADER_CIPHER_DIR} -name 'FlashData.bin' | head -n 1`
					${RK_SIGN_TOOL} aes --key ${AES_KEY} --nonce ${LOADER_IV} --init 1 --align 4 --bin ${NEW_FlashData} ${NEW_FlashData}.aes128
					sed -i "s|FlashData=${FlashData}|FlashData=${NEW_FlashData}.aes128|g" ${LOADER_CIPHER_DIR}/MINIALL.ini
				fi

				FlashBoot=`grep "FlashBoot=bin/[^ ]*" ${LOADER_CIPHER_DIR}/MINIALL.ini | tr -d ' '`
				if [ -n "${FlashBoot}" ]; then
					FlashBoot=${FlashBoot/*=/}
					NEW_FlashBoot=`find ${LOADER_CIPHER_DIR} -name 'FlashBoot.bin' | head -n 1`
					cp spl/u-boot-spl.bin ${NEW_FlashBoot}
					${RK_SIGN_TOOL} aes --key ${AES_KEY} --nonce ${LOADER_IV} --init 2 --align 4 --bin ${NEW_FlashBoot} ${NEW_FlashBoot}.aes128
					sed -i "s|FlashBoot=${FlashBoot}|FlashBoot=${NEW_FlashBoot}.aes128|g" ${LOADER_CIPHER_DIR}/MINIALL.ini
				fi

				FlashBoost=`grep "FlashBoost=bin/[^ ]*" ${LOADER_CIPHER_DIR}/MINIALL.ini | tr -d ' '`
				if [ -n "${FlashBoost}" ]; then
					FlashBoost=${FlashBoost/*=/}
					NEW_FlashBoost=`find ${LOADER_CIPHER_DIR} -name 'FlashBoost.bin' | head -n 1`
					${RK_SIGN_TOOL} aes --key ${AES_KEY} --nonce ${LOADER_IV} --init 3 --align 4 --bin ${NEW_FlashBoost} ${NEW_FlashBoost}.aes128
					sed -i "s|FlashBoost=${FlashBoost}|FlashBoost=${NEW_FlashBoost}.aes128|g" ${LOADER_CIPHER_DIR}/MINIALL.ini
				fi

				${BOOT_MERGER} ${LOADER_CIPHER_DIR}/MINIALL.ini
			fi
		fi
		${RK_SIGN_TOOL} cc --chip ${ARG_CHIP: 2: 6}
		SS_FLAG=0
		if [ -n "${ARG_ROLLBACK_IDX_LOADER}" ]; then
			RK_LOADER_VER_VALUE=`printf '0x%04x%04x' "${ARG_ROLLBACK_IDX_LOADER}" "${ARG_ROLLBACK_IDX_LOADER}"`
		fi
		if grep -q '^CONFIG_SPL_REVOKE_PUB_KEY=y' .config ; then
			SS_FLAG=$((SS_FLAG | 0x80))
			${RK_SIGN_TOOL} lk --key ${LEGACY_RSA_PRI_KEY} --pubkey ${LEGACY_RSA_PUB_KEY}
			if [ -n "${ARG_ROLLBACK_IDX_LOADER}" ]; then
				${RK_SIGN_TOOL} ss --version "${RK_LOADER_VER_VALUE}"
			fi
		else
			${RK_SIGN_TOOL} lk --key ${RSA_PRI_KEY} --pubkey ${RSA_PUB_KEY}
			if [ -n "${ARG_ROLLBACK_IDX_LOADER}" ]; then
				${RK_SIGN_TOOL} ss --version "${RK_LOADER_VER_VALUE}"
			fi
		fi
		if grep -q '^CONFIG_SPL_FIT_CIPHER=y' .config ; then
			SS_FLAG=$((SS_FLAG | 0x10))
		fi
		if [ ${SS_FLAG} -ne 0 ]; then
			${RK_SIGN_TOOL} ss --flag=`printf '0x%x' ${SS_FLAG}`
		fi
		if grep -q '^CONFIG_SPL_FIT_CIPHER=y' .config ; then
			${RK_SIGN_TOOL} ss --nonce=${LOADER_IV}
		fi
		if ls *loader*.bin >/dev/null 2>&1 ; then
			${RK_SIGN_TOOL} sl --loader *loader*.bin
		fi
		if ls *download*.bin >/dev/null 2>&1 ; then
			${RK_SIGN_TOOL} sl --loader *download*.bin
		fi
		if ls *idblock*.img >/dev/null 2>&1 ; then
			${RK_SIGN_TOOL} sb --idb *idblock*.img
		fi
	fi
}

function fit_msg_uboot()
{
	MSG_CIPHER=
	if [ "${ARG_SIGN}" != "y" ]; then
		MSG_SIGN="no-signed"
	else
		MSG_SIGN="signed"
	fi
	if [ "${ARG_NO_CIPHER}" != "y" ] && grep -q '^CONFIG_SPL_FIT_CIPHER=y' .config ; then
		MSG_CIPHER="cipher,"
	fi

	VERSION=`fdtget -ti ${ITB_UBOOT} / version`
	MSG_VER=
	if [ "${VERSION}" != "" ]; then
		MSG_VER=", version=${VERSION}"
	fi

	if [ "${ARG_SPL_ROLLBACK_PROTECT}" == "y" ]; then
		echo "Image(${MSG_CIPHER}${MSG_SIGN}${MSG_VER}, rollback-index=${ARG_ROLLBACK_IDX_UBOOT}): ${IMG_UBOOT} (with uboot, trust...) is ready"
	else
		echo "Image(${MSG_CIPHER}${MSG_SIGN}${MSG_VER}): ${IMG_UBOOT} (FIT with uboot, trust...) is ready"
	fi
}

function fit_msg_boot()
{
	if [ -z "${ARG_BOOT_IMG}" ]; then
		return;
	fi

	MSG_CIPHER=
	if [ "${ARG_SIGN}" != "y" ]; then
		MSG_SIGN="no-signed"
	else
		MSG_SIGN="signed"
	fi
	if [ "${ARG_NO_CIPHER}" != "y" ] && grep -q '^CONFIG_FIT_CIPHER=y' .config ; then
		MSG_CIPHER="cipher,"
	fi

	VERSION=`fdtget -ti ${ITB_BOOT} / version`
	MSG_VER=
	if [ "${VERSION}" != "" ]; then
		MSG_VER=", version=${VERSION}"
	fi

	if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
		echo "Image(${MSG_CIPHER}${MSG_SIGN}${MSG_VER}, rollback-index=${ARG_ROLLBACK_IDX_BOOT}): ${IMG_BOOT} is ready"
	else
		echo "Image(${MSG_CIPHER}${MSG_SIGN}${MSG_VER}): ${IMG_BOOT} (FIT with kernel, fdt, resource...) is ready"
	fi
}

function fit_msg_recovery()
{
	if [ -z "${ARG_RECOVERY_IMG}" ]; then
		return;
	fi

	MSG_CIPHER=
	if [ "${ARG_SIGN}" != "y" ]; then
		MSG_SIGN="no-signed"
	else
		MSG_SIGN="signed"
	fi
	if [ "${ARG_NO_CIPHER}" != "y" ] && grep -q '^CONFIG_FIT_CIPHER=y' .config ; then
		MSG_CIPHER="cipher,"
	fi

	VERSION=`fdtget -ti ${ITB_RECOVERY} / version`
	MSG_VER=
	if [ "${VERSION}" != "" ]; then
		MSG_VER=", version=${VERSION}"
	fi

	if [ "${ARG_ROLLBACK_PROTECT}" == "y" ]; then
		echo "Image(${MSG_CIPHER}${MSG_SIGN}${MSG_VER}, rollback-index=${ARG_ROLLBACK_IDX_RECOVERY}): ${IMG_RECOVERY} is ready"
	else
		echo "Image(${MSG_CIPHER}${MSG_SIGN}${MSG_VER}): ${IMG_RECOVERY} (FIT with kernel, fdt, resource...) is ready"
	fi
}

function fit_msg_loader()
{
	MSG_CIPHER=
	LOADER=

	if ls *loader*.bin >/dev/null 2>&1 ; then
		LOADER=`ls *loader*.bin`
	fi

	if ls *idblock*.img >/dev/null 2>&1 ; then
		IDBLOCK=`ls *idblock*.img`
		if [ -n "${LOADER}" ]; then
			LOADER="${LOADER}, ${IDBLOCK}"
		else
			LOADER="${IDBLOCK}"
		fi
	fi

	if [ "${ARG_SIGN}" == "y" ]; then
		if grep -q '^CONFIG_SPL_FIT_CIPHER=y' .config ; then
			MSG_CIPHER="cipher,"
		fi
		if [ -n "${ARG_ROLLBACK_IDX_LOADER}" ]; then
			echo "Image(${MSG_CIPHER}signed, rollback-index=${ARG_ROLLBACK_IDX_LOADER}): ${LOADER} (with spl, ddr...) is ready"
		else
			echo "Image(${MSG_CIPHER}signed): ${LOADER} (with spl, ddr...) is ready"
		fi
	else
		echo "Image(no-signed): ${LOADER} (with spl, ddr...) is ready"
	fi
}

function fit_msg_u_boot_loader()
{
	LOADER=

	if ls *loader*.bin >/dev/null 2>&1 ; then
		LOADER=`ls *loader*.bin`
	fi

	if ls *idblock*.img >/dev/null 2>&1 ; then
		IDBLOCK=`ls *idblock*.img`
		if [ -n "${LOADER}" ]; then
			LOADER="${LOADER}, ${IDBLOCK}"
		else
			LOADER="${IDBLOCK}"
		fi
	fi

	if [ "${ARG_SIGN}" == "y" ]; then
		echo "Image(signed): ${LOADER} (with u-boot, ddr...) is ready"
	else
		echo "Image(no-signed): ${LOADER} (with u-boot, ddr...) is ready"
	fi
}

function fit_signcfg_export()
{
	if [ "${ARG_NO_SIGN}" == "y" ]; then
		if ls *loader*.bin >/dev/null 2>&1 ; then
			LOADER=`ls *loader*.bin`
		elif ls *download*.bin >/dev/null 2>&1 ; then
			LOADER=`ls *download*.bin`
		else
			echo "ERROR: No loader found"
			exit 1
		fi
		cp ${ARG_INI_LOADER} ${MINIALL_INI}
		cp .config ${SIG_CONFIG}
		cp ./scripts/dtc/dtc ${DTC_TOOL}

		mkdir -p ${SIG_CFG_DIR}/test_images/
		cp uboot.img ${SIG_CFG_DIR}/test_images/
		cp ${LOADER} ${SIG_CFG_DIR}/test_images/
		tar zcvf ${SIG_CFG_DIR}/test_images.tar.gz ${SIG_CFG_DIR}/test_images >/dev/null 2>&1
		rm -rf ${SIG_CFG_DIR}/test_images/

		FDT_ADDR_R=`strings env/common.o | grep 'fdt_addr_r=' | awk -F "=" '{ print $2 }'`
		KERNEL_ADDR_R=`strings env/common.o | grep 'kernel_addr_r=' | awk -F "=" '{ print $2 }'`
		RMADISK_ADDR_R=`strings env/common.o | grep 'ramdisk_addr_r=' | awk -F "=" '{ print $2 }'`
		echo "fdt_addr_r=${FDT_ADDR_R}" >> ${SIG_CONFIG}
		echo "kernel_addr_r=${KERNEL_ADDR_R}" >> ${SIG_CONFIG}
		echo "ramdisk_addr_r=${RMADISK_ADDR_R}" >> ${SIG_CONFIG}

		CSUM=`sha256sum u-boot-nodtb.bin  | awk '{ print $1 }'`
		echo "uboot_sha256sum=${CSUM}" >> ${SIG_CONFIG}
		CSUM=`sha256sum spl/u-boot-spl-nodtb.bin  | awk '{ print $1 }'`
		echo "spl_sha256sum=${CSUM}" >> ${SIG_CONFIG}
		SIZE=`ls -l  spl/u-boot-spl-nodtb.bin | awk '{ print $5 }'`
		echo "spl_size=${SIZE}" >> ${SIG_CONFIG}

		BUILD_MAIL=`git config --get user.email`
		BUILD_HOST=`hostname`
		BUILD_USER=${USER}
		BUILD_DATE=`date`
		echo "BUILD: ${BUILD_MAIL} # ${BUILD_USER}@${BUILD_HOST} # ${BUILD_DATE}" >> ${SIG_CONFIG}
	fi
}
