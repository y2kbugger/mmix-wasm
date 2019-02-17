/*
 * disk.h -- disk simulation
 */


#ifndef _DISK_H_
#define _DISK_H_


#define SECTOR_SIZE	512	/* sector size in bytes */

#define DISK_CTRL	0	/* control register */
#define DISK_CNT	8	/* sector count register */
#define DISK_SCT	16	/* disk sector register */
#define DISK_DMA	24	/* DMA address register */
#define DISK_CAP	32	/* disk capacity register */

#define DISK_STRT	0x01	/* a 1 written here starts the disk command */
#define DISK_IEN	0x02	/* enable disk interrupt */
#define DISK_WRT	0x04	/* command type: 0 = read, 1 = write */
#define DISK_ERR	0x08	/* 0 = ok, 1 = error; valid when BUSY = 0 */
#define DISK_BUSY	0x10	/* 1 = disk is working on a command */

#define DISK_DELAY	10	/* seek start/settle + rotational delay */
#define DISK_SEEK	50	/* full disk seek time */


#endif /* _DISK_H_ */
