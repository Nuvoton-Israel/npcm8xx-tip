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

set(TARGET_NAME ${PROJECT_NAME}_L1.elf)


include (${CMAKE_CURRENT_LIST_DIR}/../../../../Cerberus.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/toolchain/GccArmCortexM4.cmake)

include(Mbedtls)

set(CORE_DIR ${CERBERUS_ROOT}/core)
set(GCC_DIR ${CERBERUS_ROOT}/projects/gcc)
set(PLATFORM_DIR ${CERBERUS_ROOT}/projects/nuvoton/tip/platform)
set(FREE_RTOS_DIR ${CERBERUS_ROOT}/projects/nuvoton/tip/freertos)
set(CERBERUS_FREE_RTOS_DIR ${CERBERUS_ROOT}/projects/freertos)
set(TIP_DIR ${CERBERUS_ROOT}/projects/nuvoton/tip)
set(TIP_COMMON_DIR ${TIP_DIR}/common)

add_executable(
	${TARGET_NAME}
	${MBEDTLS_SOURCES}
	${CORE_DIR}/attestation/attestation_responder.c
	${CORE_DIR}/attestation/pcr.c
	${CORE_DIR}/attestation/pcr_store.c
	${CORE_DIR}/cmd_interface/cerberus_protocol_debug_commands.c
	${CORE_DIR}/cmd_interface/cerberus_protocol_diagnostic_commands.c
	${CORE_DIR}/cmd_interface/cerberus_protocol_master_commands.c
	${CORE_DIR}/cmd_interface/cerberus_protocol_optional_commands.c
	${CORE_DIR}/cmd_interface/cerberus_protocol_required_commands.c
	${CORE_DIR}/cmd_interface/cmd_channel.c
	${CORE_DIR}/cmd_interface/cmd_interface.c
	${CORE_DIR}/cmd_interface/cmd_interface_system.c
	${CORE_DIR}/cmd_interface/cmd_background_handler.c
	${CORE_DIR}/cmd_interface/device_manager.c
	${CORE_DIR}/cmd_interface/session_manager.c
	${CORE_DIR}/cmd_interface/session_manager_ecc.c
	${CORE_DIR}/common/authorization_allowed.c
	${CORE_DIR}/common/authorization_challenge.c
	${CORE_DIR}/common/authorization_disallowed.c
	${CORE_DIR}/common/buffer_util.c
	${CORE_DIR}/common/observable.c
	${CORE_DIR}/crypto/base64_mbedtls.c
	${CORE_DIR}/crypto/checksum.c
	${CORE_DIR}/crypto/ecc_thread_safe.c
	${CORE_DIR}/crypto/ecc_der_util.c
	${CORE_DIR}/crypto/ecc_ecc_hw.c
	${CORE_DIR}/crypto/asn1.c
	${CORE_DIR}/crypto/hash.c
	${CORE_DIR}/crypto/hash_thread_safe.c
	${CORE_DIR}/crypto/kdf.c
	${CORE_DIR}/crypto/rng_thread_safe.c
	${CORE_DIR}/crypto/x509_mbedtls.c
	${CORE_DIR}/crypto/x509_thread_safe.c
	${CORE_DIR}/firmware/firmware_header.c
	${CORE_DIR}/firmware/firmware_update.c
	${CORE_DIR}/flash/flash_common.c
	${CORE_DIR}/flash/flash_updater.c
	${CORE_DIR}/flash/flash_util.c
	${CORE_DIR}/flash/flash_store_contiguous_blocks.c
	${CORE_DIR}/flash/spi_flash.c
	${CORE_DIR}/flash/spi_flash_sfdp.c
	${CORE_DIR}/logging/debug_log.c
	${CORE_DIR}/logging/logging_flash.c
	${CORE_DIR}/keystore/keystore_flash.c
	${CORE_DIR}/mctp/cmd_interface_mctp_control.c
	${CORE_DIR}/mctp/mctp_control_protocol_commands.c
	${CORE_DIR}/mctp/mctp_interface.c
	${CORE_DIR}/mctp/mctp_base_protocol.c
	${CORE_DIR}/riot/riot_core.c
	${CORE_DIR}/riot/riot_key_manager.c
	${CORE_DIR}/system/system.c
	${CORE_DIR}/system/event_task.c
	${FREE_RTOS_DIR}/Source/croutine.c
	${FREE_RTOS_DIR}/Source/event_groups.c
	${FREE_RTOS_DIR}/Source/list.c
	${FREE_RTOS_DIR}/Source/queue.c
	${FREE_RTOS_DIR}/Source/tasks.c
	${FREE_RTOS_DIR}/Source/timers.c
	${FREE_RTOS_DIR}/Source/portable/GCC/ARM_CM4F/port.c
	${FREE_RTOS_DIR}/Source/portable/MemMang/heap_4.c
	${CERBERUS_FREE_RTOS_DIR}/event_task_freertos.c
	${CERBERUS_FREE_RTOS_DIR}/platform.c
	${CERBERUS_FREE_RTOS_DIR}/platform_clock_freertos.c
	${TIP_COMMON_DIR}/deprecated/mctp_cmd_task.c
	${TIP_COMMON_DIR}/cmd_device/tip_cmd_device.c
	${TIP_COMMON_DIR}/cmd_interface/cmd_interface_tip.c
	${TIP_COMMON_DIR}/crypto/tip_security.c
	${TIP_COMMON_DIR}/crypto/tip_otp.c
	${TIP_COMMON_DIR}/flash/tip_flash.c
	${TIP_COMMON_DIR}/flash/tip_virtual_flash.c
	${TIP_COMMON_DIR}/firmware/tip_firmware_component.c
	${TIP_COMMON_DIR}/firmware/tip_app_context.c
	${TIP_COMMON_DIR}/firmware/tip_image_combo.c
	${TIP_COMMON_DIR}/firmware/tip_key_manifest.c
	${TIP_COMMON_DIR}/firmware/tip_skmt.c
	${TIP_COMMON_DIR}/crypto/tip_ecc_hw_ncl.c
	${TIP_COMMON_DIR}/crypto/tip_hash_ncl.c
	${TIP_COMMON_DIR}/crypto/tip_rng_ncl.c
	${TIP_COMMON_DIR}/crypto/tip_aes_ncl.c
	${TIP_COMMON_DIR}/tip_fw_update_task.c
	${TIP_COMMON_DIR}/bmc_task.c
	${TIP_COMMON_DIR}/mbx/tip_mbx.c
	${TIP_COMMON_DIR}/serial_printf/serial_printf.c
	${TIP_COMMON_DIR}/tip_version.c
	${TIP_COMMON_DIR}/tip_utils.c
	${TIP_COMMON_DIR}/tip_init_common.c
	${TIP_COMMON_DIR}/tip_stack_overflow_check.c
	${TIP_COMMON_DIR}/tip_rom_ncl_funcs.s
	${TIP_DIR}/SWC_HAL/hal.c
	${TIP_DIR}/L1/tip_fw_start_L1.s
	${TIP_DIR}/L1/tip_cerberus.c
	${GCC_DIR}/platform_compiler.c
	)

target_include_directories(
	${TARGET_NAME}
	PRIVATE
		${MBEDTLS_INCLUDES}
		${CORE_DIR}
		${PLATFORM_DIR}
		${FREE_RTOS_DIR}/Source/include
		${FREE_RTOS_DIR}/Source/portable/GCC/ARM_CM4F
		${CERBERUS_FREE_RTOS_DIR}
		${TIP_DIR}/SWC_DEFS
		${TIP_DIR}/SWC_HAL
		${TIP_COMMON_DIR}/cmd_device
		${TIP_COMMON_DIR}/cmd_interface
		${TIP_COMMON_DIR}/serial_printf
		${TIP_COMMON_DIR}/crypto
		${TIP_COMMON_DIR}/flash
		${TIP_COMMON_DIR}/firmware
		${TIP_COMMON_DIR}/mbx
		${TIP_COMMON_DIR}
		${TIP_DIR}/L1
        ${TIP_DIR}
		${GCC_DIR}
	)

target_compile_options(
	${TARGET_NAME}
	PRIVATE
        -Os
		-mthumb
		-flto
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
 		-DNPCM845_TIP
 		-D_ARBEL_Z2_
		-D_PLATFORM_RTOS_
	)

target_compile_definitions(
	${TARGET_NAME}
	PUBLIC
		_PLATFORM_RTOS_
		CHIP_NAME=npcm850_tip
		_ARBEL_Z1_
		__ARM_ARCH_7J__
		_ARM_
		_TIP_DEBUG_BUILD_
		EXTERNAL_INCLUDE_FILE=${TIP_DIR}/L1/hal_cfg_L1.h
		CMD_ENABLE_DEBUG_LOG
		ECC_ENABLE_GENERATE_KEY_PAIR
		ECC_ENABLE_ECDH
		X509_ENABLE_AUTHENTICATION
		LOGGING_SUPPORT_DEBUG_LOG
		FORCE_RECOVERY_IMAGE_MATCH_ACTIVE
		MBEDTLS_CONFIG_FILE="${TIP_DIR}/common/crypto/tip_mbedtls_config.h"
		PRE_PRODUCTION
	)

set(LINKER_SCRIPT "${TIP_DIR}/L1/tip_fw_L1.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${LINKER_SCRIPT}")

target_link_options(
	${TARGET_NAME}
	PRIVATE
		-Wno-maybe-uninitialized
		-Wl,--entry=__tip_fw_start_L1
		-Wl,--print-memory-usage,-nostdlib,--build-id=none,-Map=./arbel_tip_fw_L1.map,--entry=__tip_fw_start_L1
	)

add_custom_command(
    TARGET
        ${TARGET_NAME}
	POST_BUILD
        COMMAND ${CMAKE_OBJDUMP} -x -s ./${TARGET_NAME} > ./arbel_tip_fw_L1.map
        COMMAND ${CMAKE_OBJCOPY} -O binary ./${TARGET_NAME} ./arbel_tip_fw_L1.bin
        COMMAND ${CMAKE_OBJDUMP} -dS ./${TARGET_NAME} > ./arbel_tip_fw_L1.asm
    )