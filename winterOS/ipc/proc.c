#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

PRIVATE void block(struct proc *p);
PRIVATE void unblock(struct proc *p);
PRIVATE int msg_send(struct proc *current, int dest, MESSAGE *m);
PRIVATE int msg_receive(struct proc *current, int src, MESSAGE *m);
PRIVATE int isDeadLock(int src, int dest);

PUBLIC void schedule()
{
	struct proc *_iterator;
	int _greatest_ticks = 0;

	while (!_greatest_ticks)
	{
		for (_iterator = &FIRST_PROC; _iterator <= &LAST_PROC; _iterator++)
		{
			if (_iterator->p_status == RUNABLE)
			{
				if (_iterator->ticks > _greatest_ticks)
				{
					_greatest_ticks = _iterator->ticks;
					p_proc_ready = _iterator;
				}
			}
		}

		if (!_greatest_ticks)
			for (_iterator = &FIRST_PROC; _iterator <= &LAST_PROC; _iterator++)
				if (_iterator->p_status == RUNABLE)
					_iterator->ticks = _iterator->priority;
	}
}

PUBLIC int sys_sendrec(int function, int src_dest, MESSAGE *m, struct proc *p)
{
	// we can't use 'IPC' in ring0
	assert(k_reenter == 0);
	assert((src_dest >= 0 && src_dest < NR_TASKS + NR_PROCS) ||
		   src_dest == ANY ||
		   src_dest == INTERRUPT);

	int _ret = 0;
	int _caller = proc2pid(p);
	MESSAGE *_pmLa = (MESSAGE *)va2la(_caller, m);
	_pmLa->source = _caller;
	assert(_pmLa->source != src_dest);

	if (function == SEND)
	{
		_ret = msg_send(p, src_dest, m);
		if (_ret != 0)
			return _ret;
	}
	else if (function == RECEIVE)
	{
		_ret = msg_receive(p, src_dest, m);
		if (_ret != 0)
			return _ret;
	}
	else
		assert(function == RECEIVE || function == SEND);

	return 0;
}

PUBLIC int ldt_seg_linear(struct proc *p, int idx)
{
	struct descriptor *_pDesc = &p->ldts[idx];
	return _pDesc->base_high << 24 | _pDesc->base_mid << 16 | _pDesc->base_low;
}

PUBLIC void *va2la(int pid, void *va)
{
	struct proc *_proc = &proc_table[pid];

	u32 _seg_base = ldt_seg_linear(_proc, INDEX_LDT_RW);
	u32 _linerAddr = _seg_base + (u32)va;

	// [NR_TASKS + NR_NATIVE_PROCS]'s seg addr equal to '0'
	// so it's offset address equal to liner address
	// we must identify it
	if (pid < NR_TASKS + NR_NATIVE_PROCS)
		assert(_linerAddr == (u32)va);
	else
		__asm__ __volatile__("NOP"); // 清除警告

	return (void *)_linerAddr;
}

PUBLIC void reset_msg(MESSAGE *p)
{
	memset(p, 0, sizeof(MESSAGE));
}

PRIVATE void block(struct proc *p)
{
	assert(p->p_status);
	// reset 'p_proc_ready',when IPC int exit, will switch to
	schedule();
}

PRIVATE void unblock(struct proc *p)
{
	// Don't wake up now, wait for the next time's 'schedule'
	assert(p->p_status == RUNABLE);
}

PRIVATE int isDeadLock(int src, int dest)
{
	struct proc *p = proc_table + dest;
	while (TRUE)
	{
		if (p->p_status & SENDING)
		{
			if (p->p_wntSend_save == src)
			{
				// yes, dead lock, so print the 'circle'
				p = proc_table + dest;
				printk("##DeadLock[ %s", p->name);
				do
				{
					assert(p->p_msg);
					p = proc_table + p->p_wntSend_save;
					printk(" -> %s", p->name);
				} while (p != proc_table + src);
				printk(" ]##");

				return TRUE;
			}
			p = proc_table + p->p_wntSend_save;
		}
		else
			break;
	}
	return FALSE;
}

PRIVATE int msg_send(struct proc *current, int dest, MESSAGE *m)
{
	struct proc *_sender = current;
	struct proc *_pDest = proc_table + dest;
	assert(proc2pid(_sender) != dest);
	MESSAGE *_mLa = va2la(proc2pid(_sender), m);

	if (isDeadLock(proc2pid(_sender), dest))
		panic(">>DEADLOCK<< %s->%s", _sender->name, _pDest->name);

	if ((_pDest->p_status & RECEIVING) &&
		(_pDest->p_wntRecv_save == proc2pid(_sender) || _pDest->p_wntRecv_save == ANY))
	{
		assert(_pDest->p_msg);
		assert(m);
		phys_copy(va2la(dest, _pDest->p_msg), _mLa, sizeof(MESSAGE));

#ifdef _WINTEROS_DEBUG_
		if (_mLa->type != GET_TICKS) // ticks? too many!!!
		{
			char *_tem; // 将要接收的那个进程本来打算从哪个进程接收（就是当前进程还是 ANY 进程）
			if (_pDest->p_wntRecv_save == proc2pid(_sender))
				_tem = _sender->name;
			else
				_tem = "ANY";

			char _buf[SCR_WIDTH * 2];
			int _cnt;
			_cnt = sprintf(_buf,
						   "send succ [%7s]=>[%7s]->[%7s] # indx [%3d] T : %s\n",
						   _sender->name, _pDest->name, _tem, ++msg_counter, msgtype_str[_mLa->type]);
			_buf[_cnt] = 0;
			if ((logbuf + (msg_counter - 1) * sizeof(_buf)) >= (logbuf + LOGBUF_SIZE))
			{
				printk2("\n-------------- out of log memory (counter will rebegin) --------------\n");
				msg_counter = 1;
				strcpy((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)), _buf);
				printk2((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)));
			}
			else
			{
				strcpy((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)), _buf);
				printk2((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)));
			}
		}
#endif // _WINTEROS_DEBUG_

		_pDest->p_msg = NULL;
		_pDest->p_status &= ~RECEIVING;
		_pDest->p_wntRecv_save = NO_TASK;
		unblock(_pDest);

		assert(_pDest->p_status == RUNABLE);
		assert(_pDest->p_msg == NULL);
		assert(_pDest->p_wntRecv_save == NO_TASK);
		// 当接收方接收到了 dest 的 send 时就会设置 p_wntSend_save 为 NO_TASK
		// （初始化的时候也会设置为 NO_TASK）
		// 因为此时 dest 处于接收态，其 p_wntSend_save 为 NO_TASK
		// 已经有一个包裹要侯着接收了，不会同时再有个包裹要交给别人
		assert(_pDest->p_wntSend_save == NO_TASK);
		assert(_sender->p_status == RUNABLE);
		assert(_sender->p_msg == NULL);
		assert(_sender->p_wntRecv_save == NO_TASK);
		// _sender->p_wntSend_save 起的是保存作用，这里发送成功了，所以不保存
		assert(_sender->p_wntSend_save == NO_TASK);
	}
	else
	{
#ifdef _WINTEROS_DEBUG_
		// _pDest 在 recv ，但是想从别的进程 recv 或者在等待中断
		if ((_pDest->p_status & RECEIVING) && (_pDest->p_wntRecv_save != ANY))
		{
			char _buf[SCR_WIDTH * 2];
			char *_tem;
			int _cnt;
			if (_pDest->p_wntRecv_save == INTERRUPT) // 对方在等待中断 ...
				_tem = " INTRPT";
			else // 对方在具体等待一个进程，但不是现在这个进程 ...
				_tem = (char *)((struct proc *)(&proc_table[_pDest->p_wntRecv_save])->name);

			_cnt = sprintf(_buf,
						   "send fail [%7s]=>[%7s]->[%7s] # indx [%3d] T : %s\n",
						   _sender->name, _pDest->name, _tem, ++msg_counter, msgtype_str[_mLa->type]);
			_buf[_cnt] = 0;
			if ((logbuf + (msg_counter - 1) * sizeof(_buf)) >= (logbuf + LOGBUF_SIZE))
			{
				printk2("\n-------------- out of log memory (counter will rebegin) --------------\n");
				msg_counter = 1;
				strcpy((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)), _buf);
				printk2((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)));
			}
			else
			{
				strcpy((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)), _buf);
				printk2((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)));
			}
		}
		// 没有人 recv （前面三种等待情况都已经分析过了）
		else
		{
			char _buf[SCR_WIDTH * 2];
			int _cnt;
			_cnt = sprintf(_buf,
						   "send fail [%7s]=>[%7s] ^no acpt!^ # indx [%3d] T : %s\n",
						   _sender->name, _pDest->name, ++msg_counter, msgtype_str[_mLa->type]);
			_buf[_cnt] = 0;
			if ((logbuf + (msg_counter - 1) * sizeof(_buf)) >= (logbuf + LOGBUF_SIZE))
			{
				printk2("\n-------------- out of log memory (counter will rebegin) --------------\n");
				msg_counter = 1;
				strcpy((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)), _buf);
				printk2((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)));
			}
			else
			{
				strcpy((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)), _buf);
				printk2((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)));
			}
		}
#endif // _WINTEROS_DEBUG_

		_sender->p_status |= SENDING;
		assert(_sender->p_status == SENDING);
		_sender->p_wntSend_save = dest;
		_sender->p_msg = m;

		// append to the sending queue
		struct proc *_tail;
		if (_pDest->q_sending)
		{
			_tail = _pDest->q_sending;
			while (_tail->next_sending)
				_tail = _tail->next_sending;
			_tail->next_sending = _sender;
		}
		else 
			_pDest->q_sending = _sender;
		_sender->next_sending = 0;
		block(_sender);

		assert(_sender->p_status == SENDING);
		assert(_sender->p_msg != NULL);
		assert(_sender->p_wntRecv_save == NO_TASK);
		assert(_sender->p_wntSend_save == dest);
	}

	return 0;
}

PRIVATE int msg_receive(struct proc *current, int src, MESSAGE *m)
{
	struct proc *_current = current;
	struct proc *_pWitch = 0;
	struct proc *_pPrev = 0;
	int _copyok = FALSE;
	assert(proc2pid(_current) != src);

	if ((_current->has_int_msg) && ((src == ANY) || (src == INTERRUPT)))
	{

		MESSAGE msg;
		reset_msg(&msg);
		msg.source = INTERRUPT;
		msg.type = HARD_INT;
		assert(m);
		phys_copy(va2la(proc2pid(_current), m), &msg, sizeof(MESSAGE));

		_current->has_int_msg = 0;

		assert(_current->p_status == RUNABLE);
		assert(_current->p_msg == NULL);
		assert(_current->p_wntSend_save == NO_TASK);
		assert(_current->has_int_msg == 0);

		return 0;
	}

	// 1，请求中断，但是没有中断
	// 2，打算请求具体进程的消息，有没有中断无所谓
	// 3，想从任意进程请求消息，并且此时没有中断

	if (src == ANY) // 想从任意进程请求消息，并且此时没有中断
	{

		if (_current->q_sending)
		{
			_pWitch = _current->q_sending;
			_copyok = TRUE;

#ifdef _WINTEROS_DEBUG_
			char _buf[SCR_WIDTH * 2];
			int _cnt;
			_cnt = sprintf(_buf,
						   "recv succ [%7s]<=[%7s] from Queue # indx [%3d] Want ANY\n",
						   _current->name, _current->q_sending->name, ++msg_counter);
			_buf[_cnt] = 0;
			if ((logbuf + (msg_counter - 1) * sizeof(_buf)) >= (logbuf + LOGBUF_SIZE))
			{
				printk2("\n-------------- out of log memory (counter will rebegin) --------------\n");
				msg_counter = 1;
				strcpy((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)), _buf);
				printk2((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)));
			}
			else
			{
				strcpy((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)), _buf);
				printk2((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)));
			}
#endif // _WINTEROS_DEBUG_

			assert(_current->p_status == RUNABLE);
			assert(_current->p_msg == NULL);
			assert(_current->p_wntRecv_save == NO_TASK);
			assert(_current->p_wntSend_save == NO_TASK);
			assert(_current->q_sending != NULL);
			assert(_pWitch->p_status == SENDING);
			assert(_pWitch->p_msg != NULL);
			assert(_pWitch->p_wntRecv_save == NO_TASK);
			assert(_pWitch->p_wntSend_save == proc2pid(_current));
		}
	}
	else if (src != INTERRUPT) // 打算请求具体进程的消息，有没有中断无所谓
	{
		assert(src >= 0 && src < NR_TASKS + NR_PROCS);
		_pWitch = &proc_table[src];

		if ((_pWitch->p_status & SENDING) && (_pWitch->p_wntSend_save == proc2pid(_current)))
		{

#ifdef _WINTEROS_DEBUG_
			char _buf[SCR_WIDTH * 2];
			int _cnt;
			_cnt = sprintf(_buf,
						   "recv succ [%7s]<=[%7s]<-[%7s] # indx [%3d] want Specify\n",
						   _current->name, _pWitch->name, _current->name, ++msg_counter);
			_buf[_cnt] = 0;
			if ((logbuf + (msg_counter - 1) * sizeof(_buf)) >= (logbuf + LOGBUF_SIZE))
			{
				printk2("\n-------------- out of log memory (counter will rebegin) --------------\n");
				msg_counter = 1;
				strcpy((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)), _buf);
				printk2((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)));
			}
			else
			{
				strcpy((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)), _buf);
				printk2((char *)(logbuf + (msg_counter - 1) * sizeof(_buf)));
			}
#endif // _WINTEROS_DEBUG_

			_copyok = TRUE;
			struct proc *_iterator = _current->q_sending;
			assert(_iterator); // 因为前面确定有一个进程在往当前进程发
			while (_iterator)
			{
				assert(_pWitch->p_status & SENDING);
				if (proc2pid(_iterator) == src)
				{
					_pWitch = _iterator;
					break;
				}
				_pPrev = _iterator;
				_iterator = _iterator->next_sending;
			}

			assert(_current->p_status == RUNABLE);
			assert(_current->p_msg == NULL);
			assert(_current->p_wntRecv_save == NO_TASK);
			assert(_current->p_wntSend_save == NO_TASK);
			assert(_current->q_sending != NULL);
			assert(_pWitch->p_status == SENDING);
			assert(_pWitch->p_msg != NULL);
			assert(_pWitch->p_wntRecv_save == NO_TASK);
			assert(_pWitch->p_wntSend_save == proc2pid(_current));
		}
	}

	if (_copyok) // 从队列中任意拿了一个或者从队列中特地挑出了那个进程
	{

		// 1、2 ：当进程被从某个队列中拿出来时，他的 next_sending 就没用了，必须为 NULL
		if (_pWitch == _current->q_sending)
		{
			assert(_pPrev == NULL);
			_current->q_sending = _pWitch->next_sending;
			_pWitch->next_sending = NULL; // 1
		}
		else
		{
			assert(_pPrev);
			_pPrev->next_sending = _pWitch->next_sending;
			_pWitch->next_sending = NULL; // 2
		}								  

		assert(m);
		assert(_pWitch->p_msg);
		/* copy the message */
		phys_copy(va2la(proc2pid(_current), m),
				  va2la(proc2pid(_pWitch), _pWitch->p_msg),
				  sizeof(MESSAGE));

		_pWitch->p_msg = NULL;
		_pWitch->p_wntSend_save = NO_TASK;
		_pWitch->p_status &= ~SENDING;
		assert(_pWitch->p_status == RUNABLE);
		unblock(_pWitch);
	}
	else // 将会在 【没有任意进程可以接收】、【不能从特定进程接收】、【请求中断而没有中断】 这三种情况 RECVING
	{
		_current->p_status |= RECEIVING;
		_current->p_msg = m;

		if (src == ANY) // 没有任意进程可以接收
			_current->p_wntRecv_save = ANY;
		else if (src == INTERRUPT) // 请求中断而没有中断
			_current->p_wntRecv_save = INTERRUPT;
		else // 不能从特定进程接收
			_current->p_wntRecv_save = proc2pid(_pWitch);

		block(_current);

		assert(_current->p_status == RECEIVING);
		assert(_current->p_msg != NULL);
		assert(_current->p_wntRecv_save != NO_TASK);
		assert(_current->p_wntSend_save == NO_TASK);
		assert(_current->has_int_msg == 0);
	}

	return 0;
}

PUBLIC void inform_int(int task_nr)
{
	struct proc *p = proc_table + task_nr;

	if ((p->p_status & RECEIVING) && /* dest is waiting for the msg */
		((p->p_wntRecv_save == INTERRUPT) || (p->p_wntRecv_save == ANY)))
	{
		p->p_msg->source = INTERRUPT;
		p->p_msg->type = HARD_INT;
		p->p_msg = NULL;
		p->has_int_msg = 0;
		p->p_status &= ~RECEIVING; /* dest has received the msg */
		p->p_wntRecv_save = NO_TASK;
		assert(p->p_status == RUNABLE);
		unblock(p);

		assert(p->p_status == RUNABLE);
		assert(p->p_msg == NULL);
		assert(p->p_wntRecv_save == NO_TASK);
		assert(p->p_wntSend_save == NO_TASK);
	}
	else
		p->has_int_msg = 1;
}

PUBLIC void dump_proc(struct proc *p)
{
	char info[STR_DEFAULT_LEN];
	int i;
	int text_color = MAKE_COLOR(GREEN, RED);

	int dump_len = sizeof(struct proc);

	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, 0);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, 0);

	sprintf(info, "byte dump of proc_table[%d]:\n", p - proc_table);
	disp_color_str(info, text_color);
	for (i = 0; i < dump_len; i++)
	{
		sprintf(info, "%x.", ((unsigned char *)p)[i]);
		disp_color_str(info, text_color);
	}

	/* printk("^^"); */

	disp_color_str("\n\n", text_color);
	sprintf(info, "ANY: 0x%x.\n", ANY);
	disp_color_str(info, text_color);
	sprintf(info, "NO_TASK: 0x%x.\n", NO_TASK);
	disp_color_str(info, text_color);
	disp_color_str("\n", text_color);

	sprintf(info, "ldt_sel: 0x%x.  ", p->ldt_sel);
	disp_color_str(info, text_color);
	sprintf(info, "ticks: 0x%x.  ", p->ticks);
	disp_color_str(info, text_color);
	sprintf(info, "priority: 0x%x.  ", p->priority);
	disp_color_str(info, text_color);
	/* sprintf(info, "pid: 0x%x.  ", p->pid); disp_color_str(info, text_color); */
	sprintf(info, "name: %s.  ", p->name);
	disp_color_str(info, text_color);
	disp_color_str("\n", text_color);
	sprintf(info, "p_status: 0x%x.  ", p->p_status);
	disp_color_str(info, text_color);
	sprintf(info, "p_wntRecv_save: 0x%x.  ", p->p_wntRecv_save);
	disp_color_str(info, text_color);
	sprintf(info, "p_wntSend_save: 0x%x.  ", p->p_wntSend_save);
	disp_color_str(info, text_color);
	/* sprintf(info, "nr_tty: 0x%x.  ", p->nr_tty); disp_color_str(info, text_color); */
	disp_color_str("\n", text_color);
	sprintf(info, "has_int_msg: 0x%x.  ", p->has_int_msg);
	disp_color_str(info, text_color);
}
