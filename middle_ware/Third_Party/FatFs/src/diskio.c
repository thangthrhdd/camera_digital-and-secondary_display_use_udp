/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs                                   */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "ff.h"     // Bắt buộc include để định nghĩa LBA_t, BYTE, DSTATUS, DRESULT
#include "sd.h"     // Driver SD tự viết của bạn

#define DEV_SD    0 // Định nghĩa thẻ SD là ổ số 0
#ifndef LBA_t
typedef DWORD LBA_t; /* DWORD thực chất là uint32_t trong ff.h */
#endif
/*-----------------------------------------------------------------------*/
/* Lấy trạng thái ổ đĩa                                                  */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
    BYTE pdrv        /* Physical drive nmuber to identify the drive */
)
{
    if (pdrv != DEV_SD) return STA_NOINIT;
    return 0; // Trạng thái hoạt động bình thường (OK)
}

/*-----------------------------------------------------------------------*/
/* Khởi tạo ổ đĩa                                                        */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (
    BYTE pdrv        /* Physical drive nmuber to identify the drive */
)
{
    if (pdrv != DEV_SD) return STA_NOINIT;

    // Gọi hàm khởi tạo SD thanh ghi của bạn
    if (SD_Init() == 0) {
        return 0; // Khởi tạo thành công (RES_OK)
    }

    return STA_NOINIT; // Khởi tạo thất bại
}

/*-----------------------------------------------------------------------*/
/* Đọc Sector                                                            */
/*-----------------------------------------------------------------------*/
DRESULT disk_read (
    BYTE pdrv,       /* Physical drive nmuber to identify the drive */
    BYTE* buff,      /* Data buffer to store read data */
    LBA_t sector,    /* Start sector in LBA */
    UINT count       /* Number of sectors to read */
)
{
    if (pdrv != DEV_SD) return RES_PARERR;

    // Đọc tuần tự từng sector
    for (UINT i = 0; i < count; i++) {
        if (SD_Read_Sector(sector + i, buff + (i * 512)) != 0) {
            return RES_ERROR; // Lỗi đọc sector
        }
    }

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Ghi Sector                                                            */
/*-----------------------------------------------------------------------*/
DRESULT disk_write (
    BYTE pdrv,          /* Physical drive nmuber to identify the drive */
    const BYTE* buff,   /* Data to be written */
    LBA_t sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
)
{
    if (pdrv != DEV_SD) return RES_PARERR;

    // Ghi tuần tự từng sector
    for (UINT i = 0; i < count; i++) {
        if (SD_Write_Sector(sector + i, (uint8_t*)(buff + (i * 512))) != 0) {
            return RES_ERROR; // Lỗi ghi sector
        }
    }

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Các lệnh điều khiển bổ sung (I/O Control)                             */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE cmd,       /* Control code */
    void* buff      /* Buffer to send/receive control parameter */
)
{
    if (pdrv != DEV_SD) return RES_PARERR;

    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            return RES_ERROR;
        case GET_SECTOR_SIZE:
            *(WORD*)buff = 512;
            return RES_OK;
        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;
            return RES_OK;
    }

    return RES_PARERR;
}

/*-----------------------------------------------------------------------*/
/* Hàm lấy thời gian thực cho File (Không dùng __weak)                   */
/*-----------------------------------------------------------------------*/
DWORD get_fattime (void)
{
    // Trả về thời gian cố định: Ngày 16/07/2026, 15:00:00
    return ((DWORD)(2026 - 1980) << 25) /* Year 2026 */
         | ((DWORD)7 << 21)             /* Month 7 */
         | ((DWORD)16 << 16)            /* Mday 16 */
         | ((DWORD)15 << 11)            /* Hour 15 */
         | ((DWORD)0 << 5)              /* Min 0 */
         | ((DWORD)0 >> 1);             /* Sec 0 */
}
