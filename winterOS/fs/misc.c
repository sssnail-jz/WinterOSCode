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
#include "hd.h"
#include "fs.h"

PUBLIC int search_file(char *path)
{
	char _filename[MAX_FILENAME_LEN];
	memset(_filename, 0, MAX_FILENAME_LEN);
	struct inode *_pDirInode;
	strip_path(_filename, path, &_pDirInode);

	if (_filename[0] == 0)		  // path: "/"
		return _pDirInode->i_num; // '1'

	// search dir entry table
	int nr_dir_blks = (_pDirInode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nr_dir_entries = _pDirInode->i_size / DIR_ENTRY_SIZE;
	struct dir_entry *_iterator;
	int i, j;
	for (i = 0; i < nr_dir_blks; i++)
	{
		// 其实读数据区开始第一个扇区
		RD_SECT(_pDirInode->i_dev, _pDirInode->i_start_sect + i);
		_iterator = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, _iterator++)
		{
			if (strcmp(_filename, _iterator->name) == 0)
				return _iterator->inode_nr; // OK, catch he's inode_number
			if (--nr_dir_entries <= 0)
				goto noFile;
		}
	}

noFile:
	return 0;
}

PUBLIC int strip_path(char *filename, const char *pathname, struct inode **ppinode)
{
	const char *_s = pathname;
	char *_t = filename;
	assert(pathname != NULL);

	if (*_s == '/')
		_s++;

	while (*_s)
	{
		if (*_s == '/')
			panic("file name can'_t contain '/'!");
		*_t++ = *_s++;
		if (_t - filename >= MAX_FILENAME_LEN)
			break;
	}
	*_t = 0;

	// oh! we have just one dirNode is '/'
	// and root_inode is loaded in init fs
	*ppinode = root_inode;
	return 0;
}

PUBLIC int do_stat()
{
	char pathname[MAX_PATH]; /* parameter from the caller */
	char filename[MAX_PATH]; /* directory has been stipped */

	/* get parameters from the message */
	int name_len = fs_msg.NAME_LEN; /* length of filename */
	int src = fs_msg.source;		/* caller proc nr. */
	assert(name_len < MAX_PATH);
	phys_copy((void *)va2la(TASK_FS, pathname),	/* to   */
			  (void *)va2la(src, fs_msg.PATHNAME), /* from */
			  name_len);
	pathname[name_len] = 0; /* terminate the string */

	int inode_nr = search_file(pathname);
	if (inode_nr == INVALID_INODE)
	{ /* file not found */
		printk("{FS} FS::do_stat():: search_file() returns "
			   "invalid inode: %s\n",
			   pathname);
		return -1;
	}

	struct inode *pin = 0;

	struct inode *_pDirInode;
	if (strip_path(filename, pathname, &_pDirInode) != 0)
	{
		/* theoretically never fail here
		 * (it would have failed earlier when
		 *  search_file() was called)
		 */
		assert(0);
	}
	pin = get_inode(_pDirInode->i_dev, inode_nr);

	struct stat s; /* the thing requested */
	s.st_dev = pin->i_dev;
	s.st_ino = pin->i_num;
	s.st_mode = pin->i_mode;
	s.st_rdev = is_special(pin->i_mode) ? pin->i_start_sect : NO_DEV;
	s.st_size = pin->i_size;

	put_inode(pin);

	phys_copy((void *)va2la(src, fs_msg.BUF), /* to   */
			  (void *)va2la(TASK_FS, &s),	 /* from */
			  sizeof(struct stat));

	return 0;
}

PUBLIC void do_ls()
{
	struct file_info _fileInfo;
	struct inode *_pInode;
	struct inode *_pDirInode = root_inode;
	int nr_dir_blks = (_pDirInode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nr_dir_entries = _pDirInode->i_size / DIR_ENTRY_SIZE;
	struct dir_entry *_iterator;
	char _buf[SECTOR_SIZE];
	int i, j;
	int _cnt = 0;
	for (i = 0; i < nr_dir_blks; i++)
	{
		// usually just one or two sectors ...
		RD_SECT(_pDirInode->i_dev, _pDirInode->i_start_sect + i);
		// we must do this, because 'get_inode' will dirty the fsbuf
		memcpy(_buf, fsbuf, SECTOR_SIZE);
		_iterator = (struct dir_entry *)_buf;

		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, _iterator++)
		{
			if (_iterator->inode_nr != 0)
			{
				_pInode = get_inode(_pDirInode->i_dev, _iterator->inode_nr);
				assert(_pInode != NULL);

				_fileInfo.type = _pInode->i_mode;
				_fileInfo.inode_nr = _iterator->inode_nr;
				_fileInfo.size = _pInode->i_size;
				_fileInfo.ref_cnt = _pInode->i_cnt - 1; // care
				strcpy(_fileInfo.name, _iterator->name);
				phys_copy((void *)va2la(fs_msg.source, (struct file_info *)fs_msg.BUF + _cnt++),
						  &_fileInfo, sizeof(struct file_info));

				put_inode(_pInode);
			}
			if (--nr_dir_entries == 0)
			{
				fs_msg.CNT = _cnt;
				return;
			}
		}
	}
}

PUBLIC void do_clear()
{
	int _fd = fs_msg.FD;
	int _imode = pcaller->filp[_fd]->fd_inode->i_mode;
	if (_imode == I_CHAR_SPECIAL)
	{
		int _dev = pcaller->filp[_fd]->fd_inode->i_start_sect;
		assert(MAJOR(_dev) == 4);
		fs_msg.DEVICE = MINOR(_dev);
		fs_msg.type = TTY_CLEAR;
		assert(dd_map[MAJOR(_dev)].driver_nr != INVALID_DRIVER);
		send_recv(BOTH, dd_map[MAJOR(_dev)].driver_nr, &fs_msg);
		// OK, if work well, 'TTY' will clean the console ...
	}
	else
	{
		assert(pcaller->filp[_fd]->fd_inode->i_mode == I_REGULAR);
		assert(pcaller->filp[_fd]->fd_inode->i_mode != I_DIRECTORY);
		int _sectors = pcaller->filp[_fd]->fd_inode->i_size / 512 + 1;
		phys_set(fsbuf, 0, _sectors);
		rw_sector(DEV_WRITE, pcaller->filp[_fd]->fd_inode->i_dev,
				  pcaller->filp[_fd]->fd_inode->i_start_sect * SECTOR_SIZE,
				  _sectors << SECTOR_SIZE_SHIFT, TASK_FS, fsbuf);
		pcaller->filp[_fd]->fd_inode->i_size = 0;
		pcaller->filp[_fd]->fd_pos = 0;
		sync_inode(pcaller->filp[_fd]->fd_inode);
	}
}
