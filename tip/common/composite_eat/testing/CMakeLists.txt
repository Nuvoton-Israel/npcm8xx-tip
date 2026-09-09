# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Nuvoton Technology Corporation.

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(composite-eat-rot-neutral-tests LANGUAGES C)

include(CTest)
find_package(OpenSSL REQUIRED)

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

add_executable(
	composite-eat-tip-adapter-test
	standalone_tip_adapter_test.c
	${TIP_COMMON_DIR}/tip_rom_dme_handoff.c
	${COMPOSITE_EAT_DIR}/profile_evidence.c
	${COMPOSITE_EAT_DIR}/tcg_concise_evidence.c
	${COMPOSITE_EAT_DIR}/tip_evidence_adapter.c
)

target_include_directories(
	composite-eat-tip-adapter-test
	PRIVATE
		${CERBERUS_ROOT}/core
		${CERBERUS_ROOT}/projects/linux
		${TIP_COMMON_DIR}
)

target_compile_definitions(
	composite-eat-tip-adapter-test
	PRIVATE
		COMPOSITE_EAT_TIP_EVIDENCE_NO_DEFAULT_OPS
		HASH_ENABLE_SHA384
		TIP_ROM_DME_HANDOFF_NO_DEFAULT_OPS
)

add_executable(
	composite-eat-tip-generator-test
	standalone_generator_runner.c
	tip_main_token_generator_test.c
	${CERBERUS_ROOT}/testing/CuTest/CuTest.c
	${CERBERUS_ROOT}/core/asn1/asn1_util.c
	${CERBERUS_ROOT}/core/asn1/ecc_der_util.c
	${CERBERUS_ROOT}/core/crypto/hash.c
	${CERBERUS_ROOT}/projects/linux/crypto/ecc_openssl.c
	${CERBERUS_ROOT}/projects/linux/crypto/hash_openssl.c
	${COMPOSITE_EAT_DIR}/generation_request.c
	${COMPOSITE_EAT_DIR}/main_token_builder.c
	${COMPOSITE_EAT_DIR}/profile_evidence.c
	${COMPOSITE_EAT_DIR}/tcg_concise_evidence.c
	${COMPOSITE_EAT_DIR}/tip_evidence_adapter.c
	${COMPOSITE_EAT_DIR}/tip_main_token_generator.c
	${QCBOR_DIR}/src/UsefulBuf.c
	${QCBOR_DIR}/src/qcbor_decode.c
	${QCBOR_DIR}/src/qcbor_encode.c
)

target_include_directories(
	composite-eat-tip-generator-test
	PRIVATE
		${CERBERUS_ROOT}/core
		${CERBERUS_ROOT}/projects/linux
		${CERBERUS_ROOT}/testing
		${TIP_DIR}/src
		${TIP_DIR}/src/testing/config
		${TIP_COMMON_DIR}
		${QCBOR_DIR}/inc
		${QCBOR_DIR}/inc/qcbor
)

target_compile_definitions(
	composite-eat-tip-generator-test
	PRIVATE
		COMPOSITE_EAT_TIP_EVIDENCE_NO_DEFAULT_OPS
		COMPOSITE_EAT_TIP_MAIN_TOKEN_NO_DEFAULT_OPS
		ECC_ENABLE_ECDH
		ECC_ENABLE_GENERATE_KEY_PAIR
		HASH_ENABLE_SHA384
		QCBOR_DISABLE_FLOAT_HW_USE
		QCBOR_DISABLE_PREFERRED_FLOAT
		TIP_ROM_DME_HANDOFF_NO_DEFAULT_OPS
		USEFULBUF_DISABLE_ALL_FLOAT
)

target_link_libraries(composite-eat-tip-generator-test PRIVATE OpenSSL::Crypto)

add_executable(
	composite-eat-bmc-direct-test
	bmc_direct/standalone_bmc_direct_runner.c
	bmc_direct/bmc_direct_composite_eat_status_test.c
	bmc_direct/bmc_direct_composite_eat_transport_test.c
	${CERBERUS_ROOT}/testing/CuTest/CuTest.c
	${COMPOSITE_EAT_DIR}/bmc_direct/bmc_direct_composite_eat_status.c
	${COMPOSITE_EAT_DIR}/bmc_direct/bmc_direct_composite_eat_transport.c
)

target_include_directories(
	composite-eat-bmc-direct-test
	PRIVATE
		${CERBERUS_ROOT}/core
		${CERBERUS_ROOT}/projects/linux
		${CERBERUS_ROOT}/testing
		${TIP_COMMON_DIR}
)

target_compile_features(composite-eat-platform-neutral-test PRIVATE c_std_11)
target_compile_options(composite-eat-platform-neutral-test PRIVATE -Wall -Wextra -Werror)
target_compile_features(composite-eat-rot-neutral-suite PRIVATE c_std_11)
target_compile_options(composite-eat-rot-neutral-suite PRIVATE -Wall -Wextra -Werror)
target_compile_features(composite-eat-tip-adapter-test PRIVATE c_std_11)
target_compile_options(composite-eat-tip-adapter-test PRIVATE -Wall -Wextra -Werror)
target_compile_features(composite-eat-tip-generator-test PRIVATE c_std_11)
target_compile_options(composite-eat-tip-generator-test PRIVATE -Wall -Wextra -Werror)
target_compile_features(composite-eat-bmc-direct-test PRIVATE c_std_11)
target_compile_options(composite-eat-bmc-direct-test PRIVATE -Wall -Wextra -Werror)

add_test(NAME composite-eat-platform-neutral COMMAND composite-eat-platform-neutral-test)
add_test(NAME composite-eat-rot-neutral-suite COMMAND composite-eat-rot-neutral-suite)
add_test(NAME composite-eat-tip-adapter COMMAND composite-eat-tip-adapter-test)
add_test(NAME composite-eat-tip-generator COMMAND composite-eat-tip-generator-test)
add_test(NAME composite-eat-bmc-direct COMMAND composite-eat-bmc-direct-test)
