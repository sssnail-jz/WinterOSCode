// ----------------------------
// <vsprintf.c>
// 				Jack Zheng
// 				3112490969@qq.com
// ----------------------------
#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PUBLIC char *i2a(int val, int base, char **ps)
{
	int m = val % base;
	int q = val / base;
	if (q)
		i2a(q, base, ps);
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'A');

	return *ps;
}

PUBLIC int a2i(char s[])
{
	int i;
	int n = 0;
	for (i = 0; s[i] >= '0' && s[i] <= '9'; ++i)
		n = 10 * n + (s[i] - '0');
	return n;
}

int tolower(int c)
{
	if (c >= 'A' && c <= 'Z')
		return c + 'a' - 'A';
	else
		return c;
}

PUBLIC int h2i(char s[])
{
	int i;
	int n = 0;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		i = 2;
	else
		i = 0;
	for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'); ++i)
	{
		if (tolower(s[i]) > '9')
			n = 16 * n + (10 + tolower(s[i]) - 'a');
		else
			n = 16 * n + (tolower(s[i]) - '0');
	}
	return n;
}

PUBLIC int vsprintf(char *buf, const char *fmt, va_list args)
{
	char *_pUserBuf = NULL;
	int _nr = 0;
	char _innerBuf[256];
	int _alignNr;
	BOOL _leftAlign = FALSE;
	va_list _pNextArg = args;

	for (_pUserBuf = buf; *fmt; fmt++)
	{
		_leftAlign = FALSE;
		if (*fmt != '%')
		{
			*_pUserBuf++ = *fmt;
			continue;
		}
		else
			_alignNr = 0;
		fmt++;

		if (*fmt == '%') // %%
		{
			*_pUserBuf++ = *fmt;
			continue;
		}
		else if (*fmt == '-') // 左对齐
		{
			_leftAlign = TRUE;
			fmt++;
		}

		// 如果 '%' 后面的是数字，就解释为数字，当作对齐位数
		while (((unsigned char)(*fmt) >= '0') && ((unsigned char)(*fmt) <= '9'))
		{
			_alignNr *= 10;
			_alignNr += *fmt - '0';
			fmt++;
		}

		char *q = _innerBuf;
		memset(q, 0, sizeof(_innerBuf));

		switch (*fmt)
		{
		case 'c':
			*q++ = *((char *)_pNextArg);
			_pNextArg += 4;
			break;
		case 'x':
			_nr = *((int *)_pNextArg);
			i2a(_nr, 16, &q);
			_pNextArg += 4;
			break;
		case 'd':
			_nr = *((int *)_pNextArg);
			if (_nr < 0)
			{
				_nr = _nr * (-1);
				*q++ = '-';
			}
			i2a(_nr, 10, &q);
			_pNextArg += 4;
			break;
		case 's':
			strcpy(q, (*((char **)_pNextArg)));
			q += strlen(*((char **)_pNextArg));
			_pNextArg += 4;
			break;
		default:
			panic("vsprintf format is error!\n");
			break;
		}

		q = _innerBuf;
		int k;
		if (_leftAlign)
		{
			while (*q)
				*_pUserBuf++ = *q++;
			for (k = 0; k < ((_alignNr > strlen(_innerBuf)) ? (_alignNr - strlen(_innerBuf)) : 0); k++)
				*_pUserBuf++ = ' ';
		}
		else
		{
			for (k = 0; k < ((_alignNr > strlen(_innerBuf)) ? (_alignNr - strlen(_innerBuf)) : 0); k++)
				*_pUserBuf++ = ' ';
			while (*q)
				*_pUserBuf++ = *q++;
		}
	}

	*_pUserBuf = 0;

	return (_pUserBuf - buf);
}

int sprintf(char *buf, const char *fmt, ...)
{
	va_list arg = (va_list)((char *)(&fmt) + 4);
	return vsprintf(buf, fmt, arg);
}
