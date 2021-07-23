// ----------------------------
// <klib.c>
// 				Jack Zheng
// 				3112490969@qq.com
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
#include "proto.h"
#include "elf.h"

PUBLIC void get_boot_params(struct boot_params *pbp)
{
	int *p = (int *)BOOT_PARAM_ADDR;
	assert(p[BI_MAG] == BOOT_PARAM_MAGIC);

	pbp->mem_size = p[BI_MEM_SIZE];
	pbp->addr_kernel_file = (unsigned char *)(p[BI_KERNEL_FILE]);
	pbp->disp_pos = p[BI_DISP_POS];
}

PUBLIC int get_kernel_map(unsigned int *b, unsigned int *l)
{
	struct boot_params _bp;
	get_boot_params(&_bp);

	Elf32_Ehdr *_pELF_header = (Elf32_Ehdr *)(_bp.addr_kernel_file);

	*b = ~0;
	unsigned int t = 0;
	int i;
	for (i = 0; i < _pELF_header->e_shnum; i++)
	{
		Elf32_Shdr *section_header =
			(Elf32_Shdr *)(_bp.addr_kernel_file +
						   _pELF_header->e_shoff +
						   i * _pELF_header->e_shentsize);
		if (section_header->sh_flags & SHF_ALLOC)
		{
			int bottom = section_header->sh_addr;
			int top = section_header->sh_addr +
					  section_header->sh_size;

			if (*b > bottom)
				*b = bottom;
			if (t < top)
				t = top;
		}
	}
	assert(*b < t);
	*l = t - *b - 1;

	return 0;
}

PUBLIC char *itoa(char *str, int num) /* 数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800 */
{
	char *p = str;
	char ch;
	int i;
	int flag = 0;

	*p++ = '0';
	*p++ = 'x';

	if (num == 0)
		*p++ = '0';
	else
	{
		for (i = 28; i >= 0; i -= 4)
		{
			ch = (num >> i) & 0xF;
			if (flag || (ch > 0))
			{
				flag = 1;
				ch += '0';
				if (ch > '9')
					ch += 7;
				*p++ = ch;
			}
		}
	}

	*p = 0;

	return str;
}

PUBLIC void disp_int(int input)
{
	char output[16];
	itoa(output, input);
	disp_str(output);
}

PUBLIC void delay(int time)
{
	int i, j, k;
	for (k = 0; k < time; k++)
	{
		/*for(i=0;i<10000;i++){	for Virtual PC	*/
		for (i = 0; i < 10; i++)
		{ /*	for Bochs	*/
			for (j = 0; j < 10000; j++)
			{
			}
		}
	}
}
