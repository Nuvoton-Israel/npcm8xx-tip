# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Nuvoton Technology Corporation.

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(composite-eat-rot-neutral-tests LANGUAGES C)

include(CTest)

get_filename_component(TIP_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(TIP_COMMON_DIR "${TIP_DIR}/common")
set(COMPOSITE_EAT_DIR "${TIP_COMMON_DIR}/composite_eat")
set(QCBOR_DIR "${COMPOSITE_EAT_DIR}/qcbor")

add_executable(
	composite-eat-platform-neutral-test
	standalone_platform_neutral_test.c
	${COMPOSITE_EAT_DIR}/generation_request.c
	${COMPOSITE_EAT_DIR}/main_token_builder.c
	${COMPOSITE_EAT_DIR}/profile_evidence.c
	${COMPOSITE_EAT_DIR}/tcg_concise_evidence.c
	${QCBOR_DIR}/src/UsefulBuf.c
	${QCBOR_DIR}/src/qcbor_decode.c
)

target_include_directories(
	composite-eat-platform-neutral-test
	PRIVATE
		${TIP_COMMON_DIR}
		${QCBOR_DIR}/inc
		${QCBOR_DIR}/inc/qcbor
)

target_compile_definitions(
	composite-eat-platform-neutral-test
	PRIVATE
		QCBOR_DISABLE_FLOAT_HW_USE
		QCBOR_DISABLE_PREFERRED_FLOAT
		USEFULBUF_DISABLE_ALL_FLOAT
)

get_filename_component(CERBERUS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../../.." ABSOLUTE)

add_executable(
	composite-eat-rot-neutral-suite
	standalone_rot_neutral_runner.c
	generation_request_test.c
	main_token_builder_test.c
	tcg_concise_evidence_test.c
	${CERBERUS_ROOT}/testing/CuTest/CuTest.c
	${COMPOSITE_EAT_DIR}/generation_request.c
	${COMPOSITE_EAT_DIR}/main_token_builder.c
	${COMPOSITE_EAT_DIR}/profile_evidence.c
	${COMPOSITE_EAT_DIR}/tcg_concise_evidence.c
	${QCBOR_DIR}/src/UsefulBuf.c
	${QCBOR_DIR}/src/qcbor_decode.c
	${QCBOR_DIR}/src/qcbor_encode.c
)

target_include_directories(
	composite-eat-rot-neutral-suite
	PRIVATE
		${CERBERUS_ROOT}/core
		${CERBERUS_ROOT}/projects/linux
		${CERBERUS_ROOT}/testing
		${TIP_COMMON_DIR}
		${QCBOR_DIR}/inc
		${QCBOR_DIR}/inc/qcbor
)

target_compile_definitions(
	composite-eat-rot-neutral-suite
	PRIVATE
		QCBOR_DISABLE_FLOAT_HW_USE
		QCBOR_DISABLE_PREFERRED_FLOAT
		USEFULBUF_DISABLE_ALL_FLOAT
)

target_compile_features(composite-eat-platform-neutral-test PRIVATE c_std_11)
target_compile_options(composite-eat-platform-neutral-test PRIVATE -Wall -Wextra -Werror)
target_compile_features(composite-eat-rot-neutral-suite PRIVATE c_std_11)
target_compile_options(composite-eat-rot-neutral-suite PRIVATE -Wall -Wextra -Werror)

add_test(NAME composite-eat-platform-neutral COMMAND composite-eat-platform-neutral-test)
add_test(NAME composite-eat-rot-neutral-suite COMMAND composite-eat-rot-neutral-suite)
