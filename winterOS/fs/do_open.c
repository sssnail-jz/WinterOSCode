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

PRIVATE struct inode *create_file(char *path, int flags);
PRIVATE int alloc_imap_bit(int dev);
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc);
PRIVATE struct inode *new_inode(int dev, int inode_nr, int start_sect);
PRIVATE void new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename);

/**
 * 不管是设备文件还是普通文件，打开它们就要将它们的 inode 加载进来，并建立 ‘三者关联 ’
 * 从文件而言，它们都是相同的，因为它们都是文件
 * 可以多次打开同一文件，只是引用计数增加
 */
PUBLIC int do_open()
{
	int _fd = -1;			  // return value
	char _pathname[MAX_PATH]; // 128

	// get parameters from the message and copy pathname
	int _flags = fs_msg.FLAGS;		 // access mode
	int _name_len = fs_msg.NAME_LEN; // length of filename
	int _src = fs_msg.source;		 // who wnt to open
	assert(_name_len < MAX_PATH);
	phys_copy(_pathname, va2la(_src, fs_msg.PATHNAME), _name_len);
	_pathname[_name_len] = 0;

	// find a free slot in PROCESS::filp[]
	int i;
	for (i = 0; i < NR_FILES; i++)
		if (pcaller->filp[i] == 0)
			break;
	if ((_fd = i) >= NR_FILES)
		panic("filp[] is full (PID:%d)", proc2pid(pcaller));

	// find a free slot in f_desc_table[]
	for (i = 0; i < NR_FILE_DESC; i++)
		if (f_desc_table[i].fd_inode == 0)
			break;
	if (i >= NR_FILE_DESC)
		panic("f_desc_table[] is full (PID:%d)", proc2pid(pcaller));

	// zero or _inode_nr of file
	int _inode_nr = search_file(_pathname);
	struct inode *_pInode = NULL;

	if (_flags & O_CREAT)
	{
		if (_inode_nr)
		{
			if (_flags & O_RDWT) // 如果存在就打开，要不才创建
			{
				char _filename[MAX_FILENAME_LEN];
				struct inode *_pDirInode;
				strip_path(_filename, _pathname, &_pDirInode);
				// _inode_nr 可能为 0，则 _pInode 为 0，并且允许这种情况
				_pInode = get_inode(_pDirInode->i_dev, _inode_nr);
			}
			else // 用户就想创建，但是文件是存在的
			{
				printk("{FS} file exists.\n");
				return -1;
			}
		}
		else
			_pInode = create_file(_pathname, _flags);
	}
	else
	{
		assert(_flags & O_RDWT);

		char _filename[MAX_FILENAME_LEN];
		struct inode *_pDirInode;
		strip_path(_filename, _pathname, &_pDirInode);
		// _inode_nr 可能为 0，则 _pInode 为 0，并且允许这种情况
		_pInode = get_inode(_pDirInode->i_dev, _inode_nr);
	}

	if (_pInode)
	{
		// 联系文件句柄和文件描述符
		pcaller->filp[_fd] = &f_desc_table[i];

		// 联系文件描述符和 inode
		f_desc_table[i].fd_inode = _pInode;
		f_desc_table[i].fd_mode = _flags;
		f_desc_table[i].fd_cnt = 1;
		f_desc_table[i].fd_pos = 0; // 每个新打开此文件的句柄都是从 pos == 0 操作文件的 

		int imode = _pInode->i_mode & I_TYPE_MASK;

		if (imode == I_CHAR_SPECIAL)
		{
			MESSAGE driver_msg;
			driver_msg.type = TTY_OPEN;
			int _dev = _pInode->i_start_sect;
			driver_msg.DEVICE = MINOR(_dev);
			assert(MAJOR(_dev) == 4);
			assert(dd_map[MAJOR(_dev)].driver_nr != INVALID_DRIVER);
			send_recv(BOTH, dd_map[MAJOR(_dev)].driver_nr, &driver_msg);
		}
		else if (imode == I_DIRECTORY)
			assert(_pInode->i_num == ROOT_INODE);
		else
			assert(_pInode->i_mode == I_REGULAR);
	}
	else
		return -1;

	return _fd;
}

/**
 * 1，新建 inode 号
 * 2，新建 smap 号
 * 3，在内存中 inode array 中开辟一项
 * 4，在根目录创建此文件的目录入口
 * imap 是“无限的”，inode array 也是“无限”的，dir_entrys 也是“无限”的，
 * 唯一可能导致分配失败的是 smap 不足
 */
PRIVATE struct inode *create_file(char *path, int flags)
{
	// ah ha ,strip again? just let it go ...
	char _filename[MAX_FILENAME_LEN];
	struct inode *_pDirInode;
	strip_path(_filename, path, &_pDirInode);

	int _inode_nr = alloc_imap_bit(_pDirInode->i_dev);
	int _free_sect_nr = alloc_smap_bit(_pDirInode->i_dev, NR_DEFAULT_FILE_SECTS);
	struct inode *_newino = new_inode(_pDirInode->i_dev, _inode_nr, _free_sect_nr);
	new_dir_entry(_pDirInode, _newino->i_num, _filename);

	return _newino;
}

/**
 * find a free bit in imap
 * set '1' to this bit
 * return this bit's pos 
 */
PRIVATE int alloc_imap_bit(int dev)
{
	int _inode_nr = 0;
	int _iteSector, _iteByte, _iteBit;

	int _imap_blk0_nr = 1 + 1;
	struct super_block *_pSb = get_super_block(dev);

	// ah ha? we just have one sector, so ...
	for (_iteSector = 0; _iteSector < _pSb->nr_imap_sects; _iteSector++)
	{
		RD_SECT(dev, _imap_blk0_nr + _iteSector);
		for (_iteByte = 0; _iteByte < SECTOR_SIZE; _iteByte++)
		{
			if (fsbuf[_iteByte] == 0xFF)
				continue;
			for (_iteBit = 0; ((fsbuf[_iteByte] >> _iteBit) & 1) != 0; _iteBit++)
				__asm__ __volatile__("NOP");

			// actually is just '_iteByte * 8 + _iteBit'
			_inode_nr = (_iteSector * SECTOR_SIZE + _iteByte) * 8 + _iteBit;
			fsbuf[_iteByte] |= (1 << _iteBit);

			WR_SECT(dev, _imap_blk0_nr + _iteSector);
			break;
		}

		return _inode_nr;
	}
	panic("inode-map is probably full.\n");

	return 0;
}

/**
 * find a free bit in smap
 * begin this bit, set '1' for 2048 conter
 * return this bit's pos ( added pos of 1st_sector )
 */
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc)
{
	int _iteSector;
	int _iteByte;
	int _iteBit;

	struct super_block *_pSb = get_super_block(dev);

	int smap_blk0_nr = 1 + 1 + _pSb->nr_imap_sects;
	int free_sect_nr = 0;

	for (_iteSector = 0; _iteSector < _pSb->nr_smap_sects; _iteSector++)
	{
		RD_SECT(dev, smap_blk0_nr + _iteSector);

		// byte offset in current sect
		for (_iteByte = 0; _iteByte < SECTOR_SIZE && nr_sects_to_alloc > 0; _iteByte++)
		{
			_iteBit = 0;
			if (!free_sect_nr)
			{
				// loop until a free bit is found and calculate secNr in 'data arean'
				if (fsbuf[_iteByte] == 0xFF)
					continue;
				for (_iteBit = 0; ((fsbuf[_iteByte] >> _iteBit) & 1) != 0; _iteBit++)
					__asm__ __volatile__("NOP");

				free_sect_nr = (_iteSector * SECTOR_SIZE + _iteByte) * 8 +
							   _iteBit - 1 + _pSb->n_1st_sect;
			}
			while (_iteBit < 8)
			{
				assert(((fsbuf[_iteByte] >> _iteBit) & 1) == 0);
				fsbuf[_iteByte] |= (1 << _iteBit);
				if (--nr_sects_to_alloc == 0)
					break;
				_iteBit++;
			}
		}

		// 找到了就意味着修改了读进来的这个扇区
		if (free_sect_nr)
			WR_SECT(dev, smap_blk0_nr + _iteSector);

		if (nr_sects_to_alloc == 0)
			break;
	}

	// 如果不为 0 ，说明 smap 满了
	assert(nr_sects_to_alloc == 0);
	return free_sect_nr;
}

/**
 * load inode to memory and change inode's info in memeory and disk 
 */
PRIVATE struct inode *new_inode(int dev, int inode_nr, int start_sect)
{
	struct inode *_pNewNode = get_inode(dev, inode_nr);

	// change info in memory
	_pNewNode->i_mode = I_REGULAR; // we can just creat regular file
	_pNewNode->i_size = 0;
	_pNewNode->i_start_sect = start_sect;
	_pNewNode->i_nr_sects = NR_DEFAULT_FILE_SECTS;

	_pNewNode->i_dev = dev;
	_pNewNode->i_cnt = 1;
	_pNewNode->i_num = inode_nr;

	// update info in disk
	sync_inode(_pNewNode);

	return _pNewNode;
}

/**
 * 只在 dirEntrys 的范围内查找，如果找到了，那就重复利用这一个 DirEntr
 * 如果在 dirEntrys 范围内没有找到，那就要新开辟一个了，此时 dir_inode 也要进行更新
 */
PRIVATE void new_dir_entry(struct inode *dir_inode, int inode_nr, char *filename)
{
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE) / SECTOR_SIZE;
	int nr_dir_entries = dir_inode->i_size / DIR_ENTRY_SIZE;
	struct dir_entry *_iterator;
	struct dir_entry *_newDirEty = NULL;

	int i, j;
	for (i = 0; i < nr_dir_blks; i++)
	{
		RD_SECT(dir_inode->i_dev, dir_inode->i_start_sect + i);
		_iterator = (struct dir_entry *)fsbuf;

		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, _iterator++)
		{
			if (!nr_dir_entries--)
				break;

			if (_iterator->inode_nr == 0) // it's a free slot
			{
				_newDirEty = _iterator;
				break;
			}
		}
		if (nr_dir_entries <= 0 || _newDirEty)
			break;
	}
	if (!_newDirEty) // 开辟新的
	{
		_newDirEty = _iterator;
		dir_inode->i_size += DIR_ENTRY_SIZE;
	}
	_newDirEty->inode_nr = inode_nr;
	strcpy(_newDirEty->name, filename);

	WR_SECT(dir_inode->i_dev, dir_inode->i_start_sect + i);
	sync_inode(dir_inode); // oh bad ...
}

PUBLIC int do_close()
{
	int _fd = fs_msg.FD;
	put_inode(pcaller->filp[_fd]->fd_inode);
	if (--pcaller->filp[_fd]->fd_cnt == 0)
		pcaller->filp[_fd]->fd_inode = 0;
	pcaller->filp[_fd] = 0;

	return 0;
}

PUBLIC int do_seek()
{
	int _fd = fs_msg.FD;
	int _off = fs_msg.OFFSET;
	int whence = fs_msg.WHENCE;

	int pos = pcaller->filp[_fd]->fd_pos;
	int f_size = pcaller->filp[_fd]->fd_inode->i_size;

	switch (whence)
	{
	case SEEK_SET:
		pos = _off;
		break;
	case SEEK_CUR:
		pos += _off;
		break;
	case SEEK_END:
		pos = f_size + _off;
		break;
	default:
		panic("seek : flags error!");
		break;
	}
	if ((pos > (NR_DEFAULT_FILE_SECTS << SECTOR_SIZE_SHIFT)) || (pos < 0))
		panic("seek : set pos error !");
	pcaller->filp[_fd]->fd_pos = pos;
	return pos;
}
