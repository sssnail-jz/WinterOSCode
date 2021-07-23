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

/**
 * if 'TTY file', we send msg
 * if 'regulare file', we dont allow RD/WT out of file bound 
 */
PUBLIC int do_rdwt()
{
	int _fd = fs_msg.FD;
	void *_procBuf = fs_msg.BUF;
	int _len = fs_msg.CNT;
	int _src = fs_msg.source;

	assert((pcaller->filp[_fd] >= &f_desc_table[0]) &&
		   (pcaller->filp[_fd] < &f_desc_table[NR_FILE_DESC]));

	if (!(pcaller->filp[_fd]->fd_mode & O_RDWT))
		return 0;

	int _pos = pcaller->filp[_fd]->fd_pos;
	struct inode *_pInode = pcaller->filp[_fd]->fd_inode;
	assert(_pInode >= &inode_table[0] && _pInode < &inode_table[NR_INODE]);
	int _imode = _pInode->i_mode & I_TYPE_MASK;

	if (_imode == I_CHAR_SPECIAL) // OK, if 'TTY file', send the msg to 'TTY'
	{
		fs_msg.type = fs_msg.type == READ ? TTY_READ : TTY_WRITE;

		int _dev = _pInode->i_start_sect;
		assert(MAJOR(_dev) == 4);

		fs_msg.DEVICE = MINOR(_dev);
		fs_msg.BUF = _procBuf;
		fs_msg.CNT = _len;
		fs_msg.PROC_NR = _src;
		assert(dd_map[MAJOR(_dev)].driver_nr != INVALID_DRIVER);
		send_recv(BOTH, dd_map[MAJOR(_dev)].driver_nr, &fs_msg);
		assert(fs_msg.CNT == _len);

		return fs_msg.CNT;
	}
	else
	{
		assert(_pInode->i_mode == I_REGULAR || _pInode->i_mode == I_DIRECTORY);
		assert((fs_msg.type == READ) || (fs_msg.type == WRITE));

		// we dont allow RD/WT out of file size bound
		if (_pos + _len > NR_DEFAULT_FILE_SECTS << SECTOR_SIZE_SHIFT)
			panic("rd/wt out of file size!!!");

		int _pos_end;
		if (fs_msg.type == READ)
			_pos_end = min(_pos + _len, _pInode->i_size);
		else // WRITE
			_pos_end = _pos + _len;

		int _off = _pos % SECTOR_SIZE;
		int rw_sect_min = _pInode->i_start_sect + (_pos >> SECTOR_SIZE_SHIFT);
		int rw_sect_max = _pInode->i_start_sect + (_pos_end >> SECTOR_SIZE_SHIFT);

		// so ... we read all sectors in one time(no loop)
		int _chunk = rw_sect_max - rw_sect_min + 1;
		int _byteRdWr = _pos_end - _pos; // oh ha? _byteRdWr maybe != _len when read
		rw_sector(DEV_READ, _pInode->i_dev, rw_sect_min * SECTOR_SIZE,
				  _chunk * SECTOR_SIZE, TASK_FS, fsbuf);

		if (fs_msg.type == READ)
			phys_copy((void *)va2la(_src, _procBuf), fsbuf + _off, _byteRdWr);
		else
		{
			phys_copy(fsbuf + _off, (void *)va2la(_src, _procBuf), _byteRdWr);
			rw_sector(DEV_WRITE, _pInode->i_dev, rw_sect_min * SECTOR_SIZE,
					  _chunk * SECTOR_SIZE, TASK_FS, fsbuf);
		}

		pcaller->filp[_fd]->fd_pos += _byteRdWr; // OK, we update the pos!
		if (pcaller->filp[_fd]->fd_pos > _pInode->i_size)
		{
			// update inode
			_pInode->i_size = pcaller->filp[_fd]->fd_pos;
			sync_inode(_pInode);
		}

		return _byteRdWr;
	}
}
