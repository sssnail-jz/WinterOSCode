#ifndef _WINTEROS_TYPE_H_
#define _WINTEROS_TYPE_H_

#define PUBLIC
#define PRIVATE static

#define I_TYPE_MASK 0170000
#define I_REGULAR 0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY 0040000
#define I_CHAR_SPECIAL 0020000
#define I_NAMED_PIPE 0010000

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef char *va_list;

#define TRUE 1
#define FALSE 0
#define NULL 0
#define BOOL int

typedef void (*int_handler)();		  // 陷阱处理句柄函数
typedef void (*task_f)();			  // 进程/任务函数
typedef void (*irq_handler)(int irq); // 硬件处理句柄函数
typedef void *__SYSCALL__;			  // 系统调用处理函数类型

#define MAX_FILENAME_LEN 12

struct posix_tar_header
{						// byte offset
	char name[100];		//   0
	char mode[8];		// 100
	char uid[8];		// 108
	char gid[8];		// 116
	char size[12];		// 124
	char mtime[12];		// 136
	char chksum[8];		// 148
	char typeflag;		// 156
	char linkname[100]; // 157
	char magic[6];		// 257
	char version[2];	// 263
	char uname[32];		// 265
	char gname[32];		// 297
	char devmajor[8];   // 329
	char devminor[8];   // 337
	char prefix[155];   // 345
						// 500
};

struct _Msg
{
	int i_1;
	int i_2;
	int i_3;
	int i_4;
	int i_5;
	int i_6;
	int i_7;
	u64 l_1;
	u64 l_2;
	void *p_1;
	void *p_2;
};
typedef struct
{
	int source;
	int type;
	struct _Msg _msg;
} MESSAGE;

struct boot_params
{
	int mem_size;					 // memory size
	unsigned char *addr_kernel_file; // addr of kernel file
	int disp_pos;
};

struct time
{
	u32 year;
	u32 month;
	u32 day;
	u32 hour;
	u32 minute;
	u32 second;
};

// 文件的状态和文件的信息是不一样的
struct stat
{
	int st_dev;  // major/minor device number
	int st_ino;  // i-node number
	int st_mode; // file mode, protection bits, etc.
	int st_rdev; // device ID (if special file)
	int st_size; // file size
};

// 文件类型、inode 号、被引用次数、大小、文件名
struct file_info
{
	int type;
	int inode_nr;
	int ref_cnt;
	int size;
	char name[MAX_FILENAME_LEN];
};

#endif
