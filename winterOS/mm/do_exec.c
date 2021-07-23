// ----------------------------
// <do_exec.c>
//				Jack Zheng
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
#include "keyboard.h"
#include "proto.h"
#include "elf.h"

/**
 * 1, copy stack and reset the stack
 * 3, overwrite the proc img
 * 4, adjust ip, sp and set cx, ax
 * the route is (from user proc to MM finall back to user proc):
 * 1) user proc send msg to 'MM' and block itself
 * 2) 'MM' recved the msg, and overrite the user proc's proc img
 * 	  and reset proc's ip, sp ... then set proc's RUNABLE flags
 * 3) next schdule, load proc's modified 'ip','sp' ... and switch to usr proc
 */
PUBLIC int do_exec()
{
	int _nameLen = mm_msg.NAME_LEN;
	int _src = mm_msg.source;
	assert(_nameLen < MAX_PATH);

	char _pathname[MAX_PATH];
	phys_copy(_pathname, (void *)va2la(_src, mm_msg.PATHNAME), _nameLen);
	_pathname[_nameLen] = 0;

	// copy the arg stack(let this step front overwrite proc's img step)
	int orig_stack_len = mm_msg.BUF_LEN;
	char _stackCopy[PROC_ORIGIN_STACK];
	phys_copy(_stackCopy, (void *)va2la(_src, mm_msg.BUF), orig_stack_len);

	// OK, adjust offset of 'argv'
	u8 *_origStack = (u8 *)(PROC_IMAGE_SIZE_DEFAULT - PROC_ORIGIN_STACK);
	int _delta = (int)_origStack - (int)mm_msg.BUF;
	int _argc = 0;
	if (orig_stack_len)
	{
		char **_q;
		for (_q = (char **)_stackCopy; *_q != NULL; _q++, _argc++)
			*_q += _delta;
	}
	// change stack's pos to (0x100000 - 0x400)
	phys_copy((void *)va2la(_src, _origStack), _stackCopy, orig_stack_len);

	// get the file size
	struct stat _stat;
	int _ret = stat(_pathname, &_stat);
	assert(_ret == 0);

	// open and read the binary file
	int _fd = open(_pathname, O_RDWT);
	if (_fd == -1)
		return -1;
	assert(_stat.st_size < MMBUF_SIZE);
	read(_fd, mmbuf, _stat.st_size);
	close(_fd);

	// overwrite the current proc image with the new one
	Elf32_Ehdr *_ELFheader = (Elf32_Ehdr *)(mmbuf);
	int i;
	for (i = 0; i < _ELFheader->e_phnum; i++)
	{
		Elf32_Phdr *_progHeader = (Elf32_Phdr *)(mmbuf + _ELFheader->e_phoff +
												 (i * _ELFheader->e_phentsize));
		if (_progHeader->p_type == PT_LOAD)
		{
			assert(_progHeader->p_vaddr + _progHeader->p_memsz < PROC_IMAGE_SIZE_DEFAULT);

			disable_int();
			phys_copy((void *)va2la(_src, (void *)_progHeader->p_vaddr),
					  mmbuf + _progHeader->p_offset,
					  _progHeader->p_filesz);
		}
	}

	proc_table[_src].regs.ecx = _argc;
	proc_table[_src].regs.eax = (u32)_origStack;

	proc_table[_src].regs.eip = _ELFheader->e_entry;
	// OK, we reset pos of p_msg, because when 'MM' msg_send, will copy msg to
	proc_table[_src].p_msg = (MESSAGE *)((PROC_IMAGE_SIZE_DEFAULT - PROC_ORIGIN_STACK) -
							 sizeof(*(MESSAGE *)proc_table[_src].p_msg));
	proc_table[_src].regs.esp = (int)proc_table[_src].p_msg;

	enable_int();

	strcpy(proc_table[_src].name, _pathname);

	return 0;
}
