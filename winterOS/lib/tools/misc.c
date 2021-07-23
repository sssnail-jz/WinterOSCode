// ----------------------------
// <misc.c>
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

PUBLIC int send_recv(int function, int src_dest, MESSAGE *msg)
{
	int _ret = 0;
	if (function == RECEIVE)
		memset(msg, 0, sizeof(MESSAGE));

	switch (function)
	{
	case BOTH:
		_ret = sendrec(SEND, src_dest, msg);
		if (_ret == 0)
			_ret = sendrec(RECEIVE, src_dest, msg);
		break;
	case SEND:
	case RECEIVE:
		_ret = sendrec(function, src_dest, msg);
		break;
	default:
		assert((function == BOTH) || (function == SEND) || (function == RECEIVE));
		break;
	}

	return _ret;
}

PUBLIC int strcmp(const char *s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0))
		return -1;

	while (*s1 && *s2)
		if (*s1++ != *s2++)
			return 1;

	return (*s1 - *s2);
}

PUBLIC char *strcat(char *s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0))
		return 0;

	while (*s1++)
		__asm__ __volatile__("NOP");
	--s1;
	while (*s2)
		*s1++ = *s2++;
	*s1 = 0;

	return s1;
}

PUBLIC int subIndex_tail(char *haystack, char *needle)
{
    int m = strlen(haystack); //计算两字符串长度
    int n = strlen(needle);
    if (n == 0)
        return 0;
	int i;
    for (i = 0; i < m; i++)
    {
        int j = 0;
        int k = i;
        while (haystack[k] == needle[j] && j < n)
        {
            k++;
            j++;
            if (haystack[k] != needle[j] && j < n)
                break;
        }
        if (j == n)
            return k;
    }
    return -1;
}

PUBLIC void spin(char *func_name)
{
	printk("\nSpinning in --> [ %s ]\n", func_name);
	while (TRUE)
		__asm__ __volatile__("NOP");
}

PUBLIC void assertion_failure(char *exp, char *file, char *base_file, int line)
{
	printk("%cassert( %s ) failed! -->\n"
		   "FILE      : [ %s ]\n"
		   "BASE_FILE : [ %s ]\n"
		   "LINE      : [ %d ]",
		   MAG_CH_ASSERT, exp, file, base_file, line);
	spin("assertion_failure()");

	// should never arrive here
	__asm__ __volatile__("UD2");
}

PUBLIC void panic(const char *fmt, ...)
{
	int _cntVs;
	char buf[256];

	va_list arg = (va_list)((char *)&fmt + 4);
	_cntVs = vsprintf(buf, fmt, arg);
	printk("%cpanic --> %s", MAG_CH_PANIC, buf);

	// should never arrive here
	__asm__ __volatile__("UD2");
}
