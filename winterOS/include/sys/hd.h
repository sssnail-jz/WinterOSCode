#ifndef _WINTEROS_HD_H_
#define _WINTEROS_HD_H_

// 分区表项结构
struct part_ent
{
	u8 boot_ind; /* 分区状态（80h = 可引导、00h = 不可引导 其他 = 不合法） */

	// CHS 寻址
	u8 start_head;
	u8 start_sector;
	u8 start_cyl;

	u8 sys_id; /* 分区类型 */

	// CHS 寻址
	u8 end_head;
	u8 end_sector;
	u8 end_cyl;

	// LBA 寻址
	u32 start_sect; /* 起始扇区的 LBA */
	u32 nr_sects;   /* 此分区的扇区数目 */
};

// 暂时用不到的 command registers
#define REG_FEATURES 0x1F1
#define REG_ERROR REG_FEATURES

// 需要用到的 command registers
#define REG_DATA 0x1F0	/* 硬盘准备好数据之后从这个端口读数据 */
#define REG_NSECTOR 0x1F2 /* 欲读扇区数目 */
#define REG_LBA_LOW 0x1F3
#define REG_LBA_MID 0x1F4
#define REG_LBA_HIGH 0x1F5
#define REG_DEVICE 0x1F6

#define REG_CMD 0x1F7	  /* 对 1f7 端口写时 */
#define REG_STATUS REG_CMD /* 对 1f7 端口读时 */
#define STATUS_BSY 0x80
#define STATUS_DRDY 0x40
#define STATUS_DFSE 0x20
#define STATUS_DSC 0x10
#define STATUS_DRQ 0x08
#define STATUS_CORR 0x04
#define STATUS_IDX 0x02
#define STATUS_ERR 0x01

// control rgister
#define REG_DEV_CTRL 0x3F6			/* 写时 */
#define REG_ALT_STATUS REG_DEV_CTRL /* 读时 */

struct hd_cmd
{
	u8 features;
	u8 count;
	u8 lba_low;
	u8 lba_mid;
	u8 lba_high;
	u8 device;
	u8 command;
};

struct part_info
{
	u8 boot_ind; /* 分区状态（80h = 可引导、00h = 不可引导 其他 = 不合法） */
	u8 sys_id;   /* 分区类型 */
	u32 base;	/* LBA base */
	u32 size;	/* LBA size */
};

struct hd_info
{
	int open_cnt;
	struct part_info primary[NR_PRIM_PER_DRIVE];
	struct part_info logical[NR_SUB_PER_DRIVE];
};

#define HD_TIMEOUT 10000 /* in millisec */
#define PARTITION_TABLE_OFFSET 0x1BE
#define ATA_IDENTIFY 0xEC
#define ATA_READ 0x20
#define ATA_WRITE 0x30

// 是否开启 LBA、驱动器号、LBA 最高四位
#define MAKE_DEVICE_REG(lba, drv, lba_highest) (((lba) << 6) | \
												((drv) << 4) | \
												(lba_highest & 0xF) | 0xA0)

#endif
