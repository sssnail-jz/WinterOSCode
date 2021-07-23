// ----------------------------
// <tty.c>
// 		Jack Zheng
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

PRIVATE void init_tty();
PRIVATE void tty_dev_read();
PRIVATE void tty_dev_write();
PRIVATE void tty_do_read();
PRIVATE void tty_do_write();
PRIVATE void tty_do_clean();
PRIVATE void put_key(TTY *p_tty, u32 key);

PRIVATE TTY *pTTY;
PRIVATE MESSAGE tty_msg;
PRIVATE int edit_flags = FALSE;
PRIVATE int ctl_in_buf = FALSE;

PUBLIC void task_tty()
{
	for (pTTY = TTY_FIRST; pTTY < TTY_END; pTTY++)
		init_tty(pTTY);

	select_console(0);

	while (TRUE)
	{
		for (pTTY = TTY_FIRST; pTTY < TTY_END; pTTY++)
		{
			tty_dev_read(pTTY);
			tty_dev_write(pTTY);
		}

		send_recv(RECEIVE, ANY, &tty_msg);

		pTTY = &tty_table[tty_msg.DEVICE];
		assert(pTTY >= TTY_FIRST && pTTY <= TTY_END);
		assert(tty_msg.source != TASK_TTY);

		switch (tty_msg.type)
		{
		case TTY_OPEN:
			tty_msg.type = SYSCALL_RET;
			send_recv(SEND, tty_msg.source, &tty_msg);
			break;
		case TTY_READ:
			tty_do_read();
			break;
		case TTY_WRITE:
			tty_do_write();
			break;
		case TTY_CLEAR:
			tty_do_clean();
			break;
		case HARD_INT:
			break; // depend on kb_in.cont,so we do nothing ...
		default:
			panic("TTY::unknown msg");
			break;
		}
	}
}

PRIVATE void init_tty()
{
	pTTY->ibuf_cnt = 0;
	pTTY->ibuf_head = pTTY->ibuf_tail = pTTY->ibuf;

	init_screen(pTTY);
}

/**
 * 使 TTY 表现出不同的行为目前取决于 in_process 和 tty_dev_write 函数 
 */
PUBLIC void in_process(TTY *p_tty, u32 key)
{
	if (!(key & FLAG_EXT))
		put_key(p_tty, key);
	else
	{
		int raw_code = key & MASK_RAW;
		// 如果用户请求控制字符，我们将控制字符送给用户缓冲区
		// 并且不对其处理，因为用户要对控制字符进行处理
		if (ctl_in_buf)
		{
			put_key(p_tty, raw_code);
			return;
		}
		switch (raw_code)
		{
		case ENTER:
			put_key(p_tty, '\n');
			break;
		case BACKSPACE:
			put_key(p_tty, '\b');
			break;
		case UP:
			scroll_screen(p_tty->console, SCR_DN);
			break;
		case DOWN:
			scroll_screen(p_tty->console, SCR_UP);
			break;
		case F1:
		case F2:
		case F3:
			if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R))
				select_console(raw_code - F1);
			break;
		case ESC:
			if (edit_flags)
			{
				MESSAGE msg2caller;
				msg2caller.type = RESUME_PROC;
				msg2caller.PROC_NR = pTTY->tty_procnr; // need 'FS' to wake up the 'P'
				msg2caller.CNT = pTTY->tty_trans_cnt;
				send_recv(SEND, pTTY->tty_caller, &msg2caller);

				edit_flags = FALSE; // 非常重要
			}
			break;
		case TAB:
			put_key(p_tty, ' ');
			put_key(p_tty, ' ');
			put_key(p_tty, ' ');
			put_key(p_tty, ' ');
			break;
		default:
			break;
		}
	}
}

PRIVATE void put_key(TTY *p_tty, u32 key)
{
	if (p_tty->ibuf_cnt < TTY_IN_BYTES)
	{
		*p_tty->ibuf_head++ = key;
		if (p_tty->ibuf_head == p_tty->ibuf + TTY_IN_BYTES)
			p_tty->ibuf_head = p_tty->ibuf;
		p_tty->ibuf_cnt++;
	}
}

PRIVATE void tty_dev_read()
{
	if (is_current_console(pTTY->console))
		keyboard_read(pTTY);
}

PRIVATE void tty_dev_write()
{
	while (pTTY->ibuf_cnt)
	{
		// reduce the char queue
		char _char = (char)*(pTTY->ibuf_tail);
		pTTY->ibuf_tail++;
		if (pTTY->ibuf_tail == pTTY->ibuf + TTY_IN_BYTES)
			pTTY->ibuf_tail = pTTY->ibuf;
		pTTY->ibuf_cnt--;

		if (pTTY->tty_left_cnt)
		{
			if (ctl_in_buf) // 如果请求的是控制字符，直接交给用户不做处理
			{
				int _ctlChar = _char | FLAG_EXT; // 控制字符多余 1 个字节

				// out char to process's buffer
				void *_pos = pTTY->tty_proc_buf + pTTY->tty_trans_cnt;
				phys_copy(_pos, (void *)&_ctlChar, 4);
				pTTY->tty_trans_cnt++;
				pTTY->tty_left_cnt--;

				if (pTTY->tty_left_cnt == 0)
				{
					MESSAGE msg2caller;
					msg2caller.type = RESUME_PROC;
					msg2caller.PROC_NR = pTTY->tty_procnr; // need 'FS' to wake up the 'P'
					msg2caller.CNT = pTTY->tty_trans_cnt;
					send_recv(SEND, pTTY->tty_caller, &msg2caller);
				}
				
				ctl_in_buf = FALSE; // 非常重要
				continue;
			}

			if (_char >= ' ' && _char <= '~') /* printable */
			{
				// out char to screen
				out_char(pTTY->console, _char);

				// out char to process's buffer
				void *_pos = pTTY->tty_proc_buf + pTTY->tty_trans_cnt;
				phys_copy(_pos, &_char, 1);
				pTTY->tty_trans_cnt++;
				pTTY->tty_left_cnt--;
			}
			else if (_char == '\b' && pTTY->tty_trans_cnt)
			{
				// notify screen to delete the char
				out_char(pTTY->console, _char);

				// invalid the char in process's buffer
				pTTY->tty_trans_cnt--;
				pTTY->tty_left_cnt++;
			}
			// end the output and notify the process by message
			else if (_char == '\n' || pTTY->tty_left_cnt == 0)
			{
				out_char(pTTY->console, _char);
				if (edit_flags)
				{
					// out char to process's buffer
					void *_pos = pTTY->tty_proc_buf + pTTY->tty_trans_cnt;
					phys_copy(_pos, &_char, 1);
					pTTY->tty_trans_cnt++;
					pTTY->tty_left_cnt--;
				}
				else
				{
					MESSAGE msg2caller;
					msg2caller.type = RESUME_PROC;
					msg2caller.PROC_NR = pTTY->tty_procnr; // need 'FS' to wake up the 'P'
					msg2caller.CNT = pTTY->tty_trans_cnt;
					send_recv(SEND, pTTY->tty_caller, &msg2caller);
				}
			}
		}
	}
}

PRIVATE void tty_do_read()
{
	pTTY->tty_caller = tty_msg.source;
	pTTY->tty_procnr = tty_msg.PROC_NR;
	pTTY->tty_proc_buf = va2la(pTTY->tty_procnr, tty_msg.BUF);
	pTTY->tty_left_cnt = tty_msg.CNT;
	// 如果为 0 ，那么说明进程发起了读请求，但是读取数目为 0 ，
	// 则永远不会往用户缓冲区送数据，永远不会回车唤醒用户进程
	assert(pTTY->tty_left_cnt != 0);

	edit_flags = (tty_msg.REQUEST & EDIT) ? TRUE : FALSE;
	ctl_in_buf = (tty_msg.REQUEST & CTL_IN_BUF) ? TRUE : FALSE;

	if (edit_flags) // if 'edit' read
	{
		int i = 0;
		pTTY->tty_trans_cnt = 0;
		char *_buf = va2la(pTTY->tty_procnr, tty_msg.BUF);
		while (_buf[i])
		{
			out_char(pTTY->console, _buf[i++]);
			pTTY->tty_trans_cnt++;
		}
	}
	else
		pTTY->tty_trans_cnt = 0;

	// 'TTY' received the 'read request' and notify the 'FS' everything is OK
	tty_msg.type = SUSPEND_PROC;
	tty_msg.CNT = pTTY->tty_left_cnt;
	send_recv(SEND, pTTY->tty_caller, &tty_msg);
}

PRIVATE void tty_do_write()
{
	char *_procBuf = (char *)va2la(tty_msg.PROC_NR, tty_msg.BUF);
	int _left_cnt = tty_msg.CNT;
	// out put all process's chars to screen
	while (_left_cnt--)
		out_char(pTTY->console, *_procBuf++);

	// notify 'FS' then 'FS' notify 'P'
	tty_msg.type = SYSCALL_RET;
	send_recv(SEND, tty_msg.source, &tty_msg);
}

/**
 * 'clear_screen'  and 'flush' are in console.c
 */
PRIVATE void tty_do_clean()
{
	clear_screen(pTTY->console->orig, pTTY->console->cursor - pTTY->console->orig); // 以 ‘字’ 遍历
	pTTY->console->cursor = pTTY->console->crtc_start = pTTY->console->orig; // because will write 'notice'
	flush(pTTY->console);
	tty_msg.type = SYSCALL_RET;
	send_recv(SEND, tty_msg.source, &tty_msg);
}

PUBLIC int sys_printx(int _unused1, int _unused2, char *s, struct proc *p_proc)
{
	const char *_str;
	char _char;

	// ring[1~3],need to transform to liner address
	if (k_reenter == 0)
		_str = va2la(proc2pid(p_proc), s);
	// ring0,needn't transform
	else if (k_reenter > 0)
		_str = s;

	if ((*_str == MAG_CH_PANIC) || (*_str == MAG_CH_ASSERT && p_proc_ready < &proc_table[NR_TASKS]))
	{
		disable_int();
		char *_video = (char *)V_MEM_BASE;
		const char *_pCh = _str + 1;

		// 污染整个显存
		while (_video < (char *)(V_MEM_BASE + V_MEM_SIZE))
		{
			if (*_pCh == '\n')
			{
				_video += SCR_WIDTH * 2 - ((int)_video - V_MEM_BASE) % (SCR_WIDTH * 2);
				_pCh++;
			}
			*_video++ = *_pCh++;
			*_video++ = BLUE_RED_CHAR;
			if (!*_pCh)
			{
				while (((int)_video - V_MEM_BASE) % (SCR_WIDTH * 16))
				{
					_video++;
					*_video++ = GRAY_CHAR;
				}
				_pCh = _str + 1;
			}
		}

		__asm__ __volatile__("HLT");
	}

	while ((_char = *_str++) != 0)
	{
		if (_char == MAG_CH_PANIC || _char == MAG_CH_ASSERT)
			continue;
		out_char(TTY_FIRST->console, _char);
	}

	return 0;
}
