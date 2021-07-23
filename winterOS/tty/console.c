// ----------------------------
// <console.c>
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

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);

/**
 * 怎么说呢 ...
 * 当前的这种头和尾多留出一行算是能解决目前的需求，
 * 这里的起点和结束都是以 SCR_WIDTH 对齐的
 */
PUBLIC void init_screen(TTY *tty)
{
	int _nr_tty = tty - tty_table;
	tty->console = console_table + _nr_tty;

	// 这下面的单位是 world 不是 byte
	int _totalSize = V_MEM_SIZE >> 1;
	// 主控制台 100 行
	int _size_0st = 100 * SCR_WIDTH;
	// 剩下的控制台拆分剩下的显存
	int _size_per_con = (_totalSize - _size_0st) / (NR_CONSOLES - 1);

	if (_nr_tty == 0)
	{
		tty->console->orig = 0;
		tty->console->con_size = _size_0st;
	}
	else
	{
		// 顶头留出一行
		tty->console->orig = _size_0st + (_nr_tty - 1) * _size_per_con;
		tty->console->orig = SCR_WIDTH * (tty->console->orig / SCR_WIDTH + 1);
		// 末尾留出一行
		tty->console->con_size = _size_per_con - (_size_per_con % SCR_WIDTH) - SCR_WIDTH;
	}

	tty->console->cursor = tty->console->crtc_start = tty->console->orig;

	if (_nr_tty == 0)
	{
		tty->console->cursor = disp_pos / 2;
		disp_pos = 0;
	}

	set_cursor(tty->console->cursor);
}

PUBLIC void out_char(CONSOLE *con, char ch)
{
	if (ch == 0) // it is necessary for out put '0'
		return;
	assert(con >= console_table && con <= &console_table[NR_CONSOLES - 1]);

	u8 *_pVideo = (u8 *)(V_MEM_BASE + con->cursor * 2);
	assert(con->cursor - con->orig < con->con_size);

	switch (ch)
	{
	case '\n':
	{
		int _cnt = SCR_WIDTH - con->cursor % SCR_WIDTH;
		while (_cnt--)
		{
			*_pVideo++ = 0; // 使用若干个 0 转换换行符
			*_pVideo++ = DEFAULT_CHAR_COLOR;
			con->cursor++;
		}
	}
	break;
	case '\b':
		if (con->cursor > con->orig)
		{
			if (*(_pVideo - 2) == 0) // 换行符的标志
			{
				int _cnt = SCR_WIDTH;
				while (*(_pVideo - 2) == 0)
				{
					_pVideo -= 2; // 判断下一个
					con->cursor--;
					// 因为换行符都用填充 0 来表示，所以多个换行符就会有多个 0 ，小心一次删除多个换行
					if (!--_cnt)
						break;
				}
			}
			else
			{
				con->cursor--;
				// 这里非常奇怪，用 _pVideo-- 是不行的
				*(_pVideo - 2) = ' ';
				*(_pVideo - 1) = DEFAULT_CHAR_COLOR;
			}
		}
		break;
	default:
		*_pVideo++ = ch;
		*_pVideo++ = DEFAULT_CHAR_COLOR;
		con->cursor++;
		break;
	}

	// 控制台所拥有的显存已经写满
	// 但愿这里能够工作良好 ...
	if (con->cursor - con->orig >= con->con_size)
	{
		// 需要拷贝到起点的字符
		int _copy = SCR_SIZE - (SCR_WIDTH - con->cursor % SCR_WIDTH);
		phys_copy((void *)(V_MEM_BASE + con->orig * 2),
				  (void *)(V_MEM_BASE + (con->cursor - _copy) * 2),
				  _copy * 2);
		con->crtc_start = con->orig;
		con->cursor = con->orig + _copy;
		flush(con); // 必须在这里,解决闪烁
		clear_screen(con->cursor, con->con_size - _copy);
	}
	else if (con->cursor >= con->crtc_start + SCR_SIZE)
		con->crtc_start += SCR_WIDTH;
	flush(con);
}

PUBLIC void clear_screen(int pos, int len)
{
	u8 *_pVideo = (u8 *)(V_MEM_BASE + pos * 2);
	while (--len >= 0)
	{
		*_pVideo++ = ' ';
		*_pVideo++ = DEFAULT_CHAR_COLOR;
	}
}

PUBLIC int is_current_console(CONSOLE *con)
{
	return ((con - console_table) == current_console);
}

PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

PUBLIC void select_console(int nr_console)
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES))
		return;

	flush(&console_table[current_console = nr_console]);
}

/**
 * scroll_screen is so easy actually! 
 */
PUBLIC void scroll_screen(CONSOLE *con, int dir)
{
	if (dir == SCR_DN)
		if (con->crtc_start > con->orig)
			con->crtc_start -= SCR_WIDTH;
	if (dir == SCR_UP)
		if (con->cursor - con->crtc_start >= SCR_SIZE)
			con->crtc_start += SCR_WIDTH;
	flush(con);
}

PUBLIC void flush(CONSOLE *con)
{
	if (is_current_console(con))
	{
		set_cursor(con->cursor);
		set_video_start_addr(con->crtc_start);
	}
}
