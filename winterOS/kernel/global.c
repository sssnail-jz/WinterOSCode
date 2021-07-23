// must in top
#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "keyboard.h"
#include "protect.h"
#include "fs.h"
#include "tty.h"
#include "console.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

PUBLIC struct proc proc_table[NR_TASKS + NR_PROCS];

PUBLIC struct task task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "TTY"},
	{task_sys, STACK_SIZE_SYS, "SYS"},
	{task_hd, STACK_SIZE_HD, "HD"}, // 驱动程序也作为单独的服务进程
	{task_fs, STACK_SIZE_FS, "FS"},
	{task_mm, STACK_SIZE_MM, "MM"}};

PUBLIC struct task user_proc_table[NR_NATIVE_PROCS] = {
	{Init, STACK_SIZE_INIT, "INIT"},
	{TestA, STACK_SIZE_TESTA, "TestA"},
	{TestB, STACK_SIZE_TESTB, "TestB"},
	{TestC, STACK_SIZE_TESTC, "TestC"}};

PUBLIC char task_stack[STACK_SIZE_TOTAL];

PUBLIC TTY tty_table[NR_CONSOLES];
PUBLIC CONSOLE console_table[NR_CONSOLES];
PUBLIC irq_handler irq_table[NR_IRQ];
PUBLIC __SYSCALL__ sys_call_table[NR_SYS_CALL] = {sys_printx, sys_sendrec};

struct dev_drv_map dd_map[] = {
	{INVALID_DRIVER}, // 0 : Unused
	{INVALID_DRIVER}, // 1 : Reserved for floppy driver
	{INVALID_DRIVER}, // 2 : Reserved for cdrom driver
	{TASK_HD},		  // 3 : Hard disk
	{TASK_TTY},		  // 4 : TTY
	{INVALID_DRIVER}  // 5 : Reserved for scsi disk driver
};

PUBLIC char *msgtype_str[100] = {
	"",
	"HARD_INT",

	// SYS task
	"GET_TICKS",
	"GET_PID",
	"GET_RTC_TIME",
	"SET_COLOR",

	// FS
	"OPEN",
	"CLOSE",
	"READ",
	"WRITE",
	"SEEK",
	"STAT",
	"UNLINK",
	"FILSS_INFO",
	"CLEAR",

	// FS & TTY
	"SUSPEND_PROC",
	"RESUME_PROC",
	"TTY_OPEN",
	"TTY_READ",
	"TTY_WRITE",
	"TTY_CLEAR",

	// MM
	"EXEC",
	"WAIT",

	// FS & MM
	"FORK",
	"EXIT",

	// TTY, SYS, FS, MM, etc
	"SYSCALL_RET",

	// message type for drivers
	"DEV_OPEN",
	"DEV_CLOSE",
	"DEV_READ",
	"DEV_WRITE",
	"DEV_IOCTL"};

// 6MB~7MB: buffer for FS
PUBLIC u8 *fsbuf = (u8 *)0x600000;
PUBLIC const int FSBUF_SIZE = 0x100000;

// 7MB~8MB: buffer for MM
PUBLIC u8 *mmbuf = (u8 *)0x700000;
PUBLIC const int MMBUF_SIZE = 0x100000;

// 8M~9M: buffer for sys log
PUBLIC u8 *logbuf = (u8 *)0x800000;
PUBLIC const int LOGBUF_SIZE = 0x100000; // is it too big ?? :)

PUBLIC u8 GRAY_CHAR = MAKE_COLOR(BLACK, BLACK) | BRIGHT;
PUBLIC u8 BLUE_RED_CHAR = MAKE_COLOR(BLUE, RED) | BRIGHT;
PUBLIC u8 DEFAULT_CHAR_COLOR = MAKE_COLOR(BLACK, WHITE);
