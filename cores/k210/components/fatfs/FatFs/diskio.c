/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/
#define DBG_TAG "FatFS_DISKIO"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"
#include "rtthread.h"

#include "fatfs/fatfs/ff.h"			/* Obtains integer types */
#include "fatfs/fatfs/diskio.h"		/* Declarations of disk functions */

#include "sdcard/sdcard.h"

#include "pins_arduino.h"

/* Definitions of physical drive number for each drive */
#define DEV_SDCARD					0

static spisd_info_t _sd_info[FF_VOLUMES];
static uint8_t _dev_sts[FF_VOLUMES] = {
	[0 ... FF_VOLUMES - 1] = 0
};

static inline DRESULT cvt_result(spisd_result_t res)
{
	if(SPISD_RESULT_OK == res) {
		return RES_OK;
	}
	// else if(SPISD_RESULT_ERROR == res) {
	// 	return RES_ERROR;
	// }
	else if(SPISD_RESULT_NO_CARD == res) {
		return RES_NOTRDY;
	}
	// else if(SPISD_RESULT_TIMEOUT == res) {
	// 	return RES_ERROR;
	// }
	return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	// DSTATUS stat;
	// int result;

	switch (pdrv) {
	case DEV_SDCARD :
		if(0x00 == _dev_sts[pdrv]) {
			return STA_NOINIT;
		}
		return 0;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	spisd_result_t result;

	switch (pdrv) {
	case DEV_SDCARD :
		if(0x00 == _dev_sts[pdrv]) {
			// result  = spisd_init(&soft_spi_interface, SDCARD_CLK_PIN, SDCARD_MIS_PIN, SDCARD_MOS_PIN, SDCARD_CSX_PIN);
			result  = spisd_init(&hard_spi_interface, SDCARD_CLK_PIN, SDCARD_MIS_PIN, SDCARD_MOS_PIN, SDCARD_CSX_PIN);

			result += spisd_get_card_info(&_sd_info[pdrv]);

			if(SPISD_RESULT_OK == result) {
				_dev_sts[pdrv] = 1;
				stat = 0;
			} else if (SPISD_RESULT_NO_CARD == result) {
				stat = STA_NODISK;
			} else {
				stat = STA_PROTECT; /* error */
			}
		} else {
			stat = 0; /* default no error */
		}

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	spisd_result_t result;

	switch (pdrv) {
	case DEV_SDCARD :
		// translate the arguments here
		result  = spisd_read_multi_block_begin(sector);
		result += spisd_read_multi_block_read(buff, count);
		result += spisd_read_multi_block_end();

		res = cvt_result(result);

		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	spisd_result_t result;

	switch (pdrv) {
	case DEV_SDCARD :
		result = spisd_write_multi_block(sector, buff, count);

		res = cvt_result(result);

		return res;
	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = 0;
	// int result;

	switch (pdrv) {
	case DEV_SDCARD :
		if(_dev_sts[pdrv]) {
			switch (cmd) {
#if FF_FS_READONLY == 0
			case CTRL_SYNC:
			break;
#endif /* FF_FS_READONLY == 0 */

#if FF_USE_MKFS == 1
			case GET_SECTOR_COUNT:
				*((DWORD *) buff) = (DWORD)((uint64_t)_sd_info[pdrv].capacity / (uint64_t)_sd_info[pdrv].block_size);
				// LOG_W("GET_SECTOR_COUNT %d", *((DWORD *) buff));
			break;
			case GET_BLOCK_SIZE:
				*((WORD *) buff) = _sd_info[pdrv].block_size;
				// LOG_W("GET_BLOCK_SIZE %d", _sd_info[pdrv].block_size);
			break;
#endif /* FF_USE_MKFS == 1 */

#if FF_MAX_SS != FF_MIN_SS
			case GET_SECTOR_SIZE:

			break;
#endif /* FF_MAX_SS != FF_MIN_SS */

#if FF_USE_TRIM == 1
			case CTRL_TRIM:

			break;
#endif /* FF_USE_TRIM == 1 */
			default:
				res = RES_ERROR;
				LOG_E("Unsupport ioctl cmd");
				break;
			}
		} else {
			res = RES_ERROR;
		}

		return res;
	}

	return RES_PARERR;
}

