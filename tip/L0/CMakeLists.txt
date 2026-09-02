# ++
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Copyright (c) Nuvoton Technologies. All rights reserved.
# Licensed under the MIT license.
#
# Module Name:
#
#	CMakeLists.txt
#
# Abstract:
#
#	CMake script to build Cerberus TIP FW + Cerberus Core & Platform Unit Tests
#
# --

cmake_minimum_required(VERSION 3.12)

project(arbel_tip_fw LANGUAGES C ASM)

set(TARGET_NAME ${PROJECT_NAME}_L0.elf)


include (${CMAKE_CURRENT_LIST_DIR}/../../../../Cerberus.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/toolchain/GccArmCortexM4.cmake)

set(CORE_DIR ${CERBERUS_ROOT}/core)
set(GCC_DIR ${CERBERUS_ROOT}/projects/gcc)
set(PLATFORM_BASE_DIR ${CERBERUS_ROOT}/projects/bare_metal)
set(PLATFORM_DIR ${CERBERUS_ROOT}/projects/nuvoton/tip/platform)
set(TIP_DIR ${CERBERUS_ROOT}/projects/nuvoton/tip)
set(TIP_COMMON_DIR ${TIP_DIR}/common)

add_executable(
	${TARGET_NAME}
	${CORE_DIR}/common/buffer_util.c
	${CORE_DIR}/asn1/asn1_util.c
	${CORE_DIR}/asn1/asn1_oid.c
	${CORE_DIR}/asn1/base64_core.c
	${CORE_DIR}/asn1/ecc_der_util.c
	${CORE_DIR}/asn1/x509_extension_builder.c
	${CORE_DIR}/asn1/x509_cert_build.c
	${CORE_DIR}/asn1/dice/x509_extension_builder_dice_tcbinfo.c
	${CORE_DIR}/asn1/dice/x509_extension_builder_dice_ueid.c
	${CORE_DIR}/asn1/dme/dme_structure_raw_ecc_le.c
	${CORE_DIR}/asn1/dme/dme_structure.c
	${CORE_DIR}/asn1/dme/dme_structure_raw_ecc.c
	${CORE_DIR}/asn1/dme/x509_extension_builder_dme.c
	${CORE_DIR}/crypto/ecc_ecc_hw.c
	${CORE_DIR}/crypto/hash.c
	${CORE_DIR}/crypto/kdf.c
	${CORE_DIR}/flash/flash_common.c
	${CORE_DIR}/flash/flash_util.c
	${CORE_DIR}/flash/spi_flash.c
	${CORE_DIR}/flash/spi_flash_sfdp.c
	${CORE_DIR}/logging/debug_log.c
	${CORE_DIR}/logging/logging_flash.c
	${CORE_DIR}/riot/riot_core_common.c
	${CORE_DIR}/riot/riot_core.c
	${CORE_DIR}/riot/reference/RiotSha1.c
	${CORE_DIR}/riot/reference/RiotBase64.c
	${CORE_DIR}/riot/reference/RiotDerDec.c
	${CORE_DIR}/riot/reference/RiotDerEnc.c
	${CORE_DIR}/riot/reference/RiotX509Bldr.c
	${TIP_COMMON_DIR}/riot/riot_core_tip.c
	${TIP_COMMON_DIR}/riot/tip_dice_oid.c
	${TIP_COMMON_DIR}/cmd_device/tip_cmd_device.c
	${TIP_COMMON_DIR}/crypto/tip_security.c
	${TIP_COMMON_DIR}/crypto/tip_otp.c
	${TIP_COMMON_DIR}/flash/tip_flash.c
	${TIP_COMMON_DIR}/flash/tip_virtual_flash.c
	${TIP_COMMON_DIR}/firmware/tip_firmware_component.c
	${TIP_COMMON_DIR}/firmware/tip_key_manifest.c
	${TIP_COMMON_DIR}/firmware/tip_skmt.c
	${TIP_COMMON_DIR}/crypto/tip_ecc_hw_ncl.c
	${TIP_COMMON_DIR}/crypto/tip_hash_ncl.c
	${TIP_COMMON_DIR}/serial_printf/serial_printf.c
	${TIP_COMMON_DIR}/tip_version.c
	${TIP_COMMON_DIR}/tip_rom_ncl_funcs.s
	${TIP_COMMON_DIR}/tip_utils.c
	${TIP_COMMON_DIR}/tip_init_common.c
	${TIP_DIR}/SWC_HAL/hal.c
	${TIP_DIR}/L0/tip_fw_start_L0.s
	${TIP_DIR}/L0/tip_boot.c
	${GCC_DIR}/platform_compiler.c
	${PLATFORM_BASE_DIR}/platform_base.c
	${PLATFORM_DIR}/bare-metal/platform.c
	)

target_include_directories(
	${TARGET_NAME}
	PRIVATE
		${CORE_DIR}
		${PLATFORM_BASE_DIR}
		${PLATFORM_DIR}
		${PLATFORM_DIR}/bare-metal
		${TIP_DIR}/SWC_DEFS
		${TIP_DIR}/SWC_HAL
		${TIP_COMMON_DIR}/cmd_device
		${TIP_COMMON_DIR}/serial_printf
		${TIP_COMMON_DIR}/crypto
		${TIP_COMMON_DIR}/flash
		${TIP_COMMON_DIR}/firmware
		${TIP_COMMON_DIR}/mbx
		${TIP_COMMON_DIR}/keystore
		${TIP_COMMON_DIR}/riot
		${TIP_COMMON_DIR}
		${TIP_DIR}/L0
		${GCC_DIR}
	)

target_compile_options(
	${TARGET_NAME}
	PRIVATE
		-Os
		-fPIC   # mandatory flag for using NCLlib
		-flto
		-mthumb
		-Wno-unused-parameter
		-Wno-pedantic
		-Wno-sign-compare
		-Wno-type-limits
		-Wno-address-of-packed-member
		-Wno-maybe-uninitialized
		-Wno-format
		-fdiagnostics-color=always
		-fdiagnostics-parseable-fixits
		-fmessage-length=120
	)

target_compile_definitions(
	${TARGET_NAME}
	PUBLIC
		CHIP_NAME=npcm850_tip
		_ARBEL_Z1_
		__ARM_ARCH_7J__
		_ARM_
		EXTERNAL_INCLUDE_FILE=${TIP_DIR}/L0/hal_cfg_L0.h
		ECC_ENABLE_GENERATE_KEY_PAIR
		X509_ENABLE_CREATE_CERTIFICATES
		LOGGING_SUPPORT_DEBUG_LOG
		HASH_ENABLE_SHA384
		HASH_ENABLE_SHA1
		PRE_PRODUCTION
	)

set(LINKER_SCRIPT "${TIP_DIR}/L0/tip_fw_L0.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${LINKER_SCRIPT}")

target_link_options(
    ${TARGET_NAME}
    PRIVATE
		-Wno-maybe-uninitialized
		-Wl,--entry=__tip_fw_start_L0
		-Wl,--print-memory-usage,-nostdlib,--build-id=none,-Map=./arbel_tip_fw_L0.map
    )

add_custom_command(
	TARGET
		${TARGET_NAME}
	POST_BUILD
		COMMAND ${CMAKE_OBJDUMP} -x -s ./${TARGET_NAME} > ./arbel_tip_fw_L0.map
		COMMAND ${CMAKE_OBJCOPY} -O binary ./${TARGET_NAME} ./arbel_tip_fw_L0.bin
		COMMAND ${CMAKE_OBJDUMP} -dS ./${TARGET_NAME} > ./arbel_tip_fw_L0.asm
	)
