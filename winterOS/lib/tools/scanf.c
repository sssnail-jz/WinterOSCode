// ----------------------------
// <scanf.c>
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

PUBLIC int scanf(char *fmt, void *pDest)
{
    int _cnt;
    char _buf[160];
    // we read from TTY, so FD is '1'
    _cnt = read(1, _buf, sizeof(_buf));
    _buf[_cnt] = 0;

    if (strlen(fmt) == 2 && fmt[0] == '%' && (fmt[1] == 'x' || fmt[1] == 'd' || fmt[1] == 'c' || fmt[1] == 's'))
    {
        int _m;
        switch (fmt[1])
        {
        case 'c':
            *((char *)pDest) = *_buf;
            break;
        case 'x':
            _m = h2i(_buf);
            *((int *)pDest) = _m;
            break;
        case 'd':
            _m = a2i(_buf);
            *((int *)pDest) = _m;
            break;
        case 's':
            strcpy((char *)pDest, _buf);
            break;
        default:
            break;
        }
    }
    else
        panic("scanf format is error!\n");
    return _cnt;
}
