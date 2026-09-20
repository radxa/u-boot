#!/bin/bash
#
# Copyright (c) 2020 Fuzhou Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier: GPL-2.0
#
set -e

function usage()
{
	echo
	echo "usage:"
	echo "    $0 -f [fit/itb] -o [out] [-k aes_key] [-encrypt|-decrypt]"
	echo "    -k aes_key requires -encrypt or -decrypt"
	echo
}

function args_process()
{
	if [ $# -lt 2 ]; then
		usage
		exit 1
	fi

	while [ $# -gt 0 ]; do
		case $1 in
			-f)
				ITB=$2
				shift 2
				;;
			-o)
				OUT=$2
				shift 2
				;;
			-k)
				AES_KEY=$2
				shift 2
				;;
			-encrypt)
				if [ -n "${CIPHER_MODE}" ]; then
					echo "ERROR: Only one cipher mode can be specified"
					exit 1
				fi
				CIPHER_MODE=encrypt
				shift
				;;
			-decrypt)
				if [ -n "${CIPHER_MODE}" ]; then
					echo "ERROR: Only one cipher mode can be specified"
					exit 1
				fi
				CIPHER_MODE=decrypt
				shift
				;;
			*)
				usage
				exit 1
				;;
		esac
	done

	if [ ! -f ${ITB} ]; then
		echo "ERROR: No ${ITB}"
		exit 1
	elif ! fdtget -l "${ITB}" /images >/dev/null 2>&1; then
		echo "ERROR: ${ITB} is not FIT image"
		exit 1
	fi

	if [ -z ${OUT} ]; then
		OUT="out"
	fi

	if [ -n "${CIPHER_MODE}" ] && [ -z "${AES_KEY}" ]; then
		echo "ERROR: AES key is required for -${CIPHER_MODE}"
		exit 1
	elif [ -n "${AES_KEY}" ] && [ -z "${CIPHER_MODE}" ]; then
		echo "ERROR: -encrypt or -decrypt is required with AES key"
		exit 1
	fi

	if [ -n "${AES_KEY}" ]; then
		if [ ! -f "${AES_KEY}" ]; then
			echo "ERROR: No AES key ${AES_KEY}"
			exit 1
		fi

		case "$(wc -c < "${AES_KEY}")" in
			16) AES_BITS=128 ;;
			*)
				echo "ERROR: AES key must be 16 bytes"
				exit 1
				;;
		esac
		AES_CMD="aes-${AES_BITS}-ctr"
		AES_NAME="aes${AES_BITS}"
	fi
}

function iv_hex()
{
	local iv
	iv="$(fdtget -tx "${ITB}" "/images/${1}/cipher" iv)"
	for word in ${iv}; do
		printf "%08x" "$((16#${word}))"
	done
}

function property_hex()
{
	local value
	value="$(fdtget -tx "${ITB}" "${1}" "${2}")"
	for word in ${value}; do
		printf "%08x" "$((16#${word}))"
	done
}

function file_hex()
{
	xxd -p "${1}"
}

unpack_itb()
{
	mkdir -p ${OUT}
	echo "Unpack to directory ${OUT}:"

	for NAME in `fdtget -l ${ITB} /images`
	do
		# generate ITB
		NODE="/images/${NAME}"
		OFFS=$(fdtget -ti ${ITB} ${NODE} data-position 2>/dev/null || true)
		SIZE=$(fdtget -ti ${ITB} ${NODE} data-size 2>/dev/null || true)
		if [ -z ${OFFS} ]; then
			continue;
		fi

		if [ -n "${OFFS}" ] && [ ${SIZE} -ne 0 ]; then
			dd if=${ITB} of=${OUT}/${NAME} bs=${SIZE} count=1 skip=${OFFS} iflag=skip_bytes >/dev/null 2>&1
		elif [ -n "${OFFS}" ]; then
			touch ${OUT}/${NAME}
		fi

		# hash verify
		ALGO=`fdtget -ts ${ITB} ${NODE}/hash algo`
		if [ -z ${ALGO} ]; then
			printf "    %-20s: %d bytes" ${NAME} ${SIZE}
		else
			VALUE=`fdtget -tx ${ITB} ${NODE}/hash value`
			VALUE=`echo " "${VALUE} | sed "s/ / 0x/g"`
			CSUM=`"${ALGO}"sum ${OUT}/${NAME} | awk '{ print $1}'`

			HASH=""
			for((i=1;;i++));
			do
				HEX=`echo ${VALUE} | awk -v idx=$i '{ print $idx }'`
				if [ -z ${HEX} ]; then
					break;
				fi

				HEX=`printf "%08x" ${HEX}`
				HASH="${HASH}${HEX}"
			done

			printf "  %-20s: %d bytes... %s" ${NAME} ${SIZE} ${ALGO}
			if [ "${CSUM}" == "${HASH}" -o ${SIZE} -eq 0 ]; then
				echo "+"
			else
				echo "-"
			fi
		fi

		if [ "${CIPHER_MODE}" == "encrypt" ] && fdtget -l "${ITB}" "/images/${NAME}/cipher" >/dev/null 2>&1; then
			echo "ERROR: Image ${NAME} is already encrypted"
			exit 1
		fi

		if [ "${CIPHER_MODE}" != "encrypt" ] && fdtget -l "${ITB}" "/images/${NAME}/cipher" >/dev/null 2>&1; then
			if [ -z "${AES_KEY}" ]; then
				echo "ERROR: AES key is required to unpack ${NAME}"
				exit 1
			fi

			DEC=$(mktemp "${OUT}/.fit-unpack.XXXXXX")
			openssl enc -d "-${AES_CMD}" -in "${OUT}/${NAME}" -out "${DEC}" \
				-K "$(xxd -p "${AES_KEY}" | tr -d '\n')" -iv "$(iv_hex "${NAME}")"

			# NOTE: Move decrypted data to be NAME image under 'compression' state.
			mv "${DEC}" "${OUT}/${NAME}"

			CIPHER_HASH=$(property_hex "/images/${NAME}/cipher/hash" value)
			CSUM=$(sha256sum "${OUT}/${NAME}" | awk '{print $1}')
			if [ "${CSUM}" != "${CIPHER_HASH}" ]; then
				echo "ERROR: Cipher hash mismatch for ${NAME}"
				exit 1
			fi
		fi
	done
	echo
}

function gen_its()
{
	ITS=${OUT}/image.its
	TMP_ITB=${OUT}/image.tmp

	# add placeholder
	cp -a ${ITB} ${TMP_ITB}

	# data and digest value
	# NOTE: at this moment, compression/non-compression NAME image is decrypt in unpack_itb() if it's encrypt.
	for NAME in `fdtget -l ${ITB} /images`; do
		COMPRESSION=`fdtget -ts ${ITB} /images/${NAME} compression`
		if [ "${COMPRESSION}" == "gzip" ] && fdtget -l "${TMP_ITB}" /images/${NAME}/digest >/dev/null 2>&1; then
			COMP_NAME=${NAME}.gz
			fdtput -t s ${TMP_ITB} /images/${NAME} data "/INCBIN/(${COMP_NAME})"

			mv ${OUT}/${NAME} ${OUT}/${COMP_NAME}
			gzip -dk ${OUT}/${COMP_NAME}
			openssl dgst -sha256 -binary -out ${OUT}/${NAME}.digest ${OUT}/${NAME}
			fdtput -t s ${TMP_ITB} /images/${NAME}/digest digest "/INCBIN/(${NAME}.digest)"
		elif [ "${COMPRESSION}" == "lzma" ] && fdtget -l "${TMP_ITB}" /images/${NAME}/digest >/dev/null 2>&1; then
			COMP_NAME=${NAME}.lzma
			fdtput -t s ${TMP_ITB} /images/${NAME} data "/INCBIN/(${COMP_NAME})"

			SIZE=`ls -l ${OUT}/${NAME} | awk '{ print $5 }'`
			SIZE=$(echo "obase=10;$(($SIZE-4))"|bc)
			cp ${OUT}/${NAME} ${OUT}/${COMP_NAME}.bak
			dd if=${OUT}/${NAME} of=${OUT}/${COMP_NAME} bs=${SIZE} count=1 >/dev/null 2>&1
			lzma -df ${OUT}/${COMP_NAME}
			openssl dgst -sha256 -binary -out ${OUT}/${NAME}.digest ${OUT}/${NAME}
			mv ${OUT}/${COMP_NAME}.bak ${OUT}/${COMP_NAME}
			fdtput -t s ${TMP_ITB} /images/${NAME}/digest digest "/INCBIN/(${NAME}.digest)"

		else
			COMP_NAME=${NAME}
			fdtput -t s ${TMP_ITB} /images/${NAME} data "/INCBIN/(${NAME})"
		fi

		if [ "${CIPHER_MODE}" == "encrypt" ]; then
			IV_FILE=${COMP_NAME}.iv
			CIPHER_FILE=${COMP_NAME}.${AES_NAME}
			CIPHER_DIGEST=${COMP_NAME}.digest
			openssl rand -out "${OUT}/${IV_FILE}" 16
			openssl dgst -sha256 -binary -out "${OUT}/${CIPHER_DIGEST}" "${OUT}/${COMP_NAME}"
			# NOTE: create a encrypt image.
			openssl enc "-${AES_CMD}" -in "${OUT}/${COMP_NAME}" -out "${OUT}/${CIPHER_FILE}" \
				-K "$(file_hex "${AES_KEY}")" -iv "$(file_hex "${OUT}/${IV_FILE}")"
			fdtput -p -t s "${TMP_ITB}" /images/${NAME}/cipher algo "${AES_NAME}"
			fdtput -t s "${TMP_ITB}" /images/${NAME}/cipher iv "/INCBIN/(${IV_FILE})"
			fdtput -p -t s "${TMP_ITB}" /images/${NAME}/cipher/hash algo sha256
			fdtput -t s "${TMP_ITB}" /images/${NAME}/cipher/hash value "/INCBIN/(${CIPHER_DIGEST})"
			fdtput -t s "${TMP_ITB}" /images/${NAME} data "/INCBIN/(${CIPHER_FILE})"
		elif fdtget -l "${TMP_ITB}" /images/${NAME}/cipher >/dev/null 2>&1; then
			IV_FILE=${COMP_NAME}.iv
			CIPHER_FILE=${COMP_NAME}.${AES_NAME}
			CIPHER_DIGEST=${COMP_NAME}.digest
			printf "%s" "$(iv_hex "${NAME}")" | xxd -r -p > "${OUT}/${IV_FILE}"
			openssl dgst -sha256 -binary -out "${OUT}/${CIPHER_DIGEST}" "${OUT}/${COMP_NAME}"
			# NOTE: restore to encrypt image.
			openssl enc "-${AES_CMD}" -in "${OUT}/${COMP_NAME}" -out "${OUT}/${CIPHER_FILE}" \
				-K "$(file_hex "${AES_KEY}")" -iv "$(iv_hex "${NAME}")"
			fdtput -t s "${TMP_ITB}" /images/${NAME} data "/INCBIN/(${CIPHER_FILE})"
			fdtput -t s "${TMP_ITB}" /images/${NAME}/cipher algo "${AES_NAME}"
			fdtput -t s "${TMP_ITB}" /images/${NAME}/cipher iv "/INCBIN/(${IV_FILE})"
			fdtput -t s "${TMP_ITB}" /images/${NAME}/cipher/hash value "/INCBIN/(${CIPHER_DIGEST})"
		fi
	done

	DTC=$(find $(cd `dirname $0`; pwd) -type f -name "dtc" | head -n 1)
	if [ ! -f ${DTC} ]; then
		echo "ERROR: No DTC tool"
		exit 1
	fi
	${DTC} -I dtb -O dts ${TMP_ITB} -o ${ITS}

	rm -f ${TMP_ITB}

	# fixup placeholder: data = "/INCBIN/(...)"; -> data = /incbin/("...");
	sed -i "s/\"\/INCBIN\/(\(.*\))\"/\/incbin\/(\"\1\")/" ${ITS}

	# remove
	sed -i "/memreserve/d"		${ITS}
	sed -i "/timestamp/d"		${ITS}
	sed -i "/data-size/d"		${ITS}
	sed -i "/data-position/d"	${ITS}
	sed -i "/value = <.*>/d"	${ITS}
	sed -i "/hashed-strings/d"	${ITS}
	sed -i "/hashed-nodes/d"	${ITS}
	sed -i "/signer-version/d"	${ITS}
	sed -i "/signer-name/d"		${ITS}
	sed -i "/version/d"		${ITS}
	sed -i "/totalsize/d"		${ITS}
	sed -i "s/digest =/value =/g"	${ITS}
}

# Never change order !
args_process $*
unpack_itb
gen_its

