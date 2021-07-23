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

PUBLIC int do_unlink()
{
	char _pathname[MAX_PATH];

	// get parameters from the message
	int _nameLen = fs_msg.NAME_LEN;
	assert(_nameLen < MAX_PATH);
	phys_copy(_pathname, (void *)va2la(fs_msg.source, fs_msg.PATHNAME), _nameLen);
	_pathname[_nameLen] = 0;

	if (strcmp(_pathname, "/") == 0)
	{
		printk("{FS} FS:do_unlink():: cannot unlink the root\n");
		return -1;
	}

	int _inodeNr = search_file(_pathname);
	if (_inodeNr == INVALID_INODE)
	{
		printk("{FS} FS::do_unlink():: search_file() returns "
			   "invalid inode: %s\n",
			   _pathname);
		return -1;
	}

	// ok, the fill is exit
	char _filename[MAX_FILENAME_LEN];
	struct inode *_dirInode;
	strip_path(_filename, _pathname, &_dirInode); // just get root dir's inode

	// OK, the file maybe open just one time or many times or not be open
	struct inode *_pInode = get_inode(_dirInode->i_dev, _inodeNr);

	if (_pInode->i_mode != I_REGULAR)
	{
		printk("{FS} cannot remove file %s, because "
			   "it is not a regular file.\n",
			   _pathname);
		return -1;
	}

	if (_pInode->i_cnt > 1)
	{
		printk("{FS} cannot remove file %s, because _pInode->i_cnt is %d.\n",
			   _pathname, _pInode->i_cnt);
		return -1;
	}

	// OK, now we can delete(unlike)
	struct super_block *_pSb = get_super_block(_pInode->i_dev);

	/**
	 * free the bit in i-map
	 */
	int _byte_idx = _inodeNr / 8;
	int _bit_idx = _inodeNr % 8;
	assert(_byte_idx < SECTOR_SIZE);
	RD_SECT(_pInode->i_dev, 2);
	assert(fsbuf[_byte_idx] & (1 << _bit_idx));
	fsbuf[_byte_idx] &= ~(1 << _bit_idx);
	WR_SECT(_pInode->i_dev, 2);

	/**
	 * free the bit in s-map
	 */
	_bit_idx = _pInode->i_start_sect - _pSb->n_1st_sect + 1;
	_byte_idx = _bit_idx / 8;
	int _bits_left = _pInode->i_nr_sects;
	int _byte_cnt = (_bits_left - (8 - (_bit_idx % 8))) / 8;

	// current sector nr.
	int _s = 2 + _pSb->nr_imap_sects + _byte_idx / SECTOR_SIZE;
	RD_SECT(_pInode->i_dev, _s);

	// clear the first byte
	int i;
	for (i = _bit_idx % 8; (i < 8) && _bits_left; i++, _bits_left--)
	{
		assert((fsbuf[_byte_idx % SECTOR_SIZE] >> i & 1) == 1);
		fsbuf[_byte_idx % SECTOR_SIZE] &= ~(1 << i);
	}

	// clear bytes from the second byte to the second to last
	int k;
	i = (_byte_idx % SECTOR_SIZE) + 1; /* the second byte */
	for (k = 0; k < _byte_cnt; k++, i++, _bits_left -= 8)
	{
		if (i == SECTOR_SIZE)
		{
			i = 0;
			WR_SECT(_pInode->i_dev, _s);
			RD_SECT(_pInode->i_dev, ++_s);
		}
		assert(fsbuf[i] == 0xFF);
		fsbuf[i] = 0;
	}

	// clear the last byte
	if (i == SECTOR_SIZE)
	{
		i = 0;
		WR_SECT(_pInode->i_dev, _s);
		RD_SECT(_pInode->i_dev, ++_s);
	}
	unsigned char _mask = ~((unsigned char)(~0) << _bits_left);
	assert((fsbuf[i] & _mask) == _mask);
	fsbuf[i] &= (~0) << _bits_left;
	WR_SECT(_pInode->i_dev, _s);

	/**
	 * clear the i-node itself
	 */
	_pInode->i_mode = 0;
	_pInode->i_size = 0;
	_pInode->i_start_sect = 0;
	_pInode->i_nr_sects = 0;
	sync_inode(_pInode);
	// release slot in inode_table[]
	put_inode(_pInode);

	/**
	 * set the inode-nr to 0 in the directory entry 
	 */
	int dir_blk0_nr = _dirInode->i_start_sect;
	int nr_dir_blks = (_dirInode->i_size + SECTOR_SIZE) / SECTOR_SIZE;
	int nr_dir_entries = _dirInode->i_size / DIR_ENTRY_SIZE;
	int m = 0;
	struct dir_entry *pde = 0;
	int flg = 0;
	int dir_size = 0;

	for (i = 0; i < nr_dir_blks; i++)
	{
		RD_SECT(_dirInode->i_dev, dir_blk0_nr + i);

		pde = (struct dir_entry *)fsbuf;
		int j;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++)
		{
			if (++m > nr_dir_entries)
				break;

			if (pde->inode_nr == _inodeNr)
			{
				/* pde->inode_nr = 0; */
				memset(pde, 0, DIR_ENTRY_SIZE);
				WR_SECT(_dirInode->i_dev, dir_blk0_nr + i);
				flg = 1;
				break;
			}

			if (pde->inode_nr != INVALID_INODE)
				dir_size += DIR_ENTRY_SIZE;
		}

		if (m > nr_dir_entries || /* all entries have been iterated OR */
			flg)				  /* file is found */
			break;
	}
	assert(flg);
	if (m == nr_dir_entries)
	{ // the file is the last one in the dir
		_dirInode->i_size = dir_size;
		sync_inode(_dirInode);
	}

	return 0;
}
