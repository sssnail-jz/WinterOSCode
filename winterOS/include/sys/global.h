#ifndef _WINTEROS_GLOBAL_H_
#define _WINTEROS_GLOBAL_H_

// 将 PUBLIC 的变量真正定义在这里
#ifdef GLOBAL_VARIABLES_HERE
#undef EXTERN
#define EXTERN
#endif

// #define _WINTEROS_DEBUG_

EXTERN int ticks;
EXTERN int disp_pos;
EXTERN u32 k_reenter;
EXTERN int current_console;
EXTERN struct kb_inbuf kb_in;
EXTERN struct tss tss;
EXTERN struct proc *p_proc_ready;
EXTERN int msg_counter;

EXTERN u8 gdt_ptr[6]; /* 0~15:Limit  16~47:Base */
EXTERN struct descriptor gdt[GDT_SIZE];
EXTERN u8 idt_ptr[6]; /* 0~15:Limit  16~47:Base */
EXTERN struct gate idt[IDT_SIZE];

extern char task_stack[];
extern struct proc proc_table[];
extern struct task task_table[];
extern struct task user_proc_table[];
extern irq_handler irq_table[];
extern TTY tty_table[];
extern CONSOLE console_table[];

// FS
EXTERN struct file_desc f_desc_table[NR_FILE_DESC];
EXTERN struct inode inode_table[NR_INODE];
EXTERN struct super_block super_block[NR_SUPER_BLOCK];

// FS buffer
EXTERN MESSAGE fs_msg;
extern u8 *fsbuf;
extern const int FSBUF_SIZE;
EXTERN struct proc *pcaller;
EXTERN struct inode *root_inode;
extern struct dev_drv_map dd_map[];

// MM buffer
EXTERN MESSAGE mm_msg;
extern u8 *mmbuf;
extern const int MMBUF_SIZE;
EXTERN int memory_size;

// LOG buffer
extern u8 *logbuf;
extern const int LOGBUF_SIZE;
extern char *msgtype_str[];

// 颜色变量
extern u8 GRAY_CHAR;
extern u8 BLUE_RED_CHAR;
extern u8 DEFAULT_CHAR_COLOR;

#endif
