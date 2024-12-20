/*
 * SFSU Engr 498 Final Project
 * 
 * Joshua Mehlman, Cannek Heredia, Vrishika Vijay Mohite
 *  Fall, 2024
 *  Data Logger
 */
/*
 * File IO
 */

#ifndef __DATALOG_FILEIO_H__
#define __DATALOG_FILEIO_H__

#include "stm32l476xx.h"

#include "fatfs/source/diskio.h"

/* Cmd List:
 * CMD0: Card Reset
 *
 * CMD41: SEND_OP_CONDITION
 * CMD55: APP_CMD
 */

#define BLOCKSIZE 512

// Definition of SD card type  
#define SD_TYPE_ERR     0X00
#define SD_TYPE_MMC     0X01
#define SD_TYPE_V1      0X02
#define SD_TYPE_V2      0X04
#define SD_TYPE_V2HC    0X06	

// Response to cmd bit number
#define SD_RESPONSE_NO_ERROR      0x00    //no error
#define SD_IN_IDLE_STATE          0x01    //card not in idle state
#define SD_ERASE_RESET            0x02    //Erase sequence not cleared
#define SD_ILLEGAL_COMMAND        0x04    //Illegal command detected
#define SD_COM_CRC_ERROR          0x08    //CRC Check Failed
#define SD_ERASE_SEQUENCE_ERROR   0x10    //Erase sequence error
#define SD_ADDRESS_ERROR          0x20    //Misaligned address in command
#define SD_PARAMETER_ERROR        0x40    //Command argument ouside allowed
// Bit 7 should always be 0
#define SD_RESPONSE_FAILURE       0xFF    //the command failed at all and there was no response

#define SD_VOLT_ERROR             0xA0    // The card did not respond to voltage 

#define CMD0_CRC 0x95 // Arg: 0x00
#define CMD8_CRC 0x87 // Arg:0x01AA
#define CMD9_CRC 0xaf // Arg: 0x00
//CMD12 STOP_TRAN
#define CMD16_CRC 0x81 // Arg: 512 (Set Block Length to arg)
#define CMD18_CRC 0x57 // Arg: block start: (Read multiple blocks)
//CMD23 Erase N Blocks
#define CMD25_CRC 0x03 // Arg: block start: (Write multi blocks
#define CMD41_CRC 0x77 // Arg: 0x40000000
#define CMD58_CRC 0xfd // Arg: 0x00         Read OCR
#define CMD55_CRC 0x65 // Arg: 0x00

/*
Some random other commands..
CMD17 0, crc=0x3b (Read one block)
CMD24 0, crc=0x6f (set write address for single block)
*/

void selectPeriferal();
void deSelectPeriferal();


void testDriver();

void SD_SendCommand(SPI_TypeDef *SPI, uint8_t cmd, uint32_t arg, uint8_t cksum,  uint8_t*buf, int dataLen);
void SD_SendACMD(SPI_TypeDef *SPI, uint8_t cmd, uint32_t arg, uint8_t cksum, uint8_t*buf, int dataLen); // Send cmd55, then send cmd
void SD_setSPI_Mode(SPI_TypeDef *SPI);

DRESULT isCardRdy(int timeout);
DSTATUS SD_RX_Block(uint8_t*buff, uint16_t count);
DSTATUS SD_TX_Block(const uint8_t*buff, uint8_t command);


// Required for FatFS
int SD_disk_initialize();
DSTATUS SD_disk_status();
DRESULT SD_disk_read(uint8_t* buff, uint32_t sector, uint32_t blockCount);
DRESULT SD_disk_write(const uint8_t* buff, uint32_t sector, uint32_t blockCount);

DRESULT SD_disk_ioctl(uint8_t cmd, void* buff);
/* Generic command (Used by FatFs) */
//#define CTRL_SYNC           0   /* Complete pending write process (needed at FF_FS_READONLY == 0) */
//#define GET_SECTOR_COUNT    1   /* Get media size (needed at FF_USE_MKFS == 1) */
//#define GET_SECTOR_SIZE     2   /* Get sector size (needed at FF_MAX_SS != FF_MIN_SS) */
//#define GET_BLOCK_SIZE      3   /* Get erase block size (needed at FF_USE_MKFS == 1) */
//#define CTRL_TRIM           4   /* Inform device that the data on the block of sectors is no longer used (needed at FF_USE_TRIM == 1) */

/* MMC/SDC specific ioctl command - Not used by FatFS, but we may find usefull*/
//#define MMC_GET_TYPE        10  /* Get card type */
//#define MMC_GET_CSD         11  /* Get CSD */
//#define MMC_GET_CID         12  /* Get CID */
//#define MMC_GET_OCR         13  /* Get OCR */
//#define MMC_GET_SDSTAT      14  /* Get SD status */
//#define ISDIO_READ          55  /* Read data form SD iSDIO register */
//#define ISDIO_WRITE         56  /* Write data to SD iSDIO register */
//#define ISDIO_MRITE         57  /* Masked write data to SD iSDIO register */



#endif
