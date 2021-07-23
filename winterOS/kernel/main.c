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

PUBLIC int kernel_main()
{
	int i, j;
	int _eflags, _prio;
	u8 _rpl, _priv;

	struct task *_pTask;
	struct proc *_pProc = proc_table;
	char *_pStk = task_stack + STACK_SIZE_TOTAL;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++, _pProc++, _pTask++)
	{
		if (i >= NR_TASKS + NR_NATIVE_PROCS) // is free proc slot
		{
			_pProc->p_status = FREE_SLOT;
			continue;
		}

		if (i < NR_TASKS) // is sys task
		{
			_pTask = task_table + i;
			_priv = PRIVILEGE_TASK;
			_rpl = RPL_TASK;
			_eflags = 0x1202; // IF=1, IOPL=1, bit 2 is always 1
			_prio = 15;
		}
		else // is user proc
		{
			_pTask = user_proc_table + (i - NR_TASKS);
			_priv = PRIVILEGE_USER;
			_rpl = RPL_USER;
			_eflags = 0x202; // IF=1, bit 2 is always 1
			_prio = 5;
		}

		strcpy(_pProc->name, _pTask->name);
		_pProc->p_parent = NO_TASK;

		if (strcmp(_pTask->name, "INIT") != 0)
		{
			_pProc->ldts[INDEX_LDT_C] = gdt[SELECTOR_KERNEL_CS >> 3];
			_pProc->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			// change the DPLs
			_pProc->ldts[INDEX_LDT_C].attr1 = DA_C | _priv << 5;
			_pProc->ldts[INDEX_LDT_RW].attr1 = DA_DRW | _priv << 5;
		}
		else // 'INIT' process
		{
			unsigned int _k_base;
			unsigned int _k_limit;
			assert(get_kernel_map(&_k_base, &_k_limit) == 0);

			init_desc(&_pProc->ldts[INDEX_LDT_C],
					  0, // INIT's base must '0', because '10400'
					  (_k_base + _k_limit) >> LIMIT_4K_SHIFT,
					  DA_32 | DA_LIMIT_4K | DA_C | _priv << 5);

			init_desc(&_pProc->ldts[INDEX_LDT_RW],
					  0, // INIT's base must '0', because '10400'
					  (_k_base + _k_limit) >> LIMIT_4K_SHIFT,
					  DA_32 | DA_LIMIT_4K | DA_DRW | _priv << 5);
		}

		_pProc->regs.cs = INDEX_LDT_C << 3 | SA_TIL | _rpl;
		_pProc->regs.ds =
			_pProc->regs.es =
				_pProc->regs.fs =
					_pProc->regs.ss =
						INDEX_LDT_RW << 3 | SA_TIL | _rpl;
		_pProc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | _rpl;
		_pProc->regs.eip = (u32)_pTask->initial_eip;
		_pProc->regs.esp = (u32)_pStk;
		_pProc->regs.eflags = _eflags;

		_pProc->ticks = _pProc->priority = _prio;

		_pProc->p_status = RUNABLE;
		_pProc->p_msg = NULL;
		_pProc->p_wntRecv_save = NO_TASK;
		_pProc->p_wntSend_save = NO_TASK;
		_pProc->has_int_msg = 0;
		_pProc->q_sending = NULL;
		_pProc->next_sending = NULL;

		for (j = 0; j < NR_FILES; j++)
			_pProc->filp[j] = NULL;

		_pStk -= _pTask->stacksize;
	}

	k_reenter = 0;
	ticks = 0;
	msg_counter = 0;
	p_proc_ready = proc_table;

	init_clock();
	init_keyboard();

	restart();

	spin("Shouldn'_pTask arrive here!!!");
	return 0;
}

void untar(const char *filename)
{
	printk("\n##EXTRACT [%s] -->>\n", filename);
	int fd = open(filename, O_RDWT);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);
	BOOL _load = FALSE;
	while (TRUE)
	{
		read(fd, buf, SECTOR_SIZE);
		if (buf[0] == 0) // end of 'tar file'
		{
			if (!_load)
			{
				set_color((u8)(MAKE_COLOR(BLACK, GREEN) | BRIGHT));
				printk("\n--binary files are already loaded ...\n\n");
				set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
			}
			else
			{
				// OK, we dosn't allow reload the 'binary files'
				seek(fd, 0, SEEK_SET);
				buf[0] = 0;
				write(fd, buf, 1);
				close(fd);
			}
			printk("##<<-- DONE!\n\n");
			return;
		}
		_load = TRUE;
		struct posix_tar_header *phdr = (struct posix_tar_header *)buf;

		// calculate the file size
		char *p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWT);
		if (fdout == -1)
		{
			printk("## failed to extract file: %s ##", phdr->name);
			printk(" ABORTED ]]");
			return;
		}
		clear(fdout); // 先清除可执行文件再写入
		printk("      [%-12s] : %-6d Byte\n", phdr->name, f_len);
		while (bytes_left)
		{
			int iobytes = min(chunk, bytes_left);
			read(fd, buf, ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			write(fdout, buf, iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}
}

void shabby_shell(const char *tty_name, int _shellIndex)
{
	int fd_stdin = open(tty_name, O_RDWT);
	assert(fd_stdin == 0);
	int fd_stdout = open(tty_name, O_RDWT);
	assert(fd_stdout == 1);

	char _rdBuf[128];
	char _stackBuf[128];
	char _notice[80];
	struct time _time;

	get_time(&_time);
	set_color((u8)(MAKE_COLOR(BLACK, GREEN) | BRIGHT));
	write(fd_stdout, "\n--------------------Jack Zheng--------------------\n\n", 54);
	set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
	sprintf(_notice, "_time : Y %d M %d D %d - %d:%d:%d\n",
			_time.year, _time.month, _time.day, _time.hour, _time.minute, _time.second);
	write(fd_stdout, _notice, strlen(_notice));

	sprintf(_notice, "[root@tty_%dst winterOS]# ", _shellIndex);

	while (TRUE)
	{

		set_color((u8)(MAKE_COLOR(BLACK, GREEN) | BRIGHT));
		write(fd_stdout, _notice, strlen(_notice));
		set_color((u8)(MAKE_COLOR(BLACK, WHITE)));

		int _reCnt = read(fd_stdin, _rdBuf, sizeof(_rdBuf));
		_rdBuf[_reCnt] = 0;

		// 将 _stackBuf 制作为参数字符串
		memcpy(_stackBuf, _rdBuf, sizeof(_stackBuf));

		int argc = 0;
		char *argv[PROC_ORIGIN_STACK];
		char *_iterator = _stackBuf;
		char *_subStrBegin;
		int word = 0;
		char _ch;

		do
		{
			_ch = *_iterator;
			if (*_iterator != ' ' && *_iterator != 0 && !word)
			{
				_subStrBegin = _iterator;
				word = 1;
			}
			if ((*_iterator == ' ' || *_iterator == 0) && word)
			{
				word = 0;
				argv[argc++] = _subStrBegin;
				*_iterator = 0;
			}
			_iterator++;
		} while (_ch);

		argv[argc] = 0;

		int fd = open(argv[0], O_RDWT);
		if (fd == -1)
		{
			if (_rdBuf[0])
			{
				write(fd_stdout, "-witrsh [", 10);
				write(fd_stdout, _rdBuf, _reCnt);
				write(fd_stdout, "] command not found\n", 20);
			}
		}
		else
		{
			close(fd);
			int _pID = fork();
			if (_pID != 0)
			{
				int _status;
				wait(&_status);
			}
			else
			{
				execv(argv[0], argv);
				panic("should't arrive here(int kernel/main.c)");
			}
		}
	}

	close(fd_stdout);
	close(fd_stdin);
}

void Init()
{
	int fd_stdin = open("/dev_tty0", O_RDWT);
	assert(fd_stdin == 0);
	int fd_stdout = open("/dev_tty0", O_RDWT);
	assert(fd_stdout == 1);

	// extract `cmd.tar'
	untar("/cmd.tar");

	char *tty_list[] = {"/dev_tty1", "/dev_tty2"};

	int i;
	for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++)
	{
		int _pID = fork();
		if (_pID == 0)
		{
			close(fd_stdin);
			close(fd_stdout);
			shabby_shell(tty_list[i], i + 1);
			assert(0);
		}
	}

	while (TRUE)
	{
		int _status;
		int _child = wait(&_status);
		printf("[INIT as parent] : child ID [%2d] exited with status: [%2d]\n", _child, _status);
	}

	assert(0);
}

void TestA()
{
	while (TRUE)
		__asm__ __volatile__("NOP");
}

void TestB()
{
	while (TRUE)
		__asm__ __volatile__("NOP");
}

void TestC()
{
	while (TRUE)
		__asm__ __volatile__("NOP");
}
