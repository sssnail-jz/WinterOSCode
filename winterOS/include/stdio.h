#ifndef _WINTEROS_STDIO_H_
#define _WINTEROS_STDIO_H_

#include "type.h"

#define EXTERN extern
#define ASSERT

#ifdef ASSERT
void assertion_failure(char *exp, char *file, char *base_file, int line);
#define assert(exp)                  \
	if (exp)                         \
		__asm__ __volatile__("NOP"); \
	else                             \
		assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)
#else
#define assert(exp)
#endif

#define STR_DEFAULT_LEN 512
#define BCD_TO_DEC(x) ((x >> 4) * 10 + (x & 0x0f))

// ????
#define O_CREAT 1
#define O_RDWT 2

#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3

#define MAX_PATH 128

// tools
PUBLIC int send_recv(int function, int src_dest, MESSAGE *msg);
PUBLIC int printf(const char *fmt, ...);
PUBLIC int scanf(char *fmt, void *pDest);
PUBLIC int vsprintf(char *buf, const char *fmt, va_list args);
PUBLIC int sprintf(char *buf, const char *fmt, ...);
PUBLIC int getch();
PUBLIC int a2i(char s[]);
PUBLIC char *i2a(int val, int base, char **ps);
PUBLIC int h2i(char s[]);

// fs
PUBLIC int open(const char *pathname, int flags);
PUBLIC int close(int fd);
PUBLIC int clear(int fd);
PUBLIC int read(int fd, void *buf, int count);
PUBLIC int write(int fd, const void *buf, int count);
PUBLIC int unlink(const char *pathname);
PUBLIC int files_info(char *buf);
PUBLIC int seek(int fd, int off, int flags);
PUBLIC int stat(const char *path, struct stat *buf);

// sys
PUBLIC int get_pid();
PUBLIC int get_ticks();
PUBLIC int get_time();
PUBLIC int set_color(u8 _color);

// mm
PUBLIC int fork();
PUBLIC void exit(int status);
PUBLIC int wait(int *status);
PUBLIC void execl(const char *path, const char *arg, ...);
PUBLIC void execv(const char *path, char *argv[]);

#endif
