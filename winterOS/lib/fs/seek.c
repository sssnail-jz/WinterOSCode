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

PUBLIC int seek(int fd, int off, int flags)
{
	MESSAGE msg;
	msg.type = SEEK;
	msg.FD = fd;
	msg.OFFSET = off;
	msg.WHENCE = flags;
	send_recv(BOTH, TASK_FS, &msg);

	return msg.OFFSET;
}
