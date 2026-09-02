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

MEMORY {
	rom                    (r)      :   ORIGIN = 0x0,                LENGTH = 0x20000
	ram_code_src           (x)      :   ORIGIN = 0x20100,            LENGTH = (0x42000 - ORIGIN(ram_code_src))
	ram_code_dst           (x)      :   ORIGIN = 0x9E100,            LENGTH = (0xC0000 - ORIGIN(ram_code_dst))
	l0_l1_ram_share        (r)      :   ORIGIN = 0x6C000,            LENGTH = 0x2000
	ram                    (rw)     :   ORIGIN = 0x80000,            LENGTH = 0x8000
	ram_stack              (rw)     :   ORIGIN = 0x98000,            LENGTH = 0x6000
	ram_heap               (rw)     :   ORIGIN = 0x88000,            LENGTH = 0x10000
	tiprom_ram_share       (r)    	:   ORIGIN = 0x6E000,            LENGTH = 0x02000
}



_HEAP_SIZE  = LENGTH(ram_heap );
_STACK_SIZE = LENGTH(ram_stack);

SECTIONS
{

	/* A1 ROM placement only */
	.tiprom_ram_share 0x0006EC00 (NOLOAD) :
	{
		 KEEP(*(NVIC_HW_DispatchTable))    /* 0x0006ec00 */
		 KEEP(*(NVIC_SW_DispatchTable))    /* 0x0006edc0 */
	} > tiprom_ram_share

 	.start  : AT (ORIGIN(ram_code_dst))
 	{
 		KEEP(CMakeFiles/arbel_tip_fw_L0.elf.dir/tip_fw_start_L0.s.o(.text .text.*))
 	} > ram_code_dst


	.text  : AT (ORIGIN(ram_code_dst) + SIZEOF(.start))
	{
		. = ALIGN(8);
		*(.text)           /* .text sections (code) */
		*(.text*)          /* .text* sections (code) */
		*(.glue_7)         /* glue arm to thumb code */
		*(.glue_7t)        /* glue thumb to arm code */
		*(.eh_frame)

		KEEP (*(.init))
		KEEP (*(.fini))

		. = ALIGN(4);
		_etext = .;        /* define a global symbols at end of code */
		. = ALIGN(64);
	} > ram_code_dst

	.rodata   : AT (ORIGIN(ram_code_dst) + SIZEOF(.start) + SIZEOF(.text))
	{
		. = ALIGN(4);
		*(.rodata)
		*(.rodata.*)
		. = ALIGN(64);
	} > ram_code_dst

	.ARM.extab    :
	{
		. = ALIGN(4);
		*(.ARM.extab* .gnu.linkonce.armextab.*)
		. = ALIGN(64);
	} >ram_code_dst




	PROVIDE(__exidx_start = .);
	. = ALIGN(8);
    .ARM.exidx  :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > ram_code_dst
    . = ALIGN(8);
    PROVIDE(__exidx_end = .);






	.preinit_array    :
	{
		. = ALIGN(4);
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array*))
		PROVIDE_HIDDEN (__preinit_array_end = .);
		. = ALIGN(64);
	} >ram_code_dst

	.init_array   :
	{
		. = ALIGN(4);
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array*))
		PROVIDE_HIDDEN (__init_array_end = .);
		. = ALIGN(64);
	} >ram_code_dst

	.fini_array   :
	{
		. = ALIGN(4);
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT(.fini_array.*)))
		KEEP (*(.fini_array*))
		PROVIDE_HIDDEN (__fini_array_end = .);
		. = ALIGN(64);
	} >ram_code_dst

	_sidata = LOADADDR(.data);

	/* Initialized data sections into "RAM" Ram type memory */
	.data  :
	{
		. = ALIGN(4);
		_sdata = .;        /* create a global symbol at data start */
		*(.data)           /* .data sections */
		*(.data*)          /* .data* sections */
		*(.data.*)
		. = ALIGN(64);
		_edata = .;        /* define a global symbol at data end */

	} >ram_code_dst



	.bss (NOLOAD) :
	{
		. = ALIGN(4);
		_sbss = .;         /* define a global symbol at bss start */
		__bss_start__ = _sbss;
		*(.bss)
		*(.bss*)
		*(COMMON)

		. = ALIGN(4);
		_ebss = .;         /* define a global symbol at bss end */
		__bss_end__ = _ebss;

		. = ALIGN(64);



	} > ram




	.stack ORIGIN(ram_stack)  (NOLOAD) :
	{

		PROVIDE (_stack_end = .);
		. = . + _STACK_SIZE;
		PROVIDE ( _stack_start = . );



	} >ram_stack

	.heap  ORIGIN(ram_heap)   (NOLOAD) :
	{

		/* which one works on newlib? */
		PROVIDE ( _end = . );
		PROVIDE ( end = . );
		PROVIDE ( end_ = . );
		PROVIDE ( _end_ = . );

		_heap_start = .;
		. = . + _HEAP_SIZE;
		_heap_end = .;
	    PROVIDE (__malloc_sbrk_start = .);

	} >ram_heap



_ram_data_end = ORIGIN(ram) + LENGTH(ram);


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
		PROVIDE (__dme_dice_table_start = .);
	} > tiprom_ram_share

	/DISCARD/ :
	{
		*(.comment)
		libc.a ( * )
		libm.a ( * )
		libgcc.a ( * )
	}

	.ARM.attributes 0 : { *(.ARM.attributes) }

}

_ram_code_src_start = ORIGIN(ram_code_src) - 0x100;
_ram_code_dst_start = ORIGIN(ram_code_dst) - 0x100;
_ram_code_size = _edata - _ram_code_dst_start + 0x100;
_diff_code_src_dst = ORIGIN(ram_code_dst) - ORIGIN(ram_code_src);
_tip_rom_ram_shared_start = ORIGIN(tiprom_ram_share);
