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

PUBLIC int set_color(u8 _color)
{
	MESSAGE msg;
	msg.type = SET_COLOR;
    msg.VALUE = _color;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}
