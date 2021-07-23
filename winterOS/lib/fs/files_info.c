// ----------------------------
// <files_info.c>
// 				Jack Zheng
//				3112490969@qq.com
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

PUBLIC int files_info(char * buf)
{
	MESSAGE msg;
	msg.type = FILSS_INFO;
	msg.BUF = (void *)buf;
	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);
	return msg.CNT;
}
