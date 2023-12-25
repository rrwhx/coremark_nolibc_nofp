/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <coremark.h>
#include <stdarg.h>


/* might be ignored by the compiler without -ffreestanding, then found as
 * missing.
 */
__attribute__((weak,unused,section(".text.nolibc_memset")))
void *memset(void *dst, int b, size_t len)
{
	char *p = dst;

	while (len--) {
		/* prevent gcc from recognizing memset() here */
		__asm__ volatile("");
		*(p++) = b;
	}
	return dst;
}

static __attribute__((unused))
void *_nolibc_memcpy_up(void *dst, const void *src, size_t len)
{
	size_t pos = 0;

	while (pos < len) {
		((char *)dst)[pos] = ((const char *)src)[pos];
		pos++;
	}
	return dst;
}

static __attribute__((unused))
void *_nolibc_memcpy_down(void *dst, const void *src, size_t len)
{
	while (len) {
		len--;
		((char *)dst)[len] = ((const char *)src)[len];
	}
	return dst;
}

/* might be ignored by the compiler without -ffreestanding, then found as
 * missing.
 */
__attribute__((weak,unused,section(".text.nolibc_memmove")))
void *memmove(void *dst, const void *src, size_t len)
{
	size_t dir, pos;

	pos = len;
	dir = -1;

	if (dst < src) {
		pos = -1;
		dir = 1;
	}

	while (len) {
		pos += dir;
		((char *)dst)[pos] = ((const char *)src)[pos];
		len--;
	}
	return dst;
}

/* must be exported, as it's used by libgcc on ARM */
__attribute__((weak,unused,section(".text.nolibc_memcpy")))
void *memcpy(void *dst, const void *src, size_t len)
{
	return _nolibc_memcpy_up(dst, src, len);
}

#define ZEROPAD   (1 << 0) /* Pad with zero */
#define SIGN      (1 << 1) /* Unsigned/signed long */
#define PLUS      (1 << 2) /* Show plus */
#define SPACE     (1 << 3) /* Spacer */
#define LEFT      (1 << 4) /* Left justified */
#define HEX_PREP  (1 << 5) /* 0x */
#define UPPERCASE (1 << 6) /* 'ABCDEF' */

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static char *    digits       = "0123456789abcdefghijklmnopqrstuvwxyz";
static char *    upper_digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static ee_size_t strnlen(const char *s, ee_size_t count);

static ee_size_t
strnlen(const char *s, ee_size_t count)
{
    const char *sc;
    for (sc = s; *sc != '\0' && count--; ++sc)
        ;
    return sc - s;
}

static int
skip_atoi(const char **s)
{
    int i = 0;
    while (is_digit(**s))
        i = i * 10 + *((*s)++) - '0';
    return i;
}

static char *
number(char *str, long num, int base, int size, int precision, int type)
{
    char  c, sign, tmp[66];
    char *dig = digits;
    int   i;

    if (type & UPPERCASE)
        dig = upper_digits;
    if (type & LEFT)
        type &= ~ZEROPAD;
    if (base < 2 || base > 36)
        return 0;

    c    = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (type & SIGN)
    {
        if (num < 0)
        {
            sign = '-';
            num  = -num;
            size--;
        }
        else if (type & PLUS)
        {
            sign = '+';
            size--;
        }
        else if (type & SPACE)
        {
            sign = ' ';
            size--;
        }
    }

    if (type & HEX_PREP)
    {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }

    i = 0;

    if (num == 0)
        tmp[i++] = '0';
    else
    {
        while (num != 0)
        {
            tmp[i++] = dig[((unsigned long)num) % (unsigned)base];
            num      = ((unsigned long)num) / (unsigned)base;
        }
    }

    if (i > precision)
        precision = i;
    size -= precision;
    if (!(type & (ZEROPAD | LEFT)))
        while (size-- > 0)
            *str++ = ' ';
    if (sign)
        *str++ = sign;

    if (type & HEX_PREP)
    {
        if (base == 8)
            *str++ = '0';
        else if (base == 16)
        {
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

static char *
eaddr(char *str, unsigned char *addr, int size, int precision, int type)
{
    char  tmp[24];
    char *dig = digits;
    int   i, len;

    if (type & UPPERCASE)
        dig = upper_digits;
    len = 0;
    for (i = 0; i < 6; i++)
    {
        if (i != 0)
            tmp[len++] = ':';
        tmp[len++] = dig[addr[i] >> 4];
        tmp[len++] = dig[addr[i] & 0x0F];
    }

    if (!(type & LEFT))
        while (len < size--)
            *str++ = ' ';
    for (i = 0; i < len; ++i)
        *str++ = tmp[i];
    while (len < size--)
        *str++ = ' ';

    return str;
}

static char *
iaddr(char *str, unsigned char *addr, int size, int precision, int type)
{
    char tmp[24];
    int  i, n, len;

    len = 0;
    for (i = 0; i < 4; i++)
    {
        if (i != 0)
            tmp[len++] = '.';
        n = addr[i];

        if (n == 0)
            tmp[len++] = digits[0];
        else
        {
            if (n >= 100)
            {
                tmp[len++] = digits[n / 100];
                n          = n % 100;
                tmp[len++] = digits[n / 10];
                n          = n % 10;
            }
            else if (n >= 10)
            {
                tmp[len++] = digits[n / 10];
                n          = n % 10;
            }

            tmp[len++] = digits[n];
        }
    }

    if (!(type & LEFT))
        while (len < size--)
            *str++ = ' ';
    for (i = 0; i < len; ++i)
        *str++ = tmp[i];
    while (len < size--)
        *str++ = ' ';

    return str;
}

#if HAS_FLOAT

char *      ecvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);
char *      fcvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);
static void ee_bufcpy(char *d, char *s, int count);

void
ee_bufcpy(char *pd, char *ps, int count)
{
    char *pe = ps + count;
    while (ps != pe)
        *pd++ = *ps++;
}

static void
parse_float(double value, char *buffer, char fmt, int precision)
{
    int   decpt, sign, exp, pos;
    char *digits = NULL;
    char  cvtbuf[80];
    int   capexp = 0;
    int   magnitude;

    if (fmt == 'G' || fmt == 'E')
    {
        capexp = 1;
        fmt += 'a' - 'A';
    }

    if (fmt == 'g')
    {
        digits    = ecvtbuf(value, precision, &decpt, &sign, cvtbuf);
        magnitude = decpt - 1;
        if (magnitude < -4 || magnitude > precision - 1)
        {
            fmt = 'e';
            precision -= 1;
        }
        else
        {
            fmt = 'f';
            precision -= decpt;
        }
    }

    if (fmt == 'e')
    {
        digits = ecvtbuf(value, precision + 1, &decpt, &sign, cvtbuf);

        if (sign)
            *buffer++ = '-';
        *buffer++ = *digits;
        if (precision > 0)
            *buffer++ = '.';
        ee_bufcpy(buffer, digits + 1, precision);
        buffer += precision;
        *buffer++ = capexp ? 'E' : 'e';

        if (decpt == 0)
        {
            if (value == 0.0)
                exp = 0;
            else
                exp = -1;
        }
        else
            exp = decpt - 1;

        if (exp < 0)
        {
            *buffer++ = '-';
            exp       = -exp;
        }
        else
            *buffer++ = '+';

        buffer[2] = (exp % 10) + '0';
        exp       = exp / 10;
        buffer[1] = (exp % 10) + '0';
        exp       = exp / 10;
        buffer[0] = (exp % 10) + '0';
        buffer += 3;
    }
    else if (fmt == 'f')
    {
        digits = fcvtbuf(value, precision, &decpt, &sign, cvtbuf);
        if (sign)
            *buffer++ = '-';
        if (*digits)
        {
            if (decpt <= 0)
            {
                *buffer++ = '0';
                *buffer++ = '.';
                for (pos = 0; pos < -decpt; pos++)
                    *buffer++ = '0';
                while (*digits)
                    *buffer++ = *digits++;
            }
            else
            {
                pos = 0;
                while (*digits)
                {
                    if (pos++ == decpt)
                        *buffer++ = '.';
                    *buffer++ = *digits++;
                }
            }
        }
        else
        {
            *buffer++ = '0';
            if (precision > 0)
            {
                *buffer++ = '.';
                for (pos = 0; pos < precision; pos++)
                    *buffer++ = '0';
            }
        }
    }

    *buffer = '\0';
}

static void
decimal_point(char *buffer)
{
    while (*buffer)
    {
        if (*buffer == '.')
            return;
        if (*buffer == 'e' || *buffer == 'E')
            break;
        buffer++;
    }

    if (*buffer)
    {
        int n = strnlen(buffer, 256);
        while (n > 0)
        {
            buffer[n + 1] = buffer[n];
            n--;
        }

        *buffer = '.';
    }
    else
    {
        *buffer++ = '.';
        *buffer   = '\0';
    }
}

static void
cropzeros(char *buffer)
{
    char *stop;

    while (*buffer && *buffer != '.')
        buffer++;
    if (*buffer++)
    {
        while (*buffer && *buffer != 'e' && *buffer != 'E')
            buffer++;
        stop = buffer--;
        while (*buffer == '0')
            buffer--;
        if (*buffer == '.')
            buffer--;
        while (buffer != stop)
            *++buffer = 0;
    }
}

static char *
flt(char *str, double num, int size, int precision, char fmt, int flags)
{
    char tmp[80];
    char c, sign;
    int  n, i;

    // Left align means no zero padding
    if (flags & LEFT)
        flags &= ~ZEROPAD;

    // Determine padding and sign char
    c    = (flags & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (flags & SIGN)
    {
        if (num < 0.0)
        {
            sign = '-';
            num  = -num;
            size--;
        }
        else if (flags & PLUS)
        {
            sign = '+';
            size--;
        }
        else if (flags & SPACE)
        {
            sign = ' ';
            size--;
        }
    }

    // Compute the precision value
    if (precision < 0)
        precision = 6; // Default precision: 6

    // Convert floating point number to text
    parse_float(num, tmp, fmt, precision);

    if ((flags & HEX_PREP) && precision == 0)
        decimal_point(tmp);
    if (fmt == 'g' && !(flags & HEX_PREP))
        cropzeros(tmp);

    n = strnlen(tmp, 256);

    // Output number with alignment and padding
    size -= n;
    if (!(flags & (ZEROPAD | LEFT)))
        while (size-- > 0)
            *str++ = ' ';
    if (sign)
        *str++ = sign;
    if (!(flags & LEFT))
        while (size-- > 0)
            *str++ = c;
    for (i = 0; i < n; i++)
        *str++ = tmp[i];
    while (size-- > 0)
        *str++ = ' ';

    return str;
}

#endif

static int
ee_vsprintf(char *buf, const char *fmt, va_list args)
{
    int           len;
    unsigned long num;
    int           i, base;
    char *        str;
    char *        s;

    int flags; // Flags to number()

    int field_width; // Width of output field
    int precision;   // Min. # of digits for integers; max number of chars for
                     // from string
    int qualifier;   // 'h', 'l', or 'L' for integer fields

    for (str = buf; *fmt; fmt++)
    {
        if (*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }

        // Process flags
        flags = 0;
    repeat:
        fmt++; // This also skips first '%'
        switch (*fmt)
        {
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
                flags |= HEX_PREP;
                goto repeat;
            case '0':
                flags |= ZEROPAD;
                goto repeat;
        }

        // Get field width
        field_width = -1;
        if (is_digit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*')
        {
            fmt++;
            field_width = va_arg(args, int);
            if (field_width < 0)
            {
                field_width = -field_width;
                flags |= LEFT;
            }
        }

        // Get the precision
        precision = -1;
        if (*fmt == '.')
        {
            ++fmt;
            if (is_digit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*')
            {
                ++fmt;
                precision = va_arg(args, int);
            }
            if (precision < 0)
                precision = 0;
        }

        // Get the conversion qualifier
        qualifier = -1;
        if (*fmt == 'l' || *fmt == 'L')
        {
            qualifier = *fmt;
            fmt++;
        }

        // Default base
        base = 10;

        switch (*fmt)
        {
            case 'c':
                if (!(flags & LEFT))
                    while (--field_width > 0)
                        *str++ = ' ';
                *str++ = (unsigned char)va_arg(args, int);
                while (--field_width > 0)
                    *str++ = ' ';
                continue;

            case 's':
                s = va_arg(args, char *);
                if (!s)
                    s = "<NULL>";
                len = strnlen(s, precision);
                if (!(flags & LEFT))
                    while (len < field_width--)
                        *str++ = ' ';
                for (i = 0; i < len; ++i)
                    *str++ = *s++;
                while (len < field_width--)
                    *str++ = ' ';
                continue;

            case 'p':
                if (field_width == -1)
                {
                    field_width = 2 * sizeof(void *);
                    flags |= ZEROPAD;
                }
                str = number(str,
                             (unsigned long)va_arg(args, void *),
                             16,
                             field_width,
                             precision,
                             flags);
                continue;

            case 'A':
                flags |= UPPERCASE;

            case 'a':
                if (qualifier == 'l')
                    str = eaddr(str,
                                va_arg(args, unsigned char *),
                                field_width,
                                precision,
                                flags);
                else
                    str = iaddr(str,
                                va_arg(args, unsigned char *),
                                field_width,
                                precision,
                                flags);
                continue;

            // Integer number formats - set up the flags and "break"
            case 'o':
                base = 8;
                break;

            case 'X':
                flags |= UPPERCASE;

            case 'x':
                base = 16;
                break;

            case 'd':
            case 'i':
                flags |= SIGN;

            case 'u':
                break;

#if HAS_FLOAT

            case 'f':
                str = flt(str,
                          va_arg(args, double),
                          field_width,
                          precision,
                          *fmt,
                          flags | SIGN);
                continue;

#endif

            default:
                if (*fmt != '%')
                    *str++ = '%';
                if (*fmt)
                    *str++ = *fmt;
                else
                    --fmt;
                continue;
        }

        if (qualifier == 'l')
            num = va_arg(args, unsigned long);
        else if (flags & SIGN)
            num = va_arg(args, int);
        else
            num = va_arg(args, unsigned int);

        str = number(str, num, base, field_width, precision, flags);
    }

    *str = '\0';
    return str - buf;
}

void
uart_send_char(char c)
{
// #warning "You must implement the method uart_send_char to use this file!\n";
    /*	Output of a char to a UART usually follows the following model:
            Wait until UART is ready
            Write char to UART
            Wait until UART is done

            Or in code:
            while (*UART_CONTROL_ADDRESS != UART_READY);
            *UART_DATA_ADDRESS = c;
            while (*UART_CONTROL_ADDRESS != UART_READY);

            Check the UART sample code on your platform or the board
       documentation.
    */
}

#include <asm/unistd.h>

#if defined(__x86_64__)

#define my_syscall0(num)                                                      \
({                                                                            \
	long _ret;                                                            \
	register long _num  __asm__ ("rax") = (num);                          \
									      \
	__asm__ volatile (                                                    \
		"syscall\n"                                                   \
		: "=a"(_ret)                                                  \
		: "0"(_num)                                                   \
		: "rcx", "r11", "memory", "cc"                                \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall1(num, arg1)                                                \
({                                                                            \
	long _ret;                                                            \
	register long _num  __asm__ ("rax") = (num);                          \
	register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
									      \
	__asm__ volatile (                                                    \
		"syscall\n"                                                   \
		: "=a"(_ret)                                                  \
		: "r"(_arg1),                                                 \
		  "0"(_num)                                                   \
		: "rcx", "r11", "memory", "cc"                                \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall2(num, arg1, arg2)                                          \
({                                                                            \
	long _ret;                                                            \
	register long _num  __asm__ ("rax") = (num);                          \
	register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
	register long _arg2 __asm__ ("rsi") = (long)(arg2);                   \
									      \
	__asm__ volatile (                                                    \
		"syscall\n"                                                   \
		: "=a"(_ret)                                                  \
		: "r"(_arg1), "r"(_arg2),                                     \
		  "0"(_num)                                                   \
		: "rcx", "r11", "memory", "cc"                                \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall3(num, arg1, arg2, arg3)                                    \
({                                                                            \
	long _ret;                                                            \
	register long _num  __asm__ ("rax") = (num);                          \
	register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
	register long _arg2 __asm__ ("rsi") = (long)(arg2);                   \
	register long _arg3 __asm__ ("rdx") = (long)(arg3);                   \
									      \
	__asm__ volatile (                                                    \
		"syscall\n"                                                   \
		: "=a"(_ret)                                                  \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3),                         \
		  "0"(_num)                                                   \
		: "rcx", "r11", "memory", "cc"                                \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall4(num, arg1, arg2, arg3, arg4)                              \
({                                                                            \
	long _ret;                                                            \
	register long _num  __asm__ ("rax") = (num);                          \
	register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
	register long _arg2 __asm__ ("rsi") = (long)(arg2);                   \
	register long _arg3 __asm__ ("rdx") = (long)(arg3);                   \
	register long _arg4 __asm__ ("r10") = (long)(arg4);                   \
									      \
	__asm__ volatile (                                                    \
		"syscall\n"                                                   \
		: "=a"(_ret)                                                  \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4),             \
		  "0"(_num)                                                   \
		: "rcx", "r11", "memory", "cc"                                \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall5(num, arg1, arg2, arg3, arg4, arg5)                        \
({                                                                            \
	long _ret;                                                            \
	register long _num  __asm__ ("rax") = (num);                          \
	register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
	register long _arg2 __asm__ ("rsi") = (long)(arg2);                   \
	register long _arg3 __asm__ ("rdx") = (long)(arg3);                   \
	register long _arg4 __asm__ ("r10") = (long)(arg4);                   \
	register long _arg5 __asm__ ("r8")  = (long)(arg5);                   \
									      \
	__asm__ volatile (                                                    \
		"syscall\n"                                                   \
		: "=a"(_ret)                                                  \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
		  "0"(_num)                                                   \
		: "rcx", "r11", "memory", "cc"                                \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall6(num, arg1, arg2, arg3, arg4, arg5, arg6)                  \
({                                                                            \
	long _ret;                                                            \
	register long _num  __asm__ ("rax") = (num);                          \
	register long _arg1 __asm__ ("rdi") = (long)(arg1);                   \
	register long _arg2 __asm__ ("rsi") = (long)(arg2);                   \
	register long _arg3 __asm__ ("rdx") = (long)(arg3);                   \
	register long _arg4 __asm__ ("r10") = (long)(arg4);                   \
	register long _arg5 __asm__ ("r8")  = (long)(arg5);                   \
	register long _arg6 __asm__ ("r9")  = (long)(arg6);                   \
									      \
	__asm__ volatile (                                                    \
		"syscall\n"                                                   \
		: "=a"(_ret)                                                  \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
		  "r"(_arg6), "0"(_num)                                       \
		: "rcx", "r11", "memory", "cc"                                \
	);                                                                    \
	_ret;                                                                 \
})

/* startup code */
/*
 * x86-64 System V ABI mandates:
 * 1) %rsp must be 16-byte aligned right before the function call.
 * 2) The deepest stack frame should be zero (the %rbp).
 *
 */
void __attribute__((weak, noreturn, optimize("Os", "omit-frame-pointer"))) __attribute__((no_stack_protector)) _start(void)
{
#ifdef NO_STACK
	__asm__ volatile (
		"lea  __sstack + 0x8000 - 16, %rsp\n"       /* save stack pointer to %rdi, as arg1 of _start_c */
		"and  $-16, %rsp\n"       /* %rsp must be 16-byte aligned before call        */
		"xor  %ebp, %ebp\n"       /* zero the stack frame                            */
		"call main\n"         /* transfer to c runtime                           */
        "mov %eax, %edi\n"
        "mov $60, %rax\n"
        "syscall\n"
		"hlt\n"                   /* ensure it does not return                       */
	);
#else
	__asm__ volatile (
		"xor  %ebp, %ebp\n"       /* zero the stack frame                            */
		"mov  %rsp, %rdi\n"       /* save stack pointer to %rdi, as arg1 of _start_c */
		"and  $-16, %rsp\n"       /* %rsp must be 16-byte aligned before call        */
		"call _start_c\n"         /* transfer to c runtime                           */
		"hlt\n"                   /* ensure it does not return                       */
	);
#endif
	__builtin_unreachable();
}

#elif defined(__i386__)
/* Syscalls for i386 :
 *   - mostly similar to x86_64
 *   - registers are 32-bit
 *   - syscall number is passed in eax
 *   - arguments are in ebx, ecx, edx, esi, edi, ebp respectively
 *   - all registers are preserved (except eax of course)
 *   - the system call is performed by calling int $0x80
 *   - syscall return comes in eax
 *   - the arguments are cast to long and assigned into the target registers
 *     which are then simply passed as registers to the asm code, so that we
 *     don't have to experience issues with register constraints.
 *   - the syscall number is always specified last in order to allow to force
 *     some registers before (gcc refuses a %-register at the last position).
 *
 * Also, i386 supports the old_select syscall if newselect is not available
 */

#define my_syscall0(num)                                                      \
({                                                                            \
	long _ret;                                                            \
	register long _num __asm__ ("eax") = (num);                           \
									      \
	__asm__ volatile (                                                    \
		"int $0x80\n"                                                 \
		: "=a" (_ret)                                                 \
		: "0"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall1(num, arg1)                                                \
({                                                                            \
	long _ret;                                                            \
	register long _num __asm__ ("eax") = (num);                           \
	register long _arg1 __asm__ ("ebx") = (long)(arg1);                   \
									      \
	__asm__ volatile (                                                    \
		"int $0x80\n"                                                 \
		: "=a" (_ret)                                                 \
		: "r"(_arg1),                                                 \
		  "0"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall2(num, arg1, arg2)                                          \
({                                                                            \
	long _ret;                                                            \
	register long _num __asm__ ("eax") = (num);                           \
	register long _arg1 __asm__ ("ebx") = (long)(arg1);                   \
	register long _arg2 __asm__ ("ecx") = (long)(arg2);                   \
									      \
	__asm__ volatile (                                                    \
		"int $0x80\n"                                                 \
		: "=a" (_ret)                                                 \
		: "r"(_arg1), "r"(_arg2),                                     \
		  "0"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall3(num, arg1, arg2, arg3)                                    \
({                                                                            \
	long _ret;                                                            \
	register long _num __asm__ ("eax") = (num);                           \
	register long _arg1 __asm__ ("ebx") = (long)(arg1);                   \
	register long _arg2 __asm__ ("ecx") = (long)(arg2);                   \
	register long _arg3 __asm__ ("edx") = (long)(arg3);                   \
									      \
	__asm__ volatile (                                                    \
		"int $0x80\n"                                                 \
		: "=a" (_ret)                                                 \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3),                         \
		  "0"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall4(num, arg1, arg2, arg3, arg4)                              \
({                                                                            \
	long _ret;                                                            \
	register long _num __asm__ ("eax") = (num);                           \
	register long _arg1 __asm__ ("ebx") = (long)(arg1);                   \
	register long _arg2 __asm__ ("ecx") = (long)(arg2);                   \
	register long _arg3 __asm__ ("edx") = (long)(arg3);                   \
	register long _arg4 __asm__ ("esi") = (long)(arg4);                   \
									      \
	__asm__ volatile (                                                    \
		"int $0x80\n"                                                 \
		: "=a" (_ret)                                                 \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4),             \
		  "0"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall5(num, arg1, arg2, arg3, arg4, arg5)                        \
({                                                                            \
	long _ret;                                                            \
	register long _num __asm__ ("eax") = (num);                           \
	register long _arg1 __asm__ ("ebx") = (long)(arg1);                   \
	register long _arg2 __asm__ ("ecx") = (long)(arg2);                   \
	register long _arg3 __asm__ ("edx") = (long)(arg3);                   \
	register long _arg4 __asm__ ("esi") = (long)(arg4);                   \
	register long _arg5 __asm__ ("edi") = (long)(arg5);                   \
									      \
	__asm__ volatile (                                                    \
		"int $0x80\n"                                                 \
		: "=a" (_ret)                                                 \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
		  "0"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_ret;                                                                 \
})

#define my_syscall6(num, arg1, arg2, arg3, arg4, arg5, arg6)	\
({								\
	long _eax  = (long)(num);				\
	long _arg6 = (long)(arg6); /* Always in memory */	\
	__asm__ volatile (					\
		"pushl	%[_arg6]\n\t"				\
		"pushl	%%ebp\n\t"				\
		"movl	4(%%esp),%%ebp\n\t"			\
		"int	$0x80\n\t"				\
		"popl	%%ebp\n\t"				\
		"addl	$4,%%esp\n\t"				\
		: "+a"(_eax)		/* %eax */		\
		: "b"(arg1),		/* %ebx */		\
		  "c"(arg2),		/* %ecx */		\
		  "d"(arg3),		/* %edx */		\
		  "S"(arg4),		/* %esi */		\
		  "D"(arg5),		/* %edi */		\
		  [_arg6]"m"(_arg6)	/* memory */		\
		: "memory", "cc"				\
	);							\
	_eax;							\
})

/* startup code */
/*
 * i386 System V ABI mandates:
 * 1) last pushed argument must be 16-byte aligned.
 * 2) The deepest stack frame should be set to zero
 *
 */
void __attribute__((weak, noreturn, optimize("Os", "omit-frame-pointer"))) __attribute__((no_stack_protector)) _start(void)
{
	__asm__ volatile (
		"xor  %ebp, %ebp\n"       /* zero the stack frame                                */
		"mov  %esp, %eax\n"       /* save stack pointer to %eax, as arg1 of _start_c     */
		"add  $12, %esp\n"        /* avoid over-estimating after the 'and' & 'sub' below */
		"and  $-16, %esp\n"       /* the %esp must be 16-byte aligned on 'call'          */
		"sub  $12, %esp\n"        /* sub 12 to keep it aligned after the push %eax       */
		"push %eax\n"             /* push arg1 on stack to support plain stack modes too */
		"call _start_c\n"         /* transfer to c runtime                               */
		"hlt\n"                   /* ensure it does not return                           */
	);
	__builtin_unreachable();
}

#elif defined(__loongarch__)
/* Syscalls for LoongArch :
 *   - stack is 16-byte aligned
 *   - syscall number is passed in a7
 *   - arguments are in a0, a1, a2, a3, a4, a5
 *   - the system call is performed by calling "syscall 0"
 *   - syscall return comes in a0
 *   - the arguments are cast to long and assigned into the target
 *     registers which are then simply passed as registers to the asm code,
 *     so that we don't have to experience issues with register constraints.
 *
 * On LoongArch, select() is not implemented so we have to use pselect6().
 */
#define __ARCH_WANT_SYS_PSELECT6
#define _NOLIBC_SYSCALL_CLOBBERLIST \
	"memory", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8"

#define my_syscall0(num)                                                      \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0");                                   \
									      \
	__asm__ volatile (                                                    \
		"syscall 0\n"                                                 \
		: "=r"(_arg1)                                                 \
		: "r"(_num)                                                   \
		: _NOLIBC_SYSCALL_CLOBBERLIST                                 \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall1(num, arg1)                                                \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);		      \
									      \
	__asm__ volatile (                                                    \
		"syscall 0\n"                                                 \
		: "+r"(_arg1)                                                 \
		: "r"(_num)                                                   \
		: _NOLIBC_SYSCALL_CLOBBERLIST                                 \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall2(num, arg1, arg2)                                          \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
									      \
	__asm__ volatile (                                                    \
		"syscall 0\n"                                                 \
		: "+r"(_arg1)                                                 \
		: "r"(_arg2),                                                 \
		  "r"(_num)                                                   \
		: _NOLIBC_SYSCALL_CLOBBERLIST                                 \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall3(num, arg1, arg2, arg3)                                    \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
									      \
	__asm__ volatile (                                                    \
		"syscall 0\n"                                                 \
		: "+r"(_arg1)                                                 \
		: "r"(_arg2), "r"(_arg3),                                     \
		  "r"(_num)                                                   \
		: _NOLIBC_SYSCALL_CLOBBERLIST                                 \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall4(num, arg1, arg2, arg3, arg4)                              \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
	register long _arg4 __asm__ ("a3") = (long)(arg4);                    \
									      \
	__asm__ volatile (                                                    \
		"syscall 0\n"                                                 \
		: "+r"(_arg1)                                                 \
		: "r"(_arg2), "r"(_arg3), "r"(_arg4),                         \
		  "r"(_num)                                                   \
		: _NOLIBC_SYSCALL_CLOBBERLIST                                 \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall5(num, arg1, arg2, arg3, arg4, arg5)                        \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
	register long _arg4 __asm__ ("a3") = (long)(arg4);                    \
	register long _arg5 __asm__ ("a4") = (long)(arg5);                    \
									      \
	__asm__ volatile (                                                    \
		"syscall 0\n"                                                 \
		: "+r"(_arg1)                                                 \
		: "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5),             \
		  "r"(_num)                                                   \
		: _NOLIBC_SYSCALL_CLOBBERLIST                                 \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall6(num, arg1, arg2, arg3, arg4, arg5, arg6)                  \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
	register long _arg4 __asm__ ("a3") = (long)(arg4);                    \
	register long _arg5 __asm__ ("a4") = (long)(arg5);                    \
	register long _arg6 __asm__ ("a5") = (long)(arg6);                    \
									      \
	__asm__ volatile (                                                    \
		"syscall 0\n"                                                 \
		: "+r"(_arg1)                                                 \
		: "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), "r"(_arg6), \
		  "r"(_num)                                                   \
		: _NOLIBC_SYSCALL_CLOBBERLIST                                 \
	);                                                                    \
	_arg1;                                                                \
})

#if __loongarch_grlen == 32
#define LONG_BSTRINS "bstrins.w"
#else /* __loongarch_grlen == 64 */
#define LONG_BSTRINS "bstrins.d"
#endif

/* startup code */
void __attribute__((weak, noreturn, optimize("Os", "omit-frame-pointer"))) __attribute__((no_stack_protector)) _start(void)
{
	__asm__ volatile (
		"move          $a0, $sp\n"         /* save stack pointer to $a0, as arg1 of _start_c */
		LONG_BSTRINS " $sp, $zero, 3, 0\n" /* $sp must be 16-byte aligned                    */
		"bl            _start_c\n"         /* transfer to c runtime                          */
	);
	__builtin_unreachable();
}

#elif defined(__riscv)
/* Syscalls for RISCV :
 *   - stack is 16-byte aligned
 *   - syscall number is passed in a7
 *   - arguments are in a0, a1, a2, a3, a4, a5
 *   - the system call is performed by calling ecall
 *   - syscall return comes in a0
 *   - the arguments are cast to long and assigned into the target
 *     registers which are then simply passed as registers to the asm code,
 *     so that we don't have to experience issues with register constraints.
 *
 * On riscv, select() is not implemented so we have to use pselect6().
 */
#define __ARCH_WANT_SYS_PSELECT6

#define my_syscall0(num)                                                      \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0");                                   \
									      \
	__asm__ volatile (                                                    \
		"ecall\n\t"                                                   \
		: "=r"(_arg1)                                                 \
		: "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall1(num, arg1)                                                \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);		      \
									      \
	__asm__ volatile (                                                    \
		"ecall\n"                                                     \
		: "+r"(_arg1)                                                 \
		: "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall2(num, arg1, arg2)                                          \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
									      \
	__asm__ volatile (                                                    \
		"ecall\n"                                                     \
		: "+r"(_arg1)                                                 \
		: "r"(_arg2),                                                 \
		  "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall3(num, arg1, arg2, arg3)                                    \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
									      \
	__asm__ volatile (                                                    \
		"ecall\n\t"                                                   \
		: "+r"(_arg1)                                                 \
		: "r"(_arg2), "r"(_arg3),                                     \
		  "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall4(num, arg1, arg2, arg3, arg4)                              \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
	register long _arg4 __asm__ ("a3") = (long)(arg4);                    \
									      \
	__asm__ volatile (                                                    \
		"ecall\n"                                                     \
		: "+r"(_arg1)                                                 \
		: "r"(_arg2), "r"(_arg3), "r"(_arg4),                         \
		  "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall5(num, arg1, arg2, arg3, arg4, arg5)                        \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
	register long _arg4 __asm__ ("a3") = (long)(arg4);                    \
	register long _arg5 __asm__ ("a4") = (long)(arg5);                    \
									      \
	__asm__ volatile (                                                    \
		"ecall\n"                                                     \
		: "+r"(_arg1)                                                 \
		: "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5),             \
		  "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall6(num, arg1, arg2, arg3, arg4, arg5, arg6)                  \
({                                                                            \
	register long _num  __asm__ ("a7") = (num);                           \
	register long _arg1 __asm__ ("a0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("a1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("a2") = (long)(arg3);                    \
	register long _arg4 __asm__ ("a3") = (long)(arg4);                    \
	register long _arg5 __asm__ ("a4") = (long)(arg5);                    \
	register long _arg6 __asm__ ("a5") = (long)(arg6);                    \
									      \
	__asm__ volatile (                                                    \
		"ecall\n"                                                     \
		: "+r"(_arg1)                                                 \
		: "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), "r"(_arg6), \
		  "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

/* startup code */
void __attribute__((weak, noreturn, optimize("Os", "omit-frame-pointer"))) __attribute__((no_stack_protector)) _start(void)
{
	__asm__ volatile (
		".option push\n"
		".option norelax\n"
		"lla  gp, __global_pointer$\n"
		".option pop\n"
		"mv   a0, sp\n"           /* save stack pointer to a0, as arg1 of _start_c */
		"andi sp, a0, -16\n"      /* sp must be 16-byte aligned                    */
		"call _start_c\n"         /* transfer to c runtime                         */
	);
	__builtin_unreachable();
}
#elif defined(__aarch64__)
/* Syscalls for AARCH64 :
 *   - registers are 64-bit
 *   - stack is 16-byte aligned
 *   - syscall number is passed in x8
 *   - arguments are in x0, x1, x2, x3, x4, x5
 *   - the system call is performed by calling svc 0
 *   - syscall return comes in x0.
 *   - the arguments are cast to long and assigned into the target registers
 *     which are then simply passed as registers to the asm code, so that we
 *     don't have to experience issues with register constraints.
 *
 * On aarch64, select() is not implemented so we have to use pselect6().
 */
#define __ARCH_WANT_SYS_PSELECT6

#define my_syscall0(num)                                                      \
({                                                                            \
	register long _num  __asm__ ("x8") = (num);                           \
	register long _arg1 __asm__ ("x0");                                   \
									      \
	__asm__ volatile (                                                    \
		"svc #0\n"                                                    \
		: "=r"(_arg1)                                                 \
		: "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall1(num, arg1)                                                \
({                                                                            \
	register long _num  __asm__ ("x8") = (num);                           \
	register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
									      \
	__asm__ volatile (                                                    \
		"svc #0\n"                                                    \
		: "=r"(_arg1)                                                 \
		: "r"(_arg1),                                                 \
		  "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall2(num, arg1, arg2)                                          \
({                                                                            \
	register long _num  __asm__ ("x8") = (num);                           \
	register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("x1") = (long)(arg2);                    \
									      \
	__asm__ volatile (                                                    \
		"svc #0\n"                                                    \
		: "=r"(_arg1)                                                 \
		: "r"(_arg1), "r"(_arg2),                                     \
		  "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall3(num, arg1, arg2, arg3)                                    \
({                                                                            \
	register long _num  __asm__ ("x8") = (num);                           \
	register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("x1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("x2") = (long)(arg3);                    \
									      \
	__asm__ volatile (                                                    \
		"svc #0\n"                                                    \
		: "=r"(_arg1)                                                 \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3),                         \
		  "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall4(num, arg1, arg2, arg3, arg4)                              \
({                                                                            \
	register long _num  __asm__ ("x8") = (num);                           \
	register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("x1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("x2") = (long)(arg3);                    \
	register long _arg4 __asm__ ("x3") = (long)(arg4);                    \
									      \
	__asm__ volatile (                                                    \
		"svc #0\n"                                                    \
		: "=r"(_arg1)                                                 \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4),             \
		  "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall5(num, arg1, arg2, arg3, arg4, arg5)                        \
({                                                                            \
	register long _num  __asm__ ("x8") = (num);                           \
	register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("x1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("x2") = (long)(arg3);                    \
	register long _arg4 __asm__ ("x3") = (long)(arg4);                    \
	register long _arg5 __asm__ ("x4") = (long)(arg5);                    \
									      \
	__asm__ volatile (                                                    \
		"svc #0\n"                                                    \
		: "=r" (_arg1)                                                \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
		  "r"(_num)                                                   \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

#define my_syscall6(num, arg1, arg2, arg3, arg4, arg5, arg6)                  \
({                                                                            \
	register long _num  __asm__ ("x8") = (num);                           \
	register long _arg1 __asm__ ("x0") = (long)(arg1);                    \
	register long _arg2 __asm__ ("x1") = (long)(arg2);                    \
	register long _arg3 __asm__ ("x2") = (long)(arg3);                    \
	register long _arg4 __asm__ ("x3") = (long)(arg4);                    \
	register long _arg5 __asm__ ("x4") = (long)(arg5);                    \
	register long _arg6 __asm__ ("x5") = (long)(arg6);                    \
									      \
	__asm__ volatile (                                                    \
		"svc #0\n"                                                    \
		: "=r" (_arg1)                                                \
		: "r"(_arg1), "r"(_arg2), "r"(_arg3), "r"(_arg4), "r"(_arg5), \
		  "r"(_arg6), "r"(_num)                                       \
		: "memory", "cc"                                              \
	);                                                                    \
	_arg1;                                                                \
})

/* startup code */
void __attribute__((weak, noreturn, optimize("Os", "omit-frame-pointer"))) __attribute__((no_stack_protector)) _start(void)
{
	__asm__ volatile (
		"mov x0, sp\n"          /* save stack pointer to x0, as arg1 of _start_c */
		"and sp, x0, -16\n"     /* sp must be 16-byte aligned in the callee      */
		"bl  _start_c\n"        /* transfer to c runtime                         */
	);
	__builtin_unreachable();
}
#else

#error "not supported architecture"

#endif
#ifdef NO_STACK
	__asm__ (
        ".data \n"
        ".balign 16 \n"
        "__sstack: \n"
        ".zero 0x8000 \n"
    );
#endif
static inline int write(int fd, const char * buf, int len)
{
  int status = my_syscall3(__NR_write, fd, buf, len);
  return status;
}

#ifdef DEBUGCON
static inline void xxyy_debugcon_out_char(char c) {
	*(volatile char*)(0x800000001fe002e0ull) = c;
}

static inline void xxyy_debugcon_out_str(const char * str) {
	while (*str) {
		xxyy_debugcon_out_char(*str++);
	}
}

static inline void xxyy_debugcon_out_str_n(const char* str, int n){
	register int i = 0;
	for (; i < n; i++) {
		xxyy_debugcon_out_char(str[i]);
	}
}

#define XXYY_DUMP_CURRENT \
	{xxyy_debugcon_out_str("xxyy:" __FILE__ ":" __stringify(__LINE__) ":");xxyy_debugcon_out_str((char*)__func__);xxyy_debugcon_out_str("\n");}
#endif

int
ee_printf(const char *fmt, ...)
{
    char    buf[1024];
    va_list args;
    int     n = 0;

    va_start(args, fmt);
    n = ee_vsprintf(buf, fmt, args);
    va_end(args);
#ifdef DEBUGCON
    xxyy_debugcon_out_str_n(buf, n);
#else
    write(1, buf, n);
#endif

    return n;
}

char **environ __attribute__((weak));
const unsigned long *_auxv __attribute__((weak));

static void __stack_chk_init(void) {}

/*
 * void exit(int status);
 */

static __attribute__((noreturn,unused))
void sys_exit(int status)
{
	my_syscall1(__NR_exit, status & 255);
	while(1); /* shut the "noreturn" warnings. */
}

__attribute__((noreturn,unused))
void exit(int status)
{
	sys_exit(status);
}

void _start_c(long *sp)
{
	long argc;
	char **argv;
	char **envp;
	const unsigned long *auxv;
	/* silence potential warning: conflicting types for 'main' */
	int _nolibc_main(int, char **, char **) __asm__ ("main");

	/* initialize stack protector */
	__stack_chk_init();

	/*
	 * sp  :    argc          <-- argument count, required by main()
	 * argv:    argv[0]       <-- argument vector, required by main()
	 *          argv[1]
	 *          ...
	 *          argv[argc-1]
	 *          null
	 * environ: environ[0]    <-- environment variables, required by main() and getenv()
	 *          environ[1]
	 *          ...
	 *          null
	 * _auxv:   _auxv[0]      <-- auxiliary vector, required by getauxval()
	 *          _auxv[1]
	 *          ...
	 *          null
	 */

	/* assign argc and argv */
	argc = *sp;
	argv = (void *)(sp + 1);

	/* find environ */
	environ = envp = argv + argc + 1;

	/* find _auxv */
	for (auxv = (void *)envp; *auxv++;)
		;
	_auxv = auxv;

	/* go to application */
	exit(_nolibc_main(argc, argv, envp));
}

#if defined(__x86_64__) || defined(i386)
#include <x86intrin.h>

//12700k
#define TSC_HZ 3600000000
static inline long long get_tsc(void) {
    unsigned int temp;
    return __rdtscp(&temp);
}

static inline long long
__attribute__((always_inline))
get_cycle(void) {
    return (long long)(get_tsc() * 1.38888888888888888888);
}

#elif defined(__loongarch__)
#include <larchintrin.h>
// 3a5000
#define TSC_HZ 100000000
static inline long long get_tsc(void) {
    long long r;
    asm volatile(
    "rdtime.d %0, $r0 \n\t"
    :"=r"(r)
    :
    :"memory"
    );
    return r;
}

static inline long long
__attribute__((always_inline))
get_cycle(void) {

    return get_tsc() * 25;
}

#elif defined(__riscv)
// d1 c906
#define TSC_HZ 24000000
static inline long long get_tsc(void) {
    long long r;
    asm volatile(
    "rdtime %0 \n\t"
    :"=r"(r)
    :
    :"memory"
    );
    return r;
}

static inline long long
__attribute__((always_inline))
get_cycle(void) {
    long long r;
    asm volatile(
    "rdcycle %0 \n\t"
    :"=r"(r)
    :
    :"memory"
    );
    return r;
}

static inline long long
__attribute__((always_inline))
get_instret(void) {
    long long r;
    asm volatile(
    "rdinstret %0 \n\t"
    :"=r"(r)
    :
    :"memory"
    );
    return r;
}

#elif defined(__aarch64__)

#define TSC_HZ 24000000
static inline long long get_tsc(void) {
    struct timespec _t;
    my_syscall2(__NR_clock_gettime, 0, &_t);
    return _t.tv_sec * 1000000000ll + _t.tv_nsec;
}

#else
    #warning "no intrin.h support"
#endif

int clock_gettime(clockid_t clockid, struct timespec *tp) {
#ifdef USE_TSC
    long long t = get_tsc();
    tp->tv_sec = t / 1000000000;
    tp->tv_nsec = t % 1000000000;
#else
    my_syscall2(__NR_clock_gettime, clockid, tp);
#endif
    return 0;
}

static __attribute__((unused))
void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd,
	       off_t offset)
{
	int n;

#if defined(__NR_mmap2)
	n = __NR_mmap2;
	offset >>= 12;
#else
	n = __NR_mmap;
#endif

	return (void *)my_syscall6(n, addr, length, prot, flags, fd, offset);
}


/* flags for mmap */
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#define PROT_READ	0x1		/* Page can be read.  */
#define PROT_WRITE	0x2		/* Page can be written.  */
#define PROT_EXEC	0x4		/* Page can be executed.  */
#define PROT_NONE	0x0		/* Page can not be accessed.  */
#define PROT_GROWSDOWN	0x01000000	/* Extend change to start of
					   growsdown vma (mprotect only).  */
#define PROT_GROWSUP	0x02000000	/* Extend change to start of
					   growsup vma (mprotect only).  */

/* Sharing types (must choose one and only one of these).  */
#define MAP_SHARED	0x01		/* Share changes.  */
#define MAP_PRIVATE	0x02		/* Changes are private.  */

/* 0x01 - 0x03 are defined in linux/mman.h */
#define MAP_TYPE	0x0f		/* Mask for type of mapping */
#define MAP_FIXED	0x10		/* Interpret addr exactly */
#define MAP_ANONYMOUS	0x20		/* don't use a file */

/* 0x0100 - 0x4000 flags are defined in asm-generic/mman.h */
#define MAP_POPULATE		0x008000	/* populate (prefault) pagetables */
#define MAP_NONBLOCK		0x010000	/* do not block on IO */
#define MAP_STACK		0x020000	/* give out an address that is best suited for process/thread stacks */
#define MAP_HUGETLB		0x040000	/* create a huge page mapping */
#define MAP_SYNC		0x080000 /* perform synchronous page faults for the mapping */
#define MAP_FIXED_NOREPLACE	0x100000	/* MAP_FIXED which doesn't unmap underlying mapping */

#define MAP_UNINITIALIZED 0x4000000	/* For anonymous mmap, memory could be
					 * uninitialized */

/* Note that on Linux, MAP_FAILED is -1 so we can use the generic __sysret()
 * which returns -1 upon error and still satisfy user land that checks for
 * MAP_FAILED.
 */

static __attribute__((unused))
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	void *ret = sys_mmap(addr, length, prot, flags, fd, offset);

	if ((unsigned long)ret >= -4095UL) {
		// SET_ERRNO(-(long)ret);
		ret = MAP_FAILED;
	}
	return ret;
}

 __attribute__((unused))
void *malloc(size_t len)
{
	len  = (len + 4095UL) & -4096UL;
	void* heap = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE,
		    -1, 0);
	if (__builtin_expect(heap == MAP_FAILED, 0))
		return NULL;
	return heap;
}

__attribute__((unused))
void free(void *ptr)
{

}
