// ----------------------------
// <mm_main.c>
//				Jack Zheng
//				3112490969@qq.com
// ----------------------------
#include "type.h"
#include "config.h"
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

PRIVATE void init_mm();

PUBLIC void task_mm()
{
	init_mm();
	int _src;
	int _reply;
	int _msgtype;

	while (TRUE)
	{
		send_recv(RECEIVE, ANY, &mm_msg);

		_src = mm_msg.source;
		_reply = TRUE;
		_msgtype = mm_msg.type;

		switch (_msgtype)
		{
		case FORK:
			mm_msg.RETVAL = do_fork();
			break;
		case EXIT:
			do_exit(mm_msg.STATUS);
			_reply = FALSE;
			break;
		case EXEC:
			mm_msg.RETVAL = do_exec();
			break;
		case WAIT:
			do_wait();
			_reply = FALSE;
			break;
		default:
			panic("MM::unknown msg");
			assert(0);
			break;
		}

		if (_reply)
		{
			mm_msg.type = SYSCALL_RET;
			send_recv(SEND, _src, &mm_msg);
		}
	}
}

PRIVATE void init_mm()
{
	struct boot_params bp;
	get_boot_params(&bp);

	memory_size = bp.mem_size;
}

PUBLIC int alloc_mem(int pid, int memsize)
{
	assert(pid >= NR_TASKS + NR_NATIVE_PROCS);
	if (memsize > PROC_IMAGE_SIZE_DEFAULT)
		panic("unsupported memory request: %d. (should be less than %d)",
			  memsize, PROC_IMAGE_SIZE_DEFAULT);

	int _base = PROCS_BASE + (pid - (NR_TASKS + NR_NATIVE_PROCS)) * PROC_IMAGE_SIZE_DEFAULT;

	if (_base + memsize >= memory_size)
		panic("memory allocation failed. pid:%d", pid);

	return _base;
}

PUBLIC int free_mem(int pid)
{
	return 0;
}
