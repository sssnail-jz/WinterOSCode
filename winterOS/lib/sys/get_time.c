// ----------------------------
// <get_time.c>
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
#include "proto.h"

PUBLIC int get_time(struct time * _pTime)
{
    MESSAGE msg;
    msg.type = GET_RTC_TIME;
    msg.BUF = _pTime;
    send_recv(BOTH, TASK_SYS, &msg);
    assert(msg.type == SYSCALL_RET);
    return msg.PID;
}
