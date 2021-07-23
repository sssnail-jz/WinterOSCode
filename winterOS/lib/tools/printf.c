// ----------------------------
// <printf.c>
//              Jack Zheng
//		3112490969@qq.com
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

PUBLIC int printf(const char *fmt, ...)
{
        int _cntVs;
        int _cntWt;
        char _buf[STR_DEFAULT_LEN];

        va_list _arg = (va_list)((char *)(&fmt) + 4);
        _cntVs = vsprintf(_buf, fmt, _arg);
        _cntWt = write(1, _buf, _cntVs);
        assert(_cntWt == _cntVs);

        return _cntVs;
}

PUBLIC int printk(const char *fmt, ...)
{
        int _cnt;
        char buf[STR_DEFAULT_LEN];

        va_list arg = (va_list)((char *)(&fmt) + 4);
        _cnt = vsprintf(buf, fmt, arg);
        printx(buf);

        return _cnt;
}
