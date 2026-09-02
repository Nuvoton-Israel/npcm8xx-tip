/*------------------------------------------------------------------------------
 * Copyright (c) 2020 by Nuvoton Electronics Corporation
 * All rights reserved.
 *<<<---------------------------------------------------------------------------
 * File Contents:
 *     link.ld - Linker script
 * Notes:
 *
 * Project:
 *     ROM linker definition file for Arbel BMC co processor
 *------------------------------------------------------------------------->>>*/

/*------------------------------------------------------------------------------
*  0000_0000h - 0000_0FFFh 4 Kbytes 4 Kbytes Boot code.
*  0000_1000h - 0000_FFFFh none Reserved (IAD interrupt)
*  0001_0000h - 0003_FFFFh 192 Kbytes 192 Kbytes
*  							Coprocessor CP_RAM access window:
*  							Mapped to CP_RAM directly, in the coprocessor subsystem
*  							(changed in Arbel)
*  							Changed for M4:
*  							 First 128 KB on AHB_MATRIX S1
*  							 Next 64 KB on AHB_MATRIX S2
*  0004_0000h - 0007_FFFFh none Reserved (IAD interrupt) (removed in Arbel)
*  0008_0000h - 000B_FFFFh 256 Kbytes varies2
*  							BMC System RAM2 access window via SYS_AHB:
*  							RAM2 at system address FFFB_0000h (256 KB), according
*  							to SRAMWINC register (changed in Arbel)
 *------------------------------------------------------------------------->>>*/


OUTPUT_FORMAT("elf32-littlearm")

_STACK_SIZE_EACH = 0x2000;
_STACK_SIZE      = 0x4000;

MEMORY {
	rom                    (r)      :   ORIGIN = 0x0,                LENGTH = 0x20000
	ram_header             (r)      :   ORIGIN = 0x20000,            LENGTH = 0x00200
	ram_code               (x)      :   ORIGIN = 0x20200,            LENGTH = 0x29600
	ram                    (rw)     :   ORIGIN = 0x49800,            LENGTH = 0x1E900
	ram_stack              (rw)     :   ORIGIN = 0x68000,            LENGTH = _STACK_SIZE
	l0_l1_ram_share        (r)      :   ORIGIN = 0x6C000,            LENGTH = 0x2000
	tiprom_ram_share       (r)    	:   ORIGIN = 0x6E000,            LENGTH = 0x02000
	flash                  (r)      :   ORIGIN = 0x88000000,         LENGTH = 0x08000000
}

SECTIONS
{

	/* A1 ROM placement only */
	.tiprom_ram_share 0x0006EC00 (NOLOAD) :
	{
		 KEEP(*(NVIC_HW_DispatchTable))    /* 0x0006ec00 */
		 KEEP(*(NVIC_SW_DispatchTable))    /* 0x0006edc0 */
	} > tiprom_ram_share

 	.start ORIGIN(ram_code) :
 	{
 		KEEP(CMakeFiles/arbel_tip_fw_L1.elf.dir/tip_fw_start_L1.s.o(.text .text.*))
 	} > ram_code


	.text :
	{
		. = ALIGN(8);
		/* CREATE_OBJECT_SYMBOLS */
		*(.text)           /* .text sections (code) */
		*(.text*)          /* .text* sections (code) */
		*(.glue*)          /* glue arm to thumb code */

		*(.v4*)
		*(.vfp11_veneer)
		*(.eh_frame)

		KEEP (*(.init))
		KEEP (*(.fini))

		. = ALIGN(4);
		_etext = .;        /* define a global symbols at end of code */
		. = ALIGN(64);
	} > ram_code

    .rodata :
	{
		. = ALIGN(4);
		*(.rodata)
		*(.rodata.*)
		*(.gnu.linkonce.r*)
		. = ALIGN(64);
	} > ram_code


	.preinit_array     :
	{
		. = ALIGN(4);
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array*))
		PROVIDE_HIDDEN (__preinit_array_end = .);
		. = ALIGN(64);
	} >ram_code

	.init_array :
	{
		. = ALIGN(4);
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array*))
		PROVIDE_HIDDEN (__init_array_end = .);
		. = ALIGN(64);
	} >ram_code

	.fini_array :
	{
		. = ALIGN(4);
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT(.fini_array.*)))
		KEEP (*(.fini_array*))
		PROVIDE_HIDDEN (__fini_array_end = .);
		. = ALIGN(64);
	} >ram_code

	_sidata = LOADADDR(.data);

	/* Initialized data sections into "RAM" Ram type memory */
	.data :
	{
		. = ALIGN(4);
		_sdata = .;
		_data = .;        /* create a global symbol at data start */
		*(.data)           /* .data sections */
		*(.data*)          /* .data* sections */
		*(.gnu.linkonce.d*)
		. = ALIGN(64);
		_edata = .;        /* define a global symbol at data end */

	} >ram_code



	.bss (NOLOAD) :
	{
		. = ALIGN(4);
		_sbss = .;         /* define a global symbol at bss start */
		__bss_start__ = _sbss;
		*(.bss)
		*(.bss*)
		*(COMMON)
		*(.gnu.linkonce.b*)

		. = ALIGN(4);
		_ebss = .;         /* define a global symbol at bss end */
		__bss_end__ = _ebss;

		. = ALIGN(64);

	} > ram

	.stack (NOLOAD) :
	{
		PROVIDE (_stack_end = .);
		. = . + _STACK_SIZE_EACH;
		PROVIDE ( _stack_start_os = . );
		. = . + _STACK_SIZE_EACH;
		PROVIDE ( _stack_start = . );

	} >ram_stack


	.log     (0x6fb80) (NOLOAD) :
	{
		PROVIDE ( _tip_rom_log = . );
		. = ALIGN(4);
	} > tiprom_ram_share


	.boot_log       (0x6fce0) (NOLOAD) :
	{
		PROVIDE ( _tip_rom_boot_log = . );
		. = ALIGN(4);
	} > tiprom_ram_share

	.dme_dice_table (0x6f800) (NOLOAD) :
	{
		. = ALIGN(4);
		__dme_dice_table_start = .;
	} > tiprom_ram_share



	/DISCARD/ :
	{
		*(.comment)
		libc.a ( * )
		libm.a ( * )
		libgcc.a ( * )
	}

}


_ram_data_end = ORIGIN(ram) + LENGTH(ram);
_tip_rom_ram_shared_start = ORIGIN(tiprom_ram_share);
