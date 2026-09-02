//-----------------------------------------------------------
// SPDX-License-Identifier: GPL-2.0
//
// Copyright ARM Ltd 2009. All rights reserved.
// Copyright (c) 2010-2023 by Nuvoton Technology Corporation
// All rights reserved
//
//-----------------------------------------------------------

.syntax unified
.arch armv7e-m

	/* Reset Handler */
	.text
	.thumb
	.thumb_func
	.align 4
	.globl Reset_Handler
	.globl __tip_fw_start_L1
	.globl main
	.globl _sbss
	.globl _ebss
	.globl _stack_end
	.globl _stack_start
	.type  Reset_Handler, %function

__tip_fw_start_L1:
	/* Disable interrupts */
	cpsid if

	/* Start clean */
	mov R0, #0
	mov R1, #0
	mov R2, #0
	mov R3, #0
	mov R4, #0
	mov R5, #0
	mov R6, #0
	mov R7, #0
	mov R8, #0
	mov R9, #0
	mov R10, #0
	mov R11, #0
	mov R12, #0
	/* LR : go back to start if something goes wrong here. point to TIP ROM reset handler*/
	mov R14, #0x40

	/* Init SP, both for Process and Thread mode.
	 * These values will later be handled by the RTOS
	 */
	ldr r0, =_stack_start
	sub r0, r0, #32
	mov sp, r0
	MSR PSP, R0
	MSR MSP, R0

	/* disable MPU */ 
	mov r0, #0
	ldr r2, =0xe000ed94 /* MPU_CTRL register. */
	str r0, [r2]

	/*
	 * APSR: Application Program Status Register
	 * Clear all status bits. 
	 */
	mov r0, #0
	MSR APSR_nzcvq, R0
	MSR XPSR_nzcvq, R0

	/*
	 * CONTROL reg: control stack useage, privilege mode and FPU .
	 * FPCA ( floating-point context active during exception): enable duing ISR
	 * SPSEL: Set SPSEL bit (PSP is the current stack pointer)
	 * nPRIV: Clear nPRIV (be in privilege mode).
	 */
	MOV R0, #4 
	MSR CONTROL, R0 /* Now Thread mode use PSP */

	/*
	 * The BASEPRI register defines the minimum priority for exception processing.
	 * Set to zero to allow all interrupts to be handled.
	 */
	mov R0, #0
	MSR BASEPRI, R0

	/*
	 * The FAULTMASK register prevents activation of all exceptions except for NMI. 
	 * Set to zero to enable all traps.
	 */
	MSR faultmask, R0

	/* Enable FPU */
	ldr r3, =0xE000ED88
	ldr r0, [r3]
	ldr r1, =0x00F00000    /* enable CP10 and CP11 Full Access((3U << 10U*2U) | (3U << 11U*2U) ) */
	ORR R0, R0, R1
	str r0, [r3]

	/*
	 * zero RAM2
	*/
	ldr r1, =0xFFFB0000
	ldr r2, =0xFFFE0000 /* leave FFFE0000 till FFFF0000 as is since it is not used by L0 */
	movs r0, 0
.L_loop3_ram2:
	cmp r1, r2
	itt lt
	strlt r0, [r1], #4
	blt .L_loop3_ram2

	/*
	 * zero the bss
	 */
	ldr r1, =_sbss
	ldr r2, =_ebss
	movs r0, 0
.L_loop3_bss:
	cmp r1, r2
	itt lt
	strlt r0, [r1], #4
	blt .L_loop3_bss

	/*
	 * zero the stack
	 */
	ldr r1, =_stack_end
	ldr r2, =_stack_start
	movs r0, 0
.L_loop3_stack:
	cmp r1, r2
	itt lt
	strlt r0, [r1], #4
	blt .L_loop3_stack


	
.jump_to_main:

	/* Jump to main */
	dmb
	dsb
	isb
	bl main
	b .
	
	.pool
	.align      4
