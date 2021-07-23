// ----------------------------
// <exec.c>
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

PUBLIC void execl(const char *path, const char *arg, ...)
{
	va_list parg = (va_list)(&arg);
	char **_p = (char **)parg;
	execv(path, _p);
}

/**
 * copy stack and send msg to 'MM' 
 */
PUBLIC void execv(const char *path, char *argv[])
{
	char **_p = argv;
	char arg_stack[PROC_ORIGIN_STACK];
	int stack_len = 0;

	while (*_p++)
	{
		assert(stack_len + 2 * sizeof(char *) < PROC_ORIGIN_STACK);
		stack_len += sizeof(char *);
	}

	*((int *)(&arg_stack[stack_len])) = 0;
	stack_len += sizeof(char *);

	char **_q = (char **)arg_stack;
	for (_p = argv; *_p != 0; _p++)
	{
		*_q++ = &arg_stack[stack_len];

		assert(stack_len + strlen(*_p) + 1 < PROC_ORIGIN_STACK);
		strcpy(&arg_stack[stack_len], *_p);
		stack_len += strlen(*_p);
		arg_stack[stack_len] = 0; // stop the sring
		stack_len++;
	}

	MESSAGE msg;
	msg.type = EXEC;
	msg.PATHNAME = (void *)path;
	msg.NAME_LEN = strlen(path);
	msg.BUF = (void *)arg_stack;
	msg.BUF_LEN = stack_len;

	send_recv(BOTH, TASK_MM, &msg);
	panic("should't arrive here(int lib/mm/execv)");
}
