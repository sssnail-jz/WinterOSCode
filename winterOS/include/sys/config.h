#ifndef _WINTEROS_CONFIG_H_
#define _WINTEROS_CONFIG_H_

/**
 * [ 40257 (分区总扇区)  - 256 ]  / 512(一个文件的分区数目) == 78
 * [ 0x8D00 - 256 ] / 512 == 70
 * 故而 ：前 70 个文件留作执行程序和其余文件
 *       后面 8 个文件大小留作 程序tar包(71~74) 和 log(75~78)
 */
#define INSTALL_START_SECT 0x8D00
#define INSTALL_NR_SECTS 0x800

// 记录系统启动的时候的一些值
#define BOOT_PARAM_ADDR 0x900   
#define BOOT_PARAM_MAGIC 0xB007 // 用来比对获取对了没有

// boot_params 结构体成员的偏移量
#define BI_MAG 0
#define BI_MEM_SIZE 1
#define BI_KERNEL_FILE 2
#define BI_DISP_POS 3

#define MINOR_BOOT MINOR_hd2a

#endif
