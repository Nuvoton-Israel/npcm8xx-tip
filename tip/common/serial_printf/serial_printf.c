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
#include <stdio.h>
#include <string.h>
#include "platform_io.h"


/* Setting default configurations */
UART_DEV_T gUartLog = UART0_DEV;
#ifndef SERIAL_PRINTF_BUFFER_SIZE
#define SERIAL_PRINTF_BUFFER_SIZE _2KB_
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
	while (*s) {
		if (*s == '\n') {
			UART_PutC (gUartLog, '\r');
		}
		/* Puting the char to serial */
		UART_PutC (gUartLog, *s);
		s++;
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
 *       This routine performs string to interget conversion
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
 * @param      type -
 *
 * @return
 * @brief
 *       This routine used for printf number conversions
 */
static char *number (char *str, long num, unsigned int base, int size, int precision, int type)
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
		while (size-- > 0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	
	/* add 0x prefix */
	if (type & SPECIAL2) {
		if (base == 8)
			*str++ = '0';
		else if (base == 16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}
	if (!(type & LEFT))
		while (size-- > 0)
			*str++ = c;
	while (i < precision--)
		*str++ = '0';
	while (i-- > 0)
		*str++ = tmp[i];
	while (size-- > 0)
		*str++ = ' ';
	return str;
}

/**
 * @function    serial_vsprintf
 *
 *
 * @param      args -
 * @param      buf -
 * @param      fmt -
 *
 * @return
 * @brief
 *       This routine parses format string and dumps the output to buffer
 */
static int serial_vsprintf (char *buf, const char *fmt, const va_list args)
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
	for (str = buf; *fmt; ++fmt) {
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
					while (--field_width > 0)
						*str++ = ' ';
				*str++ = (unsigned char) va_arg (args, int);
				while (--field_width > 0)
					*str++ = ' ';
				continue;
			case 's':
				s = va_arg (args, char *);
				if (!s)
					s = "<NULL>";
				len = serial_strnlen (s, precision);
				if (!(flags & LEFT))
					while (len < field_width--)
						*str++ = ' ';
				for (i = 0; i < len; ++i)
					*str++ = *s++;
				while (len < field_width--)
					*str++ = ' ';
				continue;
			case 'p':
				if (field_width == -1) {
					field_width = 2 * sizeof (void *);
					flags |= ZEROPAD;
				}
				str = number (str, (unsigned long) va_arg (args, void *), 16, field_width,
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
				if (*fmt)
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
		str = number (str, num, base, field_width, precision, flags);
	}
	*str = '\0';
	return str - buf;
}

/**
 * @function    serial_printf
 *
 *
 * @param      fmt - format string
 *
 * @return
 * @brief
 *       This routine implements printf to serial
 */
int serial_printf (const char *fmt, ...)
{
	va_list args;
	int		i;
	/* check if logging is not allowed. */
	if (gUartLog == UART_MAX_DEV)
		return 0;

	va_start (args, fmt);

	/* Parse format string into buffer */
	i = serial_vsprintf (buf, fmt, args);
	va_end (args);

	/* Send the buffer to serial */
	serial_puts (buf);
	return i;
}
void serial_printf_init (void)
{
	uint32_t old_uart_baud;
	/* If STRAP5 is active (low), set MFSEL4 bit BSPASEL (Select BMC debug Serial Port (BSP) on
	Serial Interface 2), using UART0. Otherwise, log and FUP will be used over TIP UART */
	UART_DEV_T dev = !READ_REG_FIELD (PWRON, PWRON_BSPA) ? UART0_DEV : UART_TIP_DEV;
	/* if UART is not enabled for TIP_ROM then don't print */
	if (READ_REG_FIELD (FUSTRAP2, FUSTRAP2_TIP_UART_DIS) == 1) {
		gUartLog = UART_MAX_DEV;
		return;
	}
	gUartLog = dev;

	while (UART_TestTX (gUartLog));
	memset ((void *) buf, 0, SERIAL_PRINTF_BUFFER_SIZE);
	old_uart_baud = UART_GetBaudrate (gUartLog);
	/* UART_ResetFIFOs(gUartLog, TRUE, TRUE); */
	CLK_ConfigureUartClock ();
	UART_Init (gUartLog, UART_SKIP_MUX, UART_BAUDRATE_115200);
	serial_printf ("\n\nTIP_FW prints to UART%d. Pervious baudrate = %d\n", gUartLog,
				   old_uart_baud);
	serial_printf ("New baudrate = %d\n", UART_GetBaudrate (gUartLog));
}

void serial_printf_reconfig (void)
{
	uint32_t old_uart_baud;

	/* If STRAP5 is active (low), set MFSEL4 bit BSPASEL (Select BMC debug Serial Port (BSP) on
	Serial Interface 2), using UART0. Otherwise, log and FUP will be used over TIP UART */
	UART_DEV_T dev = !READ_REG_FIELD (PWRON, PWRON_BSPA) ? UART0_DEV : UART_TIP_DEV;

	/* if UART is not enabled for TIP_ROM then don't print: */
	if (READ_REG_FIELD (FUSTRAP2, FUSTRAP2_TIP_UART_DIS) == 1) {
		gUartLog = UART_MAX_DEV;
		return;
	}
	gUartLog = dev;
	while (UART_TestTX (gUartLog));
	old_uart_baud = UART_GetBaudrate (gUartLog);

	/* 115200 +- 3% according to uart spec is enough */
	if (old_uart_baud > 112900 && old_uart_baud < 118600)
		return;

	SET_REG_FIELD (CLKDIV3, CLKDIV3_UARTDIV2, READ_REG_FIELD (CLKDIV1, CLKDIV1_UARTDIV1));
	UART_SetBaudrate (gUartLog, UART_BAUDRATE_115200);
	CHIP_Mux_Uart (gUartLog, true, false, false);
#if 0  /* def TIP_DEBUG_BUILD  */
	serial_printf (NEWLINE NEWLINE "TIP_FW prints to UART%d. Pervious baudrate = %d", gUartLog,
				   old_uart_baud);
	serial_printf (" New baudrate = %d" NEWLINE, UART_GetBaudrate (gUartLog));
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
