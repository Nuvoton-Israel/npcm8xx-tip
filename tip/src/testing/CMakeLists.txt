# ++
#
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.
#
# Module Name:
#
#	CMakeLists.txt
#
# Abstract:
#
#	CMake script to build Cerberus Core, Microsoft/Nuvoton & Platform Unit Tests
#
# --

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(cerberus-nuvoton-unit-tests LANGUAGES C ASM)

include (${CMAKE_CURRENT_LIST_DIR}/../../../../../Cerberus.cmake)
include(Mbedtls)
include(AllFeatures)

set(CORE_DIR ${CERBERUS_ROOT}/core)
set(TESTING_DIR ${CERBERUS_ROOT}/testing)
set(PLATFORM_DIR ${CERBERUS_ROOT}/projects/linux)
set(MSFT_NUVOTON_DIR ${CERBERUS_ROOT}/projects/nuvoton/tip/src)
set(TIP_COMMON_DIR ${CERBERUS_ROOT}/projects/nuvoton/tip/common)
set(COMPOSITE_EAT_DIR ${TIP_COMMON_DIR}/composite_eat)

file(GLOB_RECURSE CORE_SOURCES "${CORE_DIR}/*.c")
set(CORE_INCLUDES ${CORE_DIR})

file(GLOB_RECURSE PLATFORM_SOURCES "${PLATFORM_DIR}/*.c")
set(PLATFORM_INCLUDES ${PLATFORM_DIR})

file(GLOB_RECURSE MSFT_NUVOTON_SOURCES "${MSFT_NUVOTON_DIR}/*.c")
set(MSFT_NUVOTON_INCLUDES ${MSFT_NUVOTON_DIR})

file(GLOB_RECURSE TESTING_SOURCES "${TESTING_DIR}/*.c")

set(COMPOSITE_EAT_SOURCES
	${COMPOSITE_EAT_DIR}/composite_eat_codec.c
	${COMPOSITE_EAT_DIR}/qcbor/src/UsefulBuf.c
	${COMPOSITE_EAT_DIR}/qcbor/src/qcbor_decode.c
	${COMPOSITE_EAT_DIR}/qcbor/src/qcbor_encode.c
	${COMPOSITE_EAT_DIR}/testing/composite_eat_codec_test.c
	)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

set(TARGET_NAME ${PROJECT_NAME})

add_executable(
	${TARGET_NAME}
	${MBEDTLS_SOURCES}
	${CORE_SOURCES}
	${TESTING_SOURCES}
	${PLATFORM_SOURCES}
	${MSFT_NUVOTON_SOURCES}
	${COMPOSITE_EAT_SOURCES}
	)

target_include_directories(
	${TARGET_NAME}
	PRIVATE
		${CORE_DIR}/CuTest
		${MBEDTLS_INCLUDES}
		${CORE_INCLUDES}
		${PLATFORM_INCLUDES}
		${TESTING_DIR}
		${MSFT_NUVOTON_INCLUDES}
		${MSFT_NUVOTON_INCLUDES}/testing/config
		${TIP_COMMON_DIR}
		${COMPOSITE_EAT_DIR}/qcbor/inc
		${COMPOSITE_EAT_DIR}/qcbor/inc/qcbor
	)

target_compile_options(
	${TARGET_NAME}
	PRIVATE
 		-fno-builtin
		-fdata-sections
		-Wall
		-Wextra
 		-Werror
		-Wno-unused-parameter
		-g -ggdb3
	)

target_compile_definitions(
	${TARGET_NAME}
	PRIVATE
		${CERBERUS_ALL_FEATURES}
		QCBOR_DISABLE_PREFERRED_FLOAT
		QCBOR_DISABLE_FLOAT_HW_USE
		USEFULBUF_DISABLE_ALL_FLOAT
	)

target_link_libraries(
	${TARGET_NAME}
	PRIVATE
		Threads::Threads
		OpenSSL::Crypto
		m
	)

include(Coverage)
SETUP_TARGET_FOR_COVERAGE(
	NAME coverage
 	EXECUTABLE ${PROJECT_NAME}
 	DEPENDENCIES ${PROJECT_NAME}
)