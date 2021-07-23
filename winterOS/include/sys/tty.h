#ifndef _WINTEROS_TTY_H_
#define _WINTEROS_TTY_H_

#define TTY_IN_BYTES 256  // tty input queue size
#define TTY_OUT_BUF_LEN 2 // tty output buffer size

struct s_console;

// TTY
typedef struct s_tty
{
	// TTY 面向底层
	u32 ibuf[TTY_IN_BYTES]; // TTY input buffer
	u32 *ibuf_head;
	u32 *ibuf_tail;
	int ibuf_cnt;

	// TTY 面向进程
	int tty_caller; // 谁通知 TTY
	int tty_procnr; // 谁请求
	void *tty_proc_buf;
	int tty_left_cnt;  // 还有多少字节需要传送给进程
	int tty_trans_cnt; // 已经传送了多少字节给进程

	struct s_console *console;
} TTY;
#endif
