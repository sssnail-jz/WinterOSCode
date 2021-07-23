#ifndef _WINTEROS_CONSOLE_H_
#define _WINTEROS_CONSOLE_H_

// CONSOLE
typedef struct s_console
{
	unsigned int crtc_start;
	unsigned int orig;
	unsigned int con_size;
	unsigned int cursor;
} CONSOLE;

#define SCR_UP 1
#define SCR_DN -1

#define SCR_SIZE (80 * 25)
#define SCR_WIDTH 80
#define SCR_HIGHT 25

#endif
