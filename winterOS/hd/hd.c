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
#include "hd.h"
/**
 * so 'partition' and 'get_partition_tale' i am not familiar
 * but let it go just 
 */
PRIVATE void init_hd();
PRIVATE void hd_open(int device);
PRIVATE void hd_close(int device);
PRIVATE void hd_rdwt(MESSAGE *p);
PRIVATE void hd_ioctl(MESSAGE *p);
PRIVATE void hd_cmd_out(struct hd_cmd *cmd);
PRIVATE void get_part_table(int drive, int sect_nr, struct part_ent *entry);
PRIVATE void partition(int device, int style);
PRIVATE void print_hdinfo(struct hd_info *hdi);
PRIVATE int waitfor(int mask, int val, int timeout);
PRIVATE void interrupt_wait();
PRIVATE void hd_identify(int drive);
PRIVATE void print_identify_info(u16 *hdinfo);

PRIVATE u8 hd_status;
PRIVATE u8 hdbuf[SECTOR_SIZE * 2];
PRIVATE struct hd_info hd_info[1];

#define DRV_OF_DEV(dev) \
	(dev <= MAX_PRIM ? dev / NR_PRIM_PER_DRIVE : (dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)

PUBLIC void task_hd()
{
	MESSAGE msg;
	init_hd();

	while (TRUE)
	{
		send_recv(RECEIVE, ANY, &msg);

		int src = msg.source;

		switch (msg.type)
		{
		case DEV_OPEN:
			hd_open(msg.DEVICE);
			break;

		case DEV_CLOSE:
			hd_close(msg.DEVICE);
			break;

		case DEV_READ:
		case DEV_WRITE:
			hd_rdwt(&msg);
			break;

		case DEV_IOCTL:
			hd_ioctl(&msg);
			break;

		default:
			panic("HD driver::unknown msg");
			spin("FS::main_loop (invalid msg.type)");
			break;
		}

		msg.type = SYSCALL_RET;
		send_recv(SEND, src, &msg);
	}
}

PRIVATE void init_hd()
{
	int i;

	// 硬盘中断处理函数的作用就是数据准备好了通知请求数据的任务（FS）
	put_irq_handler(AT_WINI_IRQ, hd_handler);
	enable_irq(CASCADE_IRQ); // 主片的级联中断
	enable_irq(AT_WINI_IRQ);

	for (i = 0; i < (sizeof(hd_info) / sizeof(hd_info[0])); i++)
		memset(&hd_info[i], 0, sizeof(hd_info[0]));
	hd_info[0].open_cnt = 0;
}

PRIVATE void hd_open(int device)
{
	int drive = DRV_OF_DEV(device);
	assert(drive == 0);

	// identify 'HD' and print some fundamental info
	hd_identify(drive);

	if (hd_info[drive].open_cnt++ == 0)
	{
		// record 'partition' info
		partition(drive * (NR_PART_PER_DRIVE + 1), P_PRIMARY);
		print_hdinfo(&hd_info[drive]);
	}
}

PRIVATE void hd_close(int device)
{
	int drive = DRV_OF_DEV(device);
	assert(drive == 0);
	hd_info[drive].open_cnt--;
}

/**
 * Message 中的 pos (in byte) ：512 字节对齐、相对于某个分区的 
 * Message 中的 CNT (in byte) ：512 字节对齐
 * 将 in byte 转化为 LBA
 * Message 中的 DEVICE 可以用来转化为相对于整块磁盘的 LBA
 */
PRIVATE void hd_rdwt(MESSAGE *p)
{
	int drive = DRV_OF_DEV(p->DEVICE);

	u64 _pos_byte = p->pos_byte;
	// 1 << 31 equal 2048 sectors (1 M)
	// 保证一次读取的扇区数不超过一个文件占用的最大扇区数
	assert((_pos_byte >> SECTOR_SIZE_SHIFT) < (1 << 31));
	// 保证 _pos_byte(in byte) 是以 512 字节对齐的
	assert((_pos_byte & 0x1FF) == 0);

	// transform _pos_byte(in byte) to _pos_sector(in sector)
	u32 _pos_sector = (u32)(_pos_byte >> SECTOR_SIZE_SHIFT);
	int _subPartNr = (p->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE;
	// _pos_sector 转化为基于整个磁盘的 LBA
	_pos_sector += p->DEVICE < MAX_PRIM ? hd_info[drive].primary[p->DEVICE].base
										: hd_info[drive].logical[_subPartNr].base;

	struct hd_cmd cmd;
	cmd.features = 0;
	// p->CNT 也是 in byte，但是 512 byte 对齐
	cmd.count = (p->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
	cmd.lba_low = _pos_sector & 0xFF;
	cmd.lba_mid = (_pos_sector >> 8) & 0xFF;
	cmd.lba_high = (_pos_sector >> 16) & 0xFF;
	cmd.device = MAKE_DEVICE_REG(1, drive, (_pos_sector >> 24) & 0xF);
	cmd.command = (p->type == DEV_READ) ? ATA_READ : ATA_WRITE;

	// so there is easy ...
	hd_cmd_out(&cmd);

	// then 'HD' will put data to some port
	// maybe 1 sectors or 2 sectors or more ...
	// who care :)
	int _sector_left = p->CNT >> SECTOR_SIZE_SHIFT;
	void *_BufLa = (void *)va2la(p->PROC_NR, p->BUF);

	while (_sector_left--)
	{
		if (p->type == DEV_READ)
		{
			interrupt_wait();
			// so . read and copy data (in sector), it is easy ...
			port_read(REG_DATA, hdbuf, SECTOR_SIZE);
			phys_copy(_BufLa, hdbuf, SECTOR_SIZE);
		}
		else
		{
			if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
				panic("wait timeout when write 'HD' :(");

			port_write(REG_DATA, _BufLa, SECTOR_SIZE);
			interrupt_wait();
		}
		_BufLa += SECTOR_SIZE;
	}
}

PRIVATE void hd_ioctl(MESSAGE *p)
{
	int _device = p->DEVICE;
	struct hd_info *_hdi = &hd_info[DRV_OF_DEV(_device)];
	if (p->REQUEST & DIOCTL_GET_GEO)
	{
		phys_copy(va2la(p->PROC_NR, p->BUF),
				  _device < MAX_PRIM ? &_hdi->primary[_device]
									 : &_hdi->logical[(_device - MINOR_hd1a) % NR_SUB_PER_DRIVE],
				  sizeof(struct part_info));
	}
	else
		assert(p->REQUEST == DIOCTL_GET_GEO);
}

PRIVATE void get_part_table(int drive, int sect_nr, struct part_ent *entry)
{
	struct hd_cmd cmd;
	cmd.features = 0;
	cmd.count = 1;
	cmd.lba_low = sect_nr & 0xFF;
	cmd.lba_mid = (sect_nr >> 8) & 0xFF;
	cmd.lba_high = (sect_nr >> 16) & 0xFF;
	cmd.device = MAKE_DEVICE_REG(1, /* LBA mode*/
								 drive,
								 (sect_nr >> 24) & 0xF);
	cmd.command = ATA_READ;
	hd_cmd_out(&cmd);
	interrupt_wait();

	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy(entry,
		   hdbuf + PARTITION_TABLE_OFFSET,
		   sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}

PRIVATE void partition(int device, int style)
{
	int i;
	int drive = DRV_OF_DEV(device);
	struct hd_info *hdi = &hd_info[drive];
	struct part_ent part_tbl[NR_SUB_PER_DRIVE];

	if (style == P_PRIMARY)
	{
		get_part_table(drive, drive, part_tbl);

		int nr_prim_parts = 0;
		for (i = 0; i < NR_PART_PER_DRIVE; i++)
		{ /* 0~3 */
			if (part_tbl[i].sys_id == NO_PART)
				continue;

			nr_prim_parts++;
			int dev_nr = i + 1; /* 1~4 */
			hdi->primary[dev_nr].base = part_tbl[i].start_sect;
			hdi->primary[dev_nr].size = part_tbl[i].nr_sects;
			hdi->primary[dev_nr].boot_ind = part_tbl[i].boot_ind;
			hdi->primary[dev_nr].sys_id = part_tbl[i].sys_id;

			if (part_tbl[i].sys_id == EXT_PART) /* extended */
				partition(device + dev_nr, P_EXTENDED);
		}
		assert(nr_prim_parts != 0);
	}
	else if (style == P_EXTENDED)
	{
		int j = device % NR_PRIM_PER_DRIVE; /* 1~4 */
		int ext_start_sect = hdi->primary[j].base;
		int s = ext_start_sect;
		int nr_1st_sub = (j - 1) * NR_SUB_PER_PART; /* 0/16/32/48 */

		for (i = 0; i < NR_SUB_PER_PART; i++)
		{
			int dev_nr = nr_1st_sub + i; /* 0~15/16~31/32~47/48~63 */
			get_part_table(drive, s, part_tbl);

			hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
			hdi->logical[dev_nr].size = part_tbl[0].nr_sects;
			hdi->logical[dev_nr].boot_ind = part_tbl[0].boot_ind;
			hdi->logical[dev_nr].sys_id = part_tbl[0].sys_id;

			s = ext_start_sect + part_tbl[1].start_sect;

			/* no more logical partitions
			   in this extended partition */
			if (part_tbl[1].sys_id == NO_PART)
				break;
		}
	}
	else
		assert(0);
}

PRIVATE void print_hdinfo(struct hd_info *hdi)
{
	printk("\n----------- partition infor ------------\n");
	int i;
	for (i = 0; i < NR_PART_PER_DRIVE + 1; i++)
	{
		printk("PART_%d: base  [%6x]   size    [%5x]%s\n"
			   "        sys_id[%6x]   boot_ind[%5s]\n",
			   i,
			   hdi->primary[i].base,
			   hdi->primary[i].size,
			   i == 0 ? "  (total)" : "",
			   hdi->primary[i].sys_id,
			   hdi->primary[i].boot_ind == 0x80 ? "yes" : " no");
	}
	for (i = 0; i < NR_SUB_PER_DRIVE; i++)
	{
		if (hdi->logical[i].size == 0)
			continue;
		printk("    %d: base  [%6x]   size    [%5x]%s\n"
			   "        sys_id[%6x]   boot_ind[%5s]\n",
			   i,
			   hdi->logical[i].base,
			   hdi->logical[i].size,
			   i == 16 ? "  (ROOT_DEV)" : "",
			   hdi->logical[i].sys_id,
			   hdi->logical[i].boot_ind == 0x80 ? "yes" : " no");
	}
	printk("----------- partition infor ------------\n\n");
}

PRIVATE void hd_identify(int drive)
{
	struct hd_cmd cmd;
	cmd.device = MAKE_DEVICE_REG(0, drive, 0);
	cmd.command = ATA_IDENTIFY;
	cmd.features = 0;
	cmd.count = 0;
	cmd.lba_low = 0;
	cmd.lba_mid = 0;
	cmd.lba_high = 0;
	hd_cmd_out(&cmd);

	interrupt_wait(); // good

	// ok, data is ready for well
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	print_identify_info((u16 *)hdbuf);

	u16 *hdinfo = (u16 *)hdbuf;

	hd_info[drive].primary[0].base = 0;
	hd_info[drive].primary[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
}

PRIVATE void print_identify_info(u16 *hdinfo)
{
	printk("----------- HD infor -----------\n");
	u8 *pNrDrives = (u8 *)(0x475);
	printk("%s[%d]\n", "DrivesCnt(BIOS)   ", *pNrDrives);
	assert(*pNrDrives);

	int i, k;
	char s[64];
	struct iden_info_ascii
	{
		int idx;
		int len;
		char *desc;
	} iinfo[] = {
		{10, 20, "SerialNr          "}, /* 序列号 （ASCII）*/
		{27, 12, "ModelNr           "}, /* 型号（ASCII进行了截断） */
	};
	for (k = 0; k < sizeof(iinfo) / sizeof(iinfo[0]); k++)
	{
		char *p = (char *)&hdinfo[iinfo[k].idx];
		for (i = 0; i < iinfo[k].len / 2; i++)
		{
			s[i * 2 + 1] = *p++;
			s[i * 2] = *p++;
		}
		s[i * 2] = 0;
		printk("%s[%s]\n", iinfo[k].desc, s);
	}
	printk("Valid SectorsNr   [%d](%dM)\n",
		   ((int)hdinfo[61] << 16) + hdinfo[60],
		   (int)(((int)hdinfo[61] << 16) + hdinfo[60]) * 512 / 1000000);
	printk("LBA supported     [%s]]\n", (int)(hdinfo[49] & 0x0200) ? "Yes" : "No");
	printk("LBA48 supported   [%s]\n", ((int)hdinfo[83] & 0x0400) ? "Yes" : "No");
	printk("----------- HD infor -----------\n");
}

PRIVATE void hd_cmd_out(struct hd_cmd *cmd)
{

	if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT))
		panic("HD waitfor status YIMEOUT!!!");

	/* Activate the Interrupt Enable (nIEN) bit */
	out_byte(REG_DEV_CTRL, 0);

	/* Load required parameters in the Command Block Registers */
	out_byte(REG_FEATURES, cmd->features);
	out_byte(REG_NSECTOR, cmd->count);
	out_byte(REG_LBA_LOW, cmd->lba_low);
	out_byte(REG_LBA_MID, cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEVICE, cmd->device);

	/* Write the command code to the Command Register */
	out_byte(REG_CMD, cmd->command);
}

PRIVATE void interrupt_wait()
{
	MESSAGE msg;
	send_recv(RECEIVE, INTERRUPT, &msg);
}

// 读 REG_STATUS 寄存器看 mask 指定的位是否值为 val
PRIVATE int waitfor(int mask, int val, int timeout)
{
	int t = get_ticks();
	while (((get_ticks() - t) * 1000 / HZ) < timeout)
		if ((in_byte(REG_STATUS) & mask) == val)
			return TRUE;
	return FALSE;
}

PUBLIC void hd_handler(int irq)
{

	hd_status = in_byte(REG_STATUS);
	inform_int(TASK_HD);
}
