#ifndef _WINTEROS_CONST_H_
#define _WINTEROS_CONST_H_

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define BLACK 0x0						/* 0000 */
#define WHITE 0x7						/* 0111 */
#define RED 0x4							/* 0100 */
#define GREEN 0x2						/* 0010 */
#define BLUE 0x1						/* 0001 */
#define FLASH 0x80						/* 1000 0000 */
#define BRIGHT 0x08						/* 0000 1000 */
#define MAKE_COLOR(x, y) ((x << 4) | y) /* MAKE_COLOR(Background,Foreground) */

/* GDT 和 IDT 中描述符的个数 */
#define GDT_SIZE 128
#define IDT_SIZE 256

/* 权限 */
#define PRIVILEGE_KRNL 0
#define PRIVILEGE_TASK 1
#define PRIVILEGE_USER 3
/* RPL */
#define RPL_KRNL SA_RPL0
#define RPL_TASK SA_RPL1
#define RPL_USER SA_RPL3

/* Process */
#define RUNABLE 0x00
#define SENDING 0x02
#define RECEIVING 0x04
#define WAITING 0x08
#define HANGING 0x10
#define FREE_SLOT 0x20

/* TTY */
#define NR_CONSOLES 3 /* consoles */
#define TTY_FIRST (tty_table)
#define TTY_END (tty_table + NR_CONSOLES)

// 初始化和控制 8259A 芯片的端口
#define INT_M_CTL 0x20
#define INT_M_CTLMASK 0x21
#define INT_S_CTL 0xA0
#define INT_S_CTLMASK 0xA1

// 8253/8254 时钟中断频率修改需要用到的端口
#define TIMER0 0x40			/* I/O port for timer channel 0 */
#define TIMER_MODE 0x43		/* I/O port for timer mode control */
#define RATE_GENERATOR 0x34 /* 00-11-010-0 :                                     \
							 * Counter0 - LSB then MSB - rate generator - binary \
							 */
#define TIMER_FREQ 1193182L /* clock frequency for timer in PC and AT */

// 针对 virtual box 和 bochs 设置时钟中断发生的频率
#define VIRTUAL_BOX
#ifdef VIRTUAL_BOX
#define HZ 5000
#else
#define HZ 500
#endif

/* AT keyboard */
/* 8042 ports */
#define KB_DATA 0x60
#define KB_CMD 0x64
#define LED_CODE 0xED
#define KB_ACK 0xFA

/* VGA */
#define CRTC_ADDR_REG 0x3D4 /* CRT Controller Registers - Addr Register */
#define CRTC_DATA_REG 0x3D5 /* CRT Controller Registers - Data Register */
#define START_ADDR_H 0xC	/* reg index of video mem start addr (MSB) */
#define START_ADDR_L 0xD	/* reg index of video mem start addr (LSB) */
#define CURSOR_H 0xE		/* reg index of cursor position (MSB) */
#define CURSOR_L 0xF		/* reg index of cursor position (LSB) */
#define V_MEM_BASE 0xB8000  /* base of color video memory */
#define V_MEM_SIZE 0x8000   /* 32K: B8000H -> BFFFFH */

/* CMOS */
// 从 BIOS 获取时间需要用到的端口
#define CLK_ELE 0x70 /* CMOS RAM address register port (write only) \
					  * Bit 7 = 1  NMI disable                      \
					  *	   0  NMI enable                            \
					  * Bits 6-0 = RAM address                      \
					  */
#define CLK_IO 0x71  /* CMOS RAM data register port (read/write) */

#define YEAR 9 /* Clock register addresses in CMOS RAM	*/
#define MONTH 8
#define DAY 7
#define HOUR 4
#define MINUTE 2
#define SECOND 0
#define CLK_STATUS 0x0B /* Status register B: RTC configuration	*/
#define CLK_HEALTH 0x0E /* Diagnostic status: (should be set by Power \
						 * On Self-Test [POST])                       \
						 * Bit  7 = RTC lost power                    \
						 *	6 = Checksum (for addr 0x10-0x2d) bad      \
						 *	5 = Config. Info. bad at POST              \
						 *	4 = Mem. size error at POST                \
						 *	3 = I/O board failed initialization        \
						 *	2 = CMOS time invalid                      \
						 *  1-0 = reserved                            \
						 */

/* Hardware interrupts */
#define NR_IRQ 16 /* Number of IRQs */
#define CLOCK_IRQ 0
#define KEYBOARD_IRQ 1
#define CASCADE_IRQ 2   /* cascade enable for 2nd AT controller */
#define ETHER_IRQ 3		/* default ethernet interrupt vector */
#define SECONDARY_IRQ 3 /* RS232 interrupt vector for port 2 */
#define RS232_IRQ 4		/* RS232 interrupt vector for port 1 */
#define XT_WINI_IRQ 5   /* xt winchester */
#define FLOPPY_IRQ 6	/* floppy disk */
#define PRINTER_IRQ 7
#define AT_WINI_IRQ 14 /* at winchester */

/* tasks */
/* 注意 TASK_XXX 的定义要与 global.c 中对应 */
#define INVALID_DRIVER -20
#define INTERRUPT -10
#define TASK_TTY 0
#define TASK_SYS 1
#define TASK_HD 2
#define TASK_FS 3
#define TASK_MM 4
#define INIT 5
#define TESTA 6
#define TESTB 7
#define TESTC 8
#define ANY (NR_TASKS + NR_PROCS + 10)
#define NO_TASK (NR_TASKS + NR_PROCS + 20)

#define MAX_TICKS 0x7FFFABCD

/* system call */
#define NR_SYS_CALL 3

/* ipc */
#define SEND 1
#define RECEIVE 2
#define BOTH 3 /* BOTH = (SEND | RECEIVE) */

/* magic chars used by `printx' */
#define MAG_CH_PANIC '\002'
#define MAG_CH_ASSERT '\003'

// 要修改 msgtype 记得同时修改 msgtype_str
enum msgtype
{
	HARD_INT = 1,

	/* SYS task */
	GET_TICKS,
	GET_PID,
	GET_RTC_TIME,
	SET_COLOR,

	/* FS */
	OPEN,
	CLOSE,
	READ,
	WRITE,
	SEEK,
	STAT,
	UNLINK,
	FILSS_INFO,
	CLEAR,

	/* FS & TTY */
	SUSPEND_PROC,
	RESUME_PROC,
	TTY_OPEN,
	TTY_READ,
	TTY_WRITE,
	TTY_CLEAR,

	/* MM */
	EXEC,
	WAIT,

	/* FS & MM */
	FORK,
	EXIT,

	/* TTY, SYS, FS, MM, etc */
	SYSCALL_RET,

	/* message type for drivers */
	DEV_OPEN,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL
};

/* macros for messages */
#define FD _msg.i_1
#define VALUE _msg.i_1
#define PATHNAME _msg.p_1
#define FLAGS _msg.i_1
#define NAME_LEN _msg.i_2
#define BUF_LEN _msg.i_3
#define CNT _msg.i_2
#define REQUEST _msg.i_5
#define PROC_NR _msg.i_3
#define DEVICE _msg.i_4
#define pos_byte _msg.l_1
#define BUF _msg.p_2
#define OFFSET _msg.i_2
#define WHENCE _msg.i_3
#define PID _msg.i_2
#define RETVAL _msg.i_1
#define STATUS _msg.i_1

#define DIOCTL_GET_GEO 0x1

#define EDIT 0x2
#define CTL_IN_BUF 0x4

/* Hard Drive */
#define SECTOR_SIZE 512
#define SECTOR_BITS (SECTOR_SIZE * 8)
#define SECTOR_SIZE_SHIFT 9

/* major device numbers (corresponding to kernel/global.c::dd_map[]) */
#define NO_DEV 0
#define DEV_FLOPPY 1
#define DEV_CDROM 2
#define DEV_HD 3
#define DEV_CHAR_TTY 4
#define DEV_SCSI 5
/* make device number from major and minor numbers */
#define MAJOR_SHIFT 8
#define MAKE_DEV(a, b) ((a << MAJOR_SHIFT) | b)
/* separate major and minor numbers from device number */
#define MAJOR(x) ((x >> MAJOR_SHIFT) & 0xFF)
#define MINOR(x) (x & 0xFF)

#define INVALID_INODE 0
#define ROOT_INODE 1

#define MAX_DRIVES 2
#define NR_PART_PER_DRIVE 4
#define NR_SUB_PER_PART 16
#define NR_SUB_PER_DRIVE (NR_SUB_PER_PART * NR_PART_PER_DRIVE)
#define NR_PRIM_PER_DRIVE (NR_PART_PER_DRIVE + 1)

#define MAX_PRIM (MAX_DRIVES * NR_PRIM_PER_DRIVE - 1)

#define MAX_SUBPARTITIONS (NR_SUB_PER_DRIVE * MAX_DRIVES)

/* device numbers of hard disk */
#define MINOR_hd1a 0x10
#define MINOR_hd2a (MINOR_hd1a + NR_SUB_PER_PART)

#define ROOT_DEV MAKE_DEV(DEV_HD, MINOR_BOOT)

#define P_PRIMARY 0
#define P_EXTENDED 1

#define WINTEROS_PART 0x99 /* 我们的分区的文件系统类型 */
#define NO_PART 0x00	   /* unused entry */
#define EXT_PART 0x05	  /* extended partition */

#define NR_FILES 64
#define NR_FILE_DESC 64 /* FIXME */
#define NR_INODE 64		/* FIXME */
#define NR_SUPER_BLOCK 8

#define is_special(m) ((((m)&I_TYPE_MASK) == I_BLOCK_SPECIAL) || \
					   (((m)&I_TYPE_MASK) == I_CHAR_SPECIAL))

#define NR_DEFAULT_FILE_SECTS 512 /* 512 * 512 = 256K */

#endif /* _WINTEROS_CONST_H_ */
