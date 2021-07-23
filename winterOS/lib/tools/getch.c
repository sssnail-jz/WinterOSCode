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

/**
 * getch 的作用主要是要返回控制字符，然后用户程序对控制字符做处理
 * winterOS 使用的控制字符的值不是 ASCII 的值，而是自定义的 ...
 * 并且控制字符的值是 2 个字节，这里使用 int 来接收
 */
PUBLIC int getch()
{
	MESSAGE msg;
    int _ctlChar;
	msg.type = READ;
	msg.FD = 1;
	msg.BUF = (char *)&_ctlChar;
	msg.CNT = 1;
	msg.REQUEST = CTL_IN_BUF;
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.CNT == 1);
	return _ctlChar;
}
