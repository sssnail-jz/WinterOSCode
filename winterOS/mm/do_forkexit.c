// ----------------------------
// <do_forkexit.c>
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

PRIVATE void cleanup(struct proc *proc);

/**
 * 1, find a free slot in 'proc table array'  
 * 2, copy 'proc table' for child(but not ldt_sel)
 * 3, modify child's 'LDT desc'
 * 4, alloc memory for child
 * 5, wake up 'parent' and 'child' with difent val(when do_fork return back)
 */
PUBLIC int do_fork()
{
	// find a free slot in proc_table
	struct proc *_proc = proc_table;
	int _childID;
	for (_childID = 0; _childID < NR_TASKS + NR_PROCS; _childID++, _proc++)
		if (_proc->p_status == FREE_SLOT)
			break;

	assert(_proc == &proc_table[_childID]);
	assert(_childID >= NR_TASKS + NR_NATIVE_PROCS);
	if (_childID == NR_TASKS + NR_PROCS) // no free slot
		return -1;
	assert(_childID < NR_TASKS + NR_PROCS);

	// duplicate the process table
	int _parentID = mm_msg.source;
	// 每个进程都有每个进程的 LDT，也有其相对应的选择子
	u16 _child_ldt_sel = _proc->ldt_sel;
	*_proc = proc_table[_parentID];
	_proc->ldt_sel = _child_ldt_sel;
	_proc->p_parent = _parentID;
	sprintf(_proc->name, "%s_%d", proc_table[_parentID].name, _childID);

	// duplicate the process: T, D & S
	struct descriptor *_pDescp;

	// Text segment
	_pDescp = &proc_table[_parentID].ldts[INDEX_LDT_C];
	int caller_T_base = reassembly(_pDescp->base_high, 24,
								   _pDescp->base_mid, 16,
								   _pDescp->base_low);
	int caller_T_limit = reassembly(0, 0,
									(_pDescp->limit_high_attr2 & 0xF), 16,
									_pDescp->limit_low);
	// carefully limit is 'K' or 'B'
	int caller_T_size = ((caller_T_limit + 1) *
						 ((_pDescp->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ? 4096 : 1));

	// Data & Stack segments
	_pDescp = &proc_table[_parentID].ldts[INDEX_LDT_RW];
	int caller_D_S_base = reassembly(_pDescp->base_high, 24,
									 _pDescp->base_mid, 16,
									 _pDescp->base_low);
	int caller_D_S_limit = reassembly((_pDescp->limit_high_attr2 & 0xF), 16,
									  0, 0,
									  _pDescp->limit_low);
	// carefully limit is 'K' or 'B'
	int caller_D_S_size = ((caller_T_limit + 1) *
						   ((_pDescp->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ? 4096 : 1));

	// we don't separate T, D & S segments, so we have:
	assert((caller_T_base == caller_D_S_base) &&
		   (caller_T_limit == caller_D_S_limit) &&
		   (caller_T_size == caller_D_S_size));

	int _child_base = alloc_mem(_childID, caller_T_size);

	// OK, there we copy ...
	phys_copy((void *)_child_base, (void *)caller_T_base, caller_T_size);

	// child's LDT
	init_desc(&_proc->ldts[INDEX_LDT_C],
			  _child_base,
			  (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
			  DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER << 5);
	init_desc(&_proc->ldts[INDEX_LDT_RW],
			  _child_base,
			  (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
			  DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER << 5);

	// printk("<FORK SUCESSFULL>\n");
	// printk("Parent    : name [%10s] ID [%2d] _base [%7x] _limit [%7x]\n",
	// 	   proc_table[_parentID].name, _parentID, caller_T_base, caller_D_S_size);
	// printk("child     : name [%10s] ID [%2d] _base [%7x] _limit [%7x]\n",
	// 	   proc_table[_childID].name, _childID, _child_base, PROC_IMAGE_SIZE_DEFAULT);

	// tell FS, add refrence count
	MESSAGE msg2fs;
	msg2fs.type = FORK;
	msg2fs.PID = _childID;
	send_recv(BOTH, TASK_FS, &msg2fs);

	// when 'MM' return, will send mm_msg to parent
	mm_msg.PID = _childID; // OK, return val given for parent is 'childID'

	// birth of the child
	MESSAGE _msg2child;
	_msg2child.type = SYSCALL_RET;
	_msg2child.RETVAL = 0;
	_msg2child.PID = 0; // OK, return val given for childen is '0'
	send_recv(SEND, _childID, &_msg2child);

	return 0;
}

PUBLIC void do_exit(int status)
{
	int i;
	int _procID = mm_msg.source; // PID of caller
	int _parentID = proc_table[_procID].p_parent;
	struct proc *_proc = &proc_table[_procID];
	// for print
	int _isWaiting;
	int _hasChild;

	// tell 'FS', decrease some refrence
	MESSAGE msg2fs;
	msg2fs.type = EXIT;
	msg2fs.PID = _procID;
	send_recv(BOTH, TASK_FS, &msg2fs);

	// so, do nothing ...
	free_mem(_procID);

	_proc->exit_status = status;
	if (proc_table[_parentID].p_status & WAITING)
	{
		_isWaiting = TRUE;
		proc_table[_parentID].p_status &= ~WAITING;
		cleanup(&proc_table[_procID]);
	}
	else
		// when parent 'wait' or parent 'exit', this proc can delete
		proc_table[_procID].p_status |= HANGING;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++)
	{
		if (proc_table[i].p_parent == _procID)
		{
			_hasChild = TRUE;
			// if proc's child is good, OK just change the parent
			// or will delete child and wake up 'INIT' once
			proc_table[i].p_parent = INIT;
			if ((proc_table[INIT].p_status & WAITING) &&
				(proc_table[i].p_status & HANGING))
			{
				proc_table[INIT].p_status &= ~WAITING;
				cleanup(&proc_table[i]);
			}
		}
	}
	// printk("<EXIT SUCESSFULL>\n");
	// printk("who exit  : name [%10s] ID [%2d] chid? [%7s]\n"
	// 	   "parent    : name [%10s] ID [%2d] wait? [%7s]\n",
	// 	   proc_table[_procID].name, _procID, _hasChild ? "HasChid" : " NoChid",
	// 	   proc_table[_parentID].name, _parentID,
	// 	   _isWaiting ? " IsWait" : " NoWait");
}

PRIVATE void cleanup(struct proc *proc)
{
	MESSAGE msg2parent;
	msg2parent.type = SYSCALL_RET;
	msg2parent.PID = proc2pid(proc);
	msg2parent.STATUS = proc->exit_status;
	send_recv(SEND, proc->p_parent, &msg2parent);

	proc->p_status = FREE_SLOT;
}

PUBLIC void do_wait()
{
	int _procID = mm_msg.source;

	int i;
	int _children = 0;
	struct proc *_pProc = proc_table;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++, _pProc++)
	{
		if (_pProc->p_parent == _procID)
		{
			_children++;
			if (_pProc->p_status & HANGING)
			{
				// find a 'HNAGING' child, clean and return just
				cleanup(_pProc);
				return;
			}
		}
	}

	// OK, there are/is some childs that this proc can wait
	if (_children)
		proc_table[_procID].p_status |= WAITING;
	else
	{
		MESSAGE msg2proc;
		msg2proc.type = SYSCALL_RET;
		msg2proc.PID = NO_TASK;
		send_recv(SEND, _procID, &msg2proc);
	}
}
