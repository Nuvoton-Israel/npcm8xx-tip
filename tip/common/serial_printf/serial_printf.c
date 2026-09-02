/*
 *  Nuvoton Technology Corporation confidential
 *
 *  Copyright (c) 2014 by Nuvoton Technology Corporation
 *  All rights reserved
 *
 *<<<--------------------------------------------------------------------------------
 * File Contents:
 *   serial_printf.c
 *            This file contains implementation of serial printf
 *  Project:
 *            SWC HAL
 */
#include "hal.h"
#include "serial_printf.h"
#include "stdarg.h"
#include "tip_rom_utils.h"
#include "platform_io.h"
#include "tip_boot.h"

/* Setting default configurations */
UART_DEV_T gUartLog = UART0_DEV;

#ifndef SERIAL_PRINTF_BUFFER_SIZE
#define SERIAL_PRINTF_BUFFER_SIZE _512B_
#endif


/* Local defines */
#define ZEROPAD  1       /* pad with zero */
#define SIGN     2       /* unsigned/signed long */
#define PLUS     4       /* show plus */
#define SPACE    8       /* space if plus */
#define LEFT     16      /* left justified */
#define SPECIAL  32      /* hex. don't add 0x */
#define LARGE    64      /* use 'ABCDEF' instead of 'abcdef' */
#define SPECIAL2 128     /* hex. add 0x */

/* Local Macros */
#define is_digit(c) ((c) >= '0' && (c) <= '9')

/* Local variables */
static char buf[SERIAL_PRINTF_BUFFER_SIZE] = { 0 };
#ifdef TIP_DEBUG_BUILD
static bool is_uart_print_enabled_var;
static bool is_memory_print_enabled_var;
#else
#define is_uart_print_enabled_var 0
#define is_memory_print_enabled_var 0
#endif
extern struct log_buffer_t tip_log_buffer;

/**
 * @function    serial_puts
 *
 * @param      s - input string
 *
 * @return    none
 * @brief
 *       This routine writes complete string to UART
 */
static void serial_puts (const char *s)
{
	int cnt = 0;

	/* Check TX UART is empty */
	while (UART_TestTX (gUartLog) == TRUE);

	while (*s) {
		if (*s == '\n') {
			UART_PutC_NB (gUartLog, '\r');
			cnt++;
		}
		/* Puting the char to serial, none-blocking */
		UART_PutC_NB (gUartLog, *s);
		s++;
		cnt++;

		/* did we fill the UART FIFO (16 bytes) ? 
		 * If yes, let's wait for the FIFO to clear
		 */
		if ((cnt % 16) == 0) {
			while (UART_TestTX (gUartLog) == TRUE);
			cnt = 0;
		}
	}
}

int fputc (int ch, FILE *f)
{
	serial_puts ((const char *) &ch);
	return ch;
}

/**
 * @function    serial_strnlen
 *
 * @param      count -
 * @param      s -
 *
 * @return
 * @brief
 *       This routine calculates string size
 */
static int serial_strnlen (const char *s, int count)
{
	const char *sc;
	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

/**
 * @function    skip_atoi
 *
 *
 * @param      s -
 *
 * @return
 * @brief
 *       This routine performs a string to an interger conversion of a positive interger
 */
static int skip_atoi (const char **s)
{
	int i = 0;
	while (is_digit (**s)) {
		i = i * 10 + *((*s)++) - '0';
	}
	return i;
}

/**
 * @function    do_div
 *
 *
 * @param      base -
 * @param      n -
 *
 * @return
 * @brief
 *       This routine performs division with reminder
 */
static long do_div (long *n, long base)
{
	int __res;
	__res = (*n) % base;
	*n = (*n) / base;
	return __res;
}

/**
 * @function    do_div_unsigned
 *
 *
 * @param      base -
 * @param      n -
 *
 * @return
 * @brief
 *       This routine performs division with reminder
 */
static unsigned long do_div_unsigned (unsigned long *n, unsigned long base)
{
	unsigned int __res;
	__res = (*n) % base;
	*n = (*n) / base;
	return __res;
}

/**
 * @function    number
 *
 *
 * @param      base -
 * @param      num -
 * @param      precision -
 * @param      size -
 * @param      str -
 * @param      end -
 * @param      type -
 *
 * @return
 * @brief
 *       This routine used for printf number conversions
 */
static char *number (char *str, const char *end, long num, unsigned int base, int size, int precision, int type)
{
	char		  c, sign, tmp[66];
	const char	 *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	int			  i;
	unsigned long unum = (unsigned long) num;
	if (type & LARGE)
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if ((type & SIGN) && ((type & SPECIAL) == 0)) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		} else if (type & PLUS) {
			sign = '+';
			size--;
		} else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}
	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else {
		if (type & SPECIAL) {
			while (unum != 0)
				tmp[i++] = digits[do_div_unsigned (&unum, (unsigned long) base)];
		} else {
			while (num != 0)
				tmp[i++] = digits[do_div (&num, base)];
		}
	}
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type & (ZEROPAD + LEFT)))
		while ((size-- > 0) && (str < end))
			*str++ = ' ';
	if (sign && (str < end))
		*str++ = sign;
	
	/* add 0x prefix */
	if (type & SPECIAL2) {
		if ((base == 8) && (str < end))
			*str++ = '0';
		else if ((base == 16) && (str < end)) {
			*str++ = '0';
			if (str < end)
				*str++ = digits[33];
		}
	}
	if (!(type & LEFT))
		while ((size-- > 0) && (str < end))
			*str++ = c;
	while ((i < precision--) && (str < end))
		*str++ = '0';
	while ((i-- > 0) && (str < end))
		*str++ = tmp[i];
	while ((size-- > 0) && (str < end))
		*str++ = ' ';
	return str;
}

/**
 * @function    serial_vsnprintf
 *
 *
 * @param      args -
 * @param      buf -
 * @param      size -
 * @param      fmt -
 *
 * @return
 * @brief
 *       This routine parses format string and dumps the output to buffer
 */
static int serial_vsnprintf (char *buf, size_t size, const char *fmt, const va_list args)
{
	int			len = 0;
	long		num = 0;
	int			i, base = 0;
	char	   *str;
	const char *s;
	int flags = 0; /* flags to number() */
	int field_width = 16; /* width of output field */
	int precision = 0;	  /* min. # of digits for integers; max number of chars for from string */
	int qualifier = 0;	  /* 'h', 'l', or 'q' for integer fields */
	char *end = buf + size - 1; /* leave space for null terminator */

	if (size == 0)
		return 0;

	for (str = buf; *fmt && (str < end); ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		/* process flags */
		flags = 0;
repeat:
		/* this also skips first '%' */
		++fmt;
		switch (*fmt) {
			case '-':
				flags |= LEFT;
				goto repeat;
			case '+':
				flags |= PLUS;
				goto repeat;
			case ' ':
				flags |= SPACE;
				goto repeat;
			case '#':
				flags |= SPECIAL2;
				goto repeat;
			case '0':
				flags |= ZEROPAD;
				goto repeat;
			default:
				break;
		}

		/* get field width */
		field_width = -1;
		if (is_digit (*fmt))
			field_width = skip_atoi (&fmt);
		else if (*fmt == '*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg (args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;
			if (is_digit (*fmt))
				precision = skip_atoi (&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg (args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z' || *fmt == 'z' ||
			*fmt == 't' || *fmt == 'q') {
			qualifier = *fmt;
			if (qualifier == 'l' && *(fmt + 1) == 'l') {
				qualifier = 'q';
				++fmt;
			}
			++fmt;
		}

		/* default base */
		base = 10;
		switch (*fmt) {
			case 'c':
				if (!(flags & LEFT))
					while ((--field_width > 0) && (str < end))
						*str++ = ' ';
				if (str < end)
					*str++ = (unsigned char) va_arg (args, int);
				while ((--field_width > 0) && (str < end))
					*str++ = ' ';
				continue;
			case 's':
				s = va_arg (args, char *);
				if (!s)
					s = "<NULL>";
				len = serial_strnlen (s, precision);
				if (!(flags & LEFT))
					while ((len < field_width--) && (str < end))
						*str++ = ' ';
				for (i = 0; (i < len) && (str < end); ++i)
					*str++ = *s++;
				while ((len < field_width--) && (str < end))
					*str++ = ' ';
				continue;
			case 'p':
				if (field_width == -1) {
					field_width = 2 * sizeof (void *);
					flags |= ZEROPAD;
				}
				str = number (str, end, (unsigned long) va_arg (args, void *), 16, field_width,
							  precision, flags);
				continue;
			case 'n':
				if (qualifier == 'l') {
					long *ip = va_arg (args, long *);
					*ip = (str - buf);
				} else {
					int *ip = va_arg (args, int *);
					*ip = (str - buf);
				}
				continue;
			case '%':
				*str++ = '%';
				continue;

				/* integer number formats - set up the flags and "break" */
			case 'o':
				base = 8;
				break;
			case 'X':
				flags |= LARGE;
				/* fall through */
			case 'x':
				flags |= SPECIAL;
				flags &= ~SIGN;
				flags &= ~LEFT;
				flags |= ZEROPAD;
				base = 16;
				break;
			case 'd':
			case 'i':
				flags |= SIGN;
			case 'u':
				break;
			default:
				*str++ = '%';
				if (*fmt && (str < end))
					*str++ = *fmt;
				else
					--fmt;
				continue;
		}
		if (qualifier == 'l') {
			if (flags & SPECIAL) {
				precision = 8;
			}
			num = va_arg (args, unsigned long);
		} else if (qualifier == 'Z' || qualifier == 'z') {
			num = va_arg (args, unsigned int);
		} else if (qualifier == 't') {
			num = va_arg (args, int);
		} else if (qualifier == 'h') {
			num = (unsigned short) va_arg (args, int);
			if (flags & SIGN)
				num = (short) num;
		} else if (flags & SIGN)
			num = va_arg (args, int);
		else
			num = va_arg (args, unsigned int);
		str = number (str, end, num, base, field_width, precision, flags);
	}
	*str = '\0';
	return str - buf;
}

/**
 * @brief Removes all sequences starting with 0x1b and ending with 0x6d (inclusive) from src,
 *        copying the result to dst. Returns the new size.
 * @param dst      Output buffer (str_temp)
 * @param src      Input buffer (str)
 * @param src_size Input buffer size (buf_size)
 * @return         New size of dst
 */
static int remove_esc_sequences (char *src, int src_size)
{
	int src_idx = 0, dst_idx = 0;
	while (src_idx < src_size) {
		if ((unsigned char) src[src_idx] == 0x1b) {
			/* Skip until and including next 0x6d or end of buffer */
			src_idx++;
			while (src_idx < src_size && (unsigned char) src[src_idx] != 0x6d) {
				src_idx++;
			}
			if (src_idx < src_size && (unsigned char) src[src_idx] == 0x6d) {
				src_idx++; /*  Skip the 0x6d itself */
			}
		} else {
			src[dst_idx++] = src[src_idx++];
		}
	}
	src[dst_idx] = '\0';
	return dst_idx;
}

/**
 * @function    memory_puts
 *
 * @param      s - input string , buf_size - the size of the input
 *
 * @return    none
 * @brief
 *       This routine writes complete string to the memory
 */
void memory_puts (const char *str, int buf_size)
{
	uint32_t start = tip_log_buffer.log_start_address;
	uint32_t end = start + tip_log_buffer.log_size;
	uint32_t next = tip_log_buffer.log_next_address;

	/* How many bytes from next to end of buffer */
	uint32_t space_to_end = end - next;
	/* when logging to DDR (addresses below 0x8000000), making sure that
	the DDR has been initialized and is ready for access */
	if (!tip_log_buffer.is_ddr_and_read && start < (SDRAM_BASE_ADDR + SDRAM_MAPPED_SIZE)) {
		if (CHIP_CfgWasDone () == FALSE)
			return;
		else
			tip_log_buffer.is_ddr_and_read = true; /* so future calls will skip this check*/
	}
    char str_no_esc[SERIAL_PRINTF_BUFFER_SIZE];
    memcpy(str_no_esc, str, buf_size);
	buf_size = remove_esc_sequences (str_no_esc, buf_size);
	if (buf_size <= space_to_end) {
		/* All fits before wrap */
		memcpy ((void *) next, str_no_esc, buf_size);
		tip_log_buffer.log_next_address = next + buf_size;
		/* wrap if exactly at end */
		if (tip_log_buffer.log_next_address == end)
			tip_log_buffer.log_next_address = start;
	} else {
		/* Split: part at end, part at start */
		memcpy ((void *) next, str_no_esc, space_to_end);
		memcpy ((void *) start, str_no_esc + space_to_end, buf_size - space_to_end);
		tip_log_buffer.log_next_address = start + (buf_size - space_to_end);
	}

	/* update the data in the scratchpad */
	REG_WRITE (SCRPAD_10_41 (17), tip_log_buffer.log_start_address);
	REG_WRITE (SCRPAD_10_41 (18), tip_log_buffer.log_size);
	REG_WRITE (SCRPAD_10_41 (19), tip_log_buffer.log_next_address);
}

/**
 * @function    memory_printf_init
 *
 * @param     tip_log_buffer 3 parameteres
 *
 * @return    none
 * @brief
 *       This routine updates the global tip_log_buffer and initializes the buffer with ffs
 */
void memory_printf_init (uint32_t log_start_address, uint32_t log_max_size,
	uint32_t log_next_address)
{
	/* init the tip_log_buffer from the relevant XML */
	memory_printf_udpate (log_start_address, log_max_size, log_next_address);

	/* put 0xffff in the entire area from log_start_address + log_max_size */
	memset ((void *) tip_log_buffer.log_start_address, 0xff, tip_log_buffer.log_size);
}

/**
 * @function    memory_printf_udpate
 *
 * @param     tip_log_buffer 3 parameteres
 *
 * @return    none
 * @brief
 *       This routine updates the global tip_log_buffer
 */
void memory_printf_udpate (uint32_t log_start_address, uint32_t log_max_size,
	uint32_t log_next_address)
{
	tip_log_buffer.log_start_address = log_start_address;
	tip_log_buffer.log_size = log_max_size;
	tip_log_buffer.log_next_address = log_next_address;
	tip_log_buffer.is_ddr_and_read = false; /*  always being initialzed with false */
}

/**
 * @function    serial_printf
 *
 *
 * @param      fmt - format string
 *
 * @return
 * @brief
 *       This routine implements printf to serial or \and to memory
 */
int serial_printf (const char *fmt, ...)
{
	va_list args;
	int i;
	/* check if logging is not allowed. */
	if (is_uart_print_enabled_var && gUartLog == UART_MAX_DEV && !is_memory_print_enabled_var)
		return 0;

	va_start (args, fmt);
	/* Parse format string into buffer */
	i = serial_vsnprintf (buf, sizeof(buf), fmt, args);
	va_end (args);

	if (is_uart_print_enabled_var) {
		serial_puts (buf);
	}

	if (is_memory_print_enabled_var) {
		memory_puts (buf, i);
	}

	return i;
}

void serial_printf_setup (bool is_init)
{
	uint32_t old_uart_baud;
	/* 
	* If STRAP5 is active (low), set MFSEL4 bit BSPASEL (Select BMC debug Serial Port (BSP) 
	* on serial Interface 2), using UART0. Otherwise, log and FUP will be used over TIP UART
	*/
	UART_DEV_T dev = (!READ_REG_FIELD (PWRON, PWRON_BSPA)) ? UART0_DEV : UART_TIP_DEV;
	/* if UART is not enabled for TIP_ROM then don't print */
	if (READ_REG_FIELD (FUSTRAP2, FUSTRAP2_TIP_UART_DIS) == 1) {
		gUartLog = UART_MAX_DEV;
		return;
	}
	gUartLog = dev;
	while (UART_TestTX (gUartLog));
	old_uart_baud = UART_GetBaudrate (gUartLog);
	if (is_init) {
		if (0 == READ_REG_FIELD (FUSTRAP1, FUSTRAP1_oFAST_UART))
		{
			CLK_ConfigureUartClockEx (CLKSEL_UARTCKSEL_CLKREF, 7);
			UART_Init (gUartLog, UART_MODE1_HSP1_SI2____HSP2_UART2__UART1_s_HSP1__UART3_s_SI2, UART_BAUDRATE_115200);
		}
		else /* FAST_UART */
		{
			CLK_ConfigureUartClockEx (CLKSEL_UARTCKSEL_PLL2, 20);
			UART_Init (gUartLog, UART_MODE1_HSP1_SI2____HSP2_UART2__UART1_s_HSP1__UART3_s_SI2, UART_BAUDRATE_750000);
		}

	}
	else { /* reconfig */
		SET_REG_FIELD (CLKDIV3, CLKDIV3_UARTDIV2, READ_REG_FIELD (CLKDIV1, CLKDIV1_UARTDIV1));
		uint32_t setup_baudrate = 0;
		if (0 == READ_REG_FIELD(FUSTRAP1, FUSTRAP1_oFAST_UART))
		{
			setup_baudrate = UART_BAUDRATE_115200;
		}
		else /*FAST_UART*/
		{
			setup_baudrate = UART_BAUDRATE_750000;	
		}
		/* +- 3.5% according to uart spec is enough */
		if (old_uart_baud > (setup_baudrate * 965 / 1000) && old_uart_baud < (setup_baudrate * 1035 / 1000))
		{
			return;
		}
		UART_SetBaudrate (gUartLog, setup_baudrate);
		CHIP_Mux_Uart (gUartLog, true, false, false);
	}
	serial_printf ("\n\nTIP_FW prints to UART%d. Pervious baudrate = %d\n", gUartLog,
			   old_uart_baud);
	serial_printf ("New baudrate = %d\n", UART_GetBaudrate (gUartLog));

}

void serial_printf_init (bool is_uart_print_enabled, bool is_memory_print_enabled)
{
#ifdef TIP_DEBUG_BUILD
	is_uart_print_enabled_var = is_uart_print_enabled;
	is_memory_print_enabled_var = is_memory_print_enabled;

	if (is_uart_print_enabled_var)
		serial_printf_setup(true);
#endif
}

void serial_printf_reconfig (void)
{
#ifdef TIP_DEBUG_BUILD
	serial_printf_setup(false);
#endif
}

int serial_get_char (char *c, bool bWait)
{
	DEFS_STATUS status;

	if (bWait == true) {
		*c = UART_GetC (gUartLog);
		return 0;
	} else {
		status = UART_GetC_NB (gUartLog, c);
		if (status != DEFS_STATUS_OK)
			return -1;
	}
	return 0;
}
