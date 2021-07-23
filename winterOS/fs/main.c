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
#include "hd.h"

PRIVATE void init_fs();
PRIVATE void mkfs();
PRIVATE void read_super_block(int dev);
PRIVATE int fs_fork();
PRIVATE int fs_exit();

PUBLIC void task_fs()
{
	init_fs();

	while (TRUE)
	{
		send_recv(RECEIVE, ANY, &fs_msg);

		int msgtype = fs_msg.type;
		int _src = fs_msg.source;
		pcaller = &proc_table[_src];

		switch (msgtype)
		{
		case OPEN:
			fs_msg.FD = do_open();
			break;
		case CLOSE:
			fs_msg.RETVAL = do_close();
			break;
		case READ:
		case WRITE:
			fs_msg.CNT = do_rdwt();
			break;
		case UNLINK:
			fs_msg.RETVAL = do_unlink();
			break;
		case RESUME_PROC:
			_src = fs_msg.PROC_NR;
			break;
		case FORK:
			fs_msg.RETVAL = fs_fork();
			break;
		case EXIT:
			fs_msg.RETVAL = fs_exit();
			break;
		case SEEK:
			fs_msg.OFFSET = do_seek();
			break;
		case STAT:
			fs_msg.RETVAL = do_stat();
			break;
		case FILSS_INFO:
			do_ls();
			break;
		case CLEAR:
			do_clear();
			break;
		default:
			panic("FS::unknown message:");
			assert(0);
			break;
		}

		// reply
		if (fs_msg.type != SUSPEND_PROC)
		{
			fs_msg.type = SYSCALL_RET;
			send_recv(SEND, _src, &fs_msg);
		}
	}
}

PRIVATE void init_fs()
{
	int i;

	// f_desc_table[]
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));

	// inode_table[]
	for (i = 0; i < NR_INODE; i++)
		memset(&inode_table[i], 0, sizeof(struct inode));

	// super_block[]
	struct super_block *_pSb;
	for (_pSb = super_block; _pSb < &super_block[NR_SUPER_BLOCK]; _pSb++)
		_pSb->sb_dev = NO_DEV;

	// open the device: hard disk
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	driver_msg.DEVICE = MINOR(ROOT_DEV);
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);

	printk("\n--make fs begin ...\n\n");
	// 读超级块并且判断文件系统是否已经制作过
	RD_SECT(ROOT_DEV, 1);
	_pSb = (struct super_block *)fsbuf;
	if (_pSb->magic != MAGIC_V1)
		mkfs();
	else
	{
		set_color((u8)(MAKE_COLOR(BLACK, GREEN) | BRIGHT));
		printk("--fs is already ready ...\n");
		set_color((u8)(MAKE_COLOR(BLACK, WHITE)));
	}
	printk("\n--make fs OK ...\n");

	// load super block of ROOT
	read_super_block(ROOT_DEV);
	get_super_block(ROOT_DEV);
	root_inode = get_inode(ROOT_DEV, ROOT_INODE);
}

PRIVATE void mkfs()
{
	MESSAGE driver_msg;
	int i, j;

	struct part_info _rootPartInfo;
	driver_msg.type = DEV_IOCTL;
	driver_msg.DEVICE = MINOR(ROOT_DEV);
	driver_msg.REQUEST = DIOCTL_GET_GEO;
	driver_msg.BUF = &_rootPartInfo;
	driver_msg.PROC_NR = TASK_FS;
	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);

	/**
	 * write the super block 
	 */
	int _bits_sect = SECTOR_SIZE * 8; // 一个扇区多少位
	struct super_block _Sb;
	_Sb.magic = MAGIC_V1;										   // 0x111
	_Sb.nr_inodes = _bits_sect;									   // 1024
	_Sb.nr_inode_sects = _Sb.nr_inodes * INODE_SIZE / SECTOR_SIZE; // 256
	_Sb.nr_sects = _rootPartInfo.size;							   // 40257
	_Sb.nr_imap_sects = 1;
	_Sb.nr_smap_sects = _Sb.nr_sects / _bits_sect + 1;									 // 10
	_Sb.n_1st_sect = 1 + 1 + _Sb.nr_imap_sects + _Sb.nr_smap_sects + _Sb.nr_inode_sects; // 269
	_Sb.root_inode = ROOT_INODE;														 // 第一个的意思（不管是从 inode_table 中还是 imap 中）
	_Sb.inode_size = INODE_SIZE;

	struct inode _tem_inode;
	_Sb.inode_isize_off = (int)&_tem_inode.i_size - (int)&_tem_inode;
	_Sb.inode_start_off = (int)&_tem_inode.i_start_sect - (int)&_tem_inode;
	_Sb.dir_ent_size = DIR_ENTRY_SIZE;
	struct dir_entry _tem_dEty;
	_Sb.dir_ent_inode_off = (int)&_tem_dEty.inode_nr - (int)&_tem_dEty;
	_Sb.dir_ent_fname_off = (int)&_tem_dEty.name - (int)&_tem_dEty;

	memset(fsbuf, 0x90, SECTOR_SIZE);
	memcpy(fsbuf, &_Sb, SUPER_BLOCK_SIZE);

	// write the super block
	WR_SECT(ROOT_DEV, 1);

	// easy .. print FS's layout in hard disk
	printk("\n------------- FS layout -------------\n");
	printk("--address(HEX)\n");
	printk("SuperBlockAddr(Byte):      [0x%x]\n"
		   "imapAddr(Byte):            [0x%x]\n"
		   "smapAddr(Byte):            [0x%x]\n"
		   "inodeAddr(Byte):           [0x%x]\n"
		   "1st_sectorAdrr(Byte):      [0x%x]\n",
		   (_rootPartInfo.base + 1) * 512,
		   (_rootPartInfo.base + 1 + 1) * 512,
		   (_rootPartInfo.base + 1 + 1 + _Sb.nr_imap_sects) * 512,
		   (_rootPartInfo.base + 1 + 1 + _Sb.nr_imap_sects + _Sb.nr_smap_sects) * 512,
		   (_rootPartInfo.base + _Sb.n_1st_sect) * 512);

	printk("\n--size(DEC)\n");
	printk("RootDev's size(sector):    [%8d]\n"
		   "imap's size(sector):       [%8d]\n"
		   "smap's size(sector):       [%8d]\n"
		   "inode's size(sector):      [%8d]\n"
		   "1st_sectorAddr(sector):    [%8d]\n"
		   "inode's count:             [%8d]\n",
		   _rootPartInfo.size,
		   _Sb.nr_imap_sects,
		   _Sb.nr_smap_sects,
		   _Sb.nr_inode_sects,
		   1 + 1 + _Sb.nr_imap_sects + _Sb.nr_smap_sects + _Sb.nr_inode_sects,
		   _Sb.nr_inodes);
	printk("------------- FS layout -------------\n\n");

	/**
	 * write inode map 
	 */
	memset(fsbuf, 0, SECTOR_SIZE);
	// reserved , / , dev_tty0 , dev_tty1 , dev_tty2 , /cmd.tar
	for (i = 0; i < (NR_CONSOLES + 3); i++)
		fsbuf[0] |= 1 << i;
	assert(fsbuf[0] == 0x3F);
	WR_SECT(ROOT_DEV, 2);

	/**
	 * write sector map 
	 */
	memset(fsbuf, 0, SECTOR_SIZE);
	int _nr_sects = NR_DEFAULT_FILE_SECTS + 1; // reserved and '/'
	for (i = 0; i < _nr_sects / 8; i++)		   // asign '1' in byte
		fsbuf[i] = 0xFF;
	for (j = 0; j < _nr_sects % 8; j++) // asign '1' in bit
		fsbuf[i] |= (1 << j);
	WR_SECT(ROOT_DEV, 2 + _Sb.nr_imap_sects); // 512 + 1 还不超过 1 个扇区的比特数

	// zeromemory the rest sector-map
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 1; i < _Sb.nr_smap_sects; i++)
		WR_SECT(ROOT_DEV, 2 + _Sb.nr_imap_sects + i);

	// cmd.tar
	int bit_offset = INSTALL_START_SECT - _Sb.n_1st_sect + 1;
	int bit_off_in_sect = bit_offset % (SECTOR_SIZE * 8);
	int cur_sect = bit_offset / (SECTOR_SIZE * 8);
	int bit_left = INSTALL_NR_SECTS;
	RD_SECT(ROOT_DEV, 2 + _Sb.nr_imap_sects + cur_sect);
	while (bit_left)
	{
		int byte_off = bit_off_in_sect / 8;
		// 每个循环只设置一个比特位 ...
		fsbuf[byte_off] |= 1 << (bit_off_in_sect % 8);
		bit_left--;
		bit_off_in_sect++;
		// 已经写满 smap 中的一个扇区，to next sector in smap
		if (bit_off_in_sect == (SECTOR_SIZE * 8))
		{
			WR_SECT(ROOT_DEV, 2 + _Sb.nr_imap_sects + cur_sect);
			cur_sect++;
			RD_SECT(ROOT_DEV, 2 + _Sb.nr_imap_sects + cur_sect);
			bit_off_in_sect = 0;
		}
	}
	WR_SECT(ROOT_DEV, 2 + _Sb.nr_imap_sects + cur_sect);

	/**
	 * write inode array 
	 */
	// '/'
	memset(fsbuf, 0, SECTOR_SIZE);
	struct inode *_pInode = (struct inode *)fsbuf;
	_pInode->i_mode = I_DIRECTORY;
	_pInode->i_size = DIR_ENTRY_SIZE * 5; // `.',`dev_tty0', `dev_tty1', `dev_tty2',`cmd.tar'
	_pInode->i_start_sect = _Sb.n_1st_sect;
	_pInode->i_nr_sects = NR_DEFAULT_FILE_SECTS;
	// inode of `/dev_tty0~2'
	for (i = 0; i < NR_CONSOLES; i++)
	{
		_pInode = (struct inode *)(fsbuf + (INODE_SIZE * (i + 1)));
		_pInode->i_mode = I_CHAR_SPECIAL;
		_pInode->i_size = 0;
		_pInode->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i); // careful
		_pInode->i_nr_sects = 0;
	}
	// inode of `/cmd.tar'
	_pInode = (struct inode *)(fsbuf + (INODE_SIZE * (NR_CONSOLES + 1)));
	_pInode->i_mode = I_REGULAR;
	_pInode->i_size = INSTALL_NR_SECTS * SECTOR_SIZE;
	_pInode->i_start_sect = INSTALL_START_SECT;
	_pInode->i_nr_sects = INSTALL_NR_SECTS;
	WR_SECT(ROOT_DEV, 2 + _Sb.nr_imap_sects + _Sb.nr_smap_sects); // 5 个 inode 不会超过一个扇区

	/**
	 * write dir entrys of '/'
	 */
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry *_pDEty = (struct dir_entry *)fsbuf;
	_pDEty->inode_nr = 1;
	strcpy(_pDEty->name, ".");
	// dir entries of `/dev_tty0~2'
	for (i = 0; i < NR_CONSOLES; i++)
	{
		_pDEty++;
		_pDEty->inode_nr = i + 2; // dev_tty0's inode_nr is 2
		sprintf(_pDEty->name, "dev_tty%d", i);
	}
	(++_pDEty)->inode_nr = NR_CONSOLES + 2;
	sprintf(_pDEty->name, "cmd.tar", i);
	WR_SECT(ROOT_DEV, _Sb.n_1st_sect);
}

PUBLIC int rw_sector(int io_type, int dev, u64 pos, int bytes,
					 int proc_nr, void *buf)
{
	MESSAGE driver_msg;

	driver_msg.type = io_type;
	driver_msg.DEVICE = MINOR(dev);
	driver_msg.pos_byte = pos;
	driver_msg.BUF = buf;
	driver_msg.CNT = bytes;
	driver_msg.PROC_NR = proc_nr;
	assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);

	return 0;
}

PRIVATE void read_super_block(int dev)
{
	int i;
	MESSAGE driver_msg;

	driver_msg.type = DEV_READ;
	driver_msg.DEVICE = MINOR(dev);
	driver_msg.pos_byte = SECTOR_SIZE * 1;
	driver_msg.BUF = fsbuf;
	driver_msg.CNT = SECTOR_SIZE;
	driver_msg.PROC_NR = TASK_FS;
	assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
	send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);

	// actually ,just one ...
	for (i = 0; i < NR_SUPER_BLOCK; i++)
		if (super_block[i].sb_dev == NO_DEV)
			break;
	if (i == NR_SUPER_BLOCK)
		panic("super_block slots used up");

	// 内存中的 sb 结构体比磁盘中的多成员
	super_block[i] = *(struct super_block *)fsbuf;
	super_block[i].sb_dev = dev; // 只存在于内存中
}

PUBLIC struct super_block *get_super_block(int dev)
{
	struct super_block *_pSb;
	for (_pSb = super_block; _pSb < &super_block[NR_SUPER_BLOCK]; _pSb++)
		if (_pSb->sb_dev == dev)
			return _pSb;

	panic("super block of devie %d not found.\n", dev);
	return NULL;
}

/**
 * imap 中的号与 inode_array(disk) 中项的对应
 */
PUBLIC struct inode *get_inode(int dev, int num)
{
	if (num == 0)
		return 0;

	struct inode *_iterator;
	struct inode *_pRecordFree = NULL;
	for (_iterator = &inode_table[0]; _iterator < &inode_table[NR_INODE]; _iterator++)
	{
		if (_iterator->i_cnt) // whitch is not free
		{
			// dev,number is set when creat the inode
			if ((_iterator->i_dev == dev) && (_iterator->i_num == num))
			{
				_iterator->i_cnt++;
				return _iterator;
			}
		}
		else
		{
			_pRecordFree = _iterator;
			break;
		}
	}

	if (!_pRecordFree)
		panic("the inode table is full");

	// dev,number is set when creat inode
	_pRecordFree->i_dev = dev;
	_pRecordFree->i_num = num;
	_pRecordFree->i_cnt = 1;

	struct super_block *_pSb = get_super_block(dev);
	int _blk_nr = 1 + 1 + _pSb->nr_imap_sects + _pSb->nr_smap_sects +
				  ((num - 1) / (SECTOR_SIZE / INODE_SIZE));
	RD_SECT(dev, _blk_nr);
	struct inode *_pInode = (struct inode *)((u8 *)fsbuf + ((num - 1) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE);
	// yes we readed it ...
	_pRecordFree->i_mode = _pInode->i_mode;
	_pRecordFree->i_size = _pInode->i_size;
	_pRecordFree->i_start_sect = _pInode->i_start_sect;
	_pRecordFree->i_nr_sects = _pInode->i_nr_sects;
	return _pRecordFree;
}

PUBLIC void put_inode(struct inode *pinode)
{
	assert(pinode->i_cnt > 0);
	pinode->i_cnt--;
}

PUBLIC void sync_inode(struct inode *p)
{
	struct inode *_pInode;
	struct super_block *_pSb = get_super_block(p->i_dev);
	int _blk_nr = 1 + 1 + _pSb->nr_imap_sects + _pSb->nr_smap_sects +
				  ((p->i_num - 1) / (SECTOR_SIZE / INODE_SIZE));
	RD_SECT(p->i_dev, _blk_nr);
	_pInode = (struct inode *)((u8 *)fsbuf +
							   (((p->i_num - 1) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE));
	_pInode->i_mode = p->i_mode;
	_pInode->i_size = p->i_size;
	_pInode->i_start_sect = p->i_start_sect;
	_pInode->i_nr_sects = p->i_nr_sects;
	WR_SECT(p->i_dev, _blk_nr);
}

/**
 * inc 'fd_cnt' and 'i_cnt' after copy 
 */
PRIVATE int fs_fork()
{
	int i;
	struct proc *_child = &proc_table[fs_msg.PID];
	for (i = 0; i < NR_FILES; i++)
	{
		if (_child->filp[i])
		{
			_child->filp[i]->fd_cnt++;
			_child->filp[i]->fd_inode->i_cnt++;
		}
	}

	return 0;
}

PRIVATE int fs_exit()
{
	int i;
	struct proc *p = &proc_table[fs_msg.PID];
	for (i = 0; i < NR_FILES; i++)
	{
		if (p->filp[i])
		{
			p->filp[i]->fd_inode->i_cnt--;
			if (--p->filp[i]->fd_cnt == 0)
				p->filp[i]->fd_inode = 0;
			p->filp[i] = 0;
		}
	}
	return 0;
}
