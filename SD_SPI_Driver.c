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

#include "stdio.h"

#include "SD_SPI_Driver.h"
#include "dIO.h"
#include "SPI.h"
#include "serialHand.h"
#include "timing.h"

char sendString[BUFFERSIZE];

uint8_t cardType = SD_TYPE_ERR;

GPIO_TypeDef * CD_PORT = GPIOA; // The btn is port C, so is UART2
const int CD_Pin = 9;

SPI_TypeDef *SPI = SPI2;

// Move to SPI?
void selectPeriferal()
{
    set_PinState(GPIOB, 9, 0); // Low for selected
}

void deSelectPeriferal()
{
    set_PinState(GPIOB, 9, 1); // High for deselected
}

int SD_disk_initialize()
//int SD_disk_initialize(SPI_TypeDef *SPI)
{
    // Card detect 
  	configure_portNum(0); // Turn on the clock for the port
	configure_inPin(CD_PORT, CD_Pin, PIN_FLOATING);
  	if(!get_pinState(CD_PORT, CD_Pin)) { return STA_NODISK; }

	SPI_Init(2); // SPI2 is for the file IO

    // Wait for the card to power up.
    delay_s(0.250);


    // Slow the clock down for init: <= 400kHz
    //SPI->CR1 |= 0b110 << 3;//fPCLK/128             // BR bits 5:3

    SD_setSPI_Mode(SPI); // Sends 80 clock pulses to put the card in SPI mode

    selectPeriferal(); // Select to send the init commands


    // Send cmd0: Go to idle state
    //    After sending the command, SD_SendCommand() waits for the R1 response. 
    //    This is the first byte that follows which has bit# 7 set low: Expecting 0x01
    //    Bits 0:6 contain possible error codes with bit 0 indicating that the SD card is in idle tate.
    uint8_t SDCMD = 0; // CMD0: GO_IDLE_STATE, expect 0x01
    uint8_t response[1];
    uint8_t trys = 10;
    //sprintf(sendString, "Send CMD0 rx size: %i", sizeof(response));
	//writeSerialString(USART2, sendString, 1); 
    do 
    {
        SD_SendCommand(SPI, SDCMD, 0, 0x95, response, 1);  // SPI, CMD, ARG, CRC, expedt return print debug
    } while((response[0] != SD_IN_IDLE_STATE) && trys-- > 0);
    if(response[0] !=SD_IN_IDLE_STATE)
    {
	    sprintf(sendString, "CMD%i Error: Failed to goto IDLE = 0x%02x\n\r", SDCMD, response[0]); 
	    writeSerialString(USART2, sendString, 0); 
        return STA_NOINIT;
        //return -1*SD_IN_IDLE_STATE;
    }

    SDCMD = 8;// CMD8: IS it V2? if response = 1, version 2
    uint8_t batVoltRsp[5];
    // CMD8: Card version, and then voltage response
	//writeSerialString(USART2, "Send CMD8: ", 1); 
    SD_SendCommand(SPI, SDCMD, 0x1AA, CMD8_CRC, batVoltRsp, 5);
    if(response[0] == 0x01)
    {
        cardType =  SD_TYPE_V2;
    }
    else
    {
	    sprintf(sendString, "Response to cmd%i: Unsuported card  = 0x%02x\n\r", SDCMD, batVoltRsp[0]); 
	    writeSerialString(USART2, sendString, 0); 
        return STA_NOINIT;
        //return -SD_TYPE_ERR;
    }
    if((batVoltRsp[3] != 0x01) && (batVoltRsp[4] != 0xAA))
    {
        //0x00 00 01 AA
        // The AA is the check pattern, the 01 = yes, can operate at this voltage
	    sprintf(sendString, "Response voltage:  = 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x\n\r", 
                                                  batVoltRsp[0], batVoltRsp[1], batVoltRsp[2], batVoltRsp[3], batVoltRsp[4]); 
	    writeSerialString(USART2, sendString, 0); 
        return STA_NOINIT;
    }

    //cardType =  SD_TYPE_MMC;
    if(cardType == SD_TYPE_MMC)
    {
        SDCMD = 1;// CMD1: return 0x00 (no error)
        trys = 0xFE;
        do {
            SD_SendCommand(SPI, SDCMD, 0x00, 0x01, response, 1); 
        }while((response[0] != SD_RESPONSE_NO_ERROR) && trys-- > 0);

        if(response[0] != SD_RESPONSE_NO_ERROR)
        {
	        sprintf(sendString, "CMD%i Error: MMC Failed to leave IDLE = 0x%02x\n\r", SDCMD, response[0]); 
	        writeSerialString(USART2, sendString, 0); 
            return STA_NOINIT;
        }
    }
    else if(cardType == SD_TYPE_V2)
    {
        // Send cmd to leave idle state
        SDCMD = 41;// CMD41: return 0x00 (no error)
        trys = 10;
	    //writeSerialString(USART2, "CMD41\n\r", 0); 
        do 
        {
            SD_SendACMD(SPI, SDCMD, 0x40000000, CMD41_CRC, response, 1); // Send cmd55 then the cmd
        }while((response[0] != SD_RESPONSE_NO_ERROR) && trys-- > 0);
        if(response[0] != SD_RESPONSE_NO_ERROR)
        {
	        sprintf(sendString, "CMD%i Error: Failed to leave IDLE = 0x%02x\n\r", SDCMD, response[0]); 
	        writeSerialString(USART2, sendString, 0); 
            return STA_NOINIT;
        }
    
        SDCMD = 58;// CMD58: read OCR: 
        // If bit 6 of the last of 4 is 1, it high capacity
	    //writeSerialString(USART2, "CMD58\n\r", 0); 
        SD_SendCommand(SPI, SDCMD, 0,0x01, response, 1);
        if(( response[0]&0x40 >> 6) == 1)
        {
        cardType = SD_TYPE_V2HC; // High Capacity card
        }
	    //sprintf(sendString, "CMD%i response: Card Type = 0x%02x\n\r", SDCMD, response); 
	    //writeSerialString(USART2, sendString, 0); // add newline and carriage return
    } // End V2

    //CMD2, Not in SPI
    //CMD3, Not in SPI

    // Set block lengh: CMD16: Returns 0x00 on sucess
    // If SDHC block leght is already set
    /*
    SDCMD = 16;// CMD16: Set Block length
    trys = 10;
    do 
    {
        SD_SendCommand(SPI, SDCMD, BLOCKSIZE, CMD16_CRC, response, 1);
        if(response[0] != SD_RESPONSE_NO_ERROR)
        { writeSerialString(USART2, "Retrying\n\r", 0); }
    }while((response[0] != SD_RESPONSE_NO_ERROR) && trys-- > 0);
    if(response[0] != SD_RESPONSE_NO_ERROR)
    {
	    sprintf(sendString, "CMD%i Error: Set block error= 0x%02x\n\r", SDCMD, response[0]); 
	    writeSerialString(USART2, sendString, 0); 
        return STA_NOINIT;
    }
    */

    // Get number of sectors

    // Check SCR register ACMD51
    //SD_SendCommand(SPI, 51, 0x00, 0x01, response, 8);
    // Bit 46 is CMD23 support
    deSelectPeriferal();


    // once we are cooking we can set the clock back to high
    SPI->CR1 &= ~SPI_CR1_BR_Msk; // 0b000 is 1/2 clock
    //SPI->CR1 |= 0b001 << 3;  // BR bits 5:3  0b001 is 1/4 clock speed

	writeSerialString(consolSerialPort, "SD Card Init Finished\n\r", 0); 
    return SD_disk_status();
}

DSTATUS SD_disk_status()
{
	//writeSerialString(consolSerialPort, "Check Disk Status\n\r", 0); 
  	if(!get_pinState(CD_PORT, CD_Pin)) 
    { 
	    writeSerialString(consolSerialPort, "No Card\n\r", 0); 
        set_runMode(-1);
        return STA_NODISK | STA_NOINIT; 
    }

    // Check for write protect

    return 0x00;
}

void SD_SendCommand(SPI_TypeDef *SPI, uint8_t cmd, uint32_t arg, uint8_t cksum, uint8_t*buf, int dataLen)
{
    // TODO: Number of bytes we are expecting
    //buf[0] = 0xFF;
    uint8_t rspNum = 0;
    int retryDelay = 1; 

    SPI_Send(SPI, 0x40 | cmd);            // Command, must start with 10, then the 6 bit command
    SPI_Send(SPI, (arg >> 24) & 0xFF);    // Argument[31:24]
    SPI_Send(SPI, (arg >> 16) & 0xFF);    // Argument[23:16]
    SPI_Send(SPI, (arg >> 8)  & 0xFF);    // Argument[15:8]
    SPI_Send(SPI,  arg        & 0xFF);    // Argument[7:0]
    buf[0] = SPI_Send(SPI, cksum);

    int32_t timOutLen = 15; 
    int32_t delayLen = 15; 
    
    if((dataLen > 8) && (cmd != 0)) // CMD8 does not send the start token
    { // get to the start of block token
        int32_t timeout = timOutLen; 
        do{

            buf[0] = SPI_Send(SPI2, 0xFF);
        } while (buf[0] != 0xFE && timeout-- > 0);

    }
	//writeSerialString(USART2, "*** Response: ", 0); 
	//sprintf(sendString, "%i = 0x%02x, ", rspNum, buf[0]); 
	//writeSerialString(USART2, sendString, 0); 
    for(int i = 0; i < dataLen; i++)
    {
        int delay = delayLen;
        int32_t timeout = timOutLen; 
        do{
            // Add a small delay between trys, even need it on the first
            // The card seems fine, but the anlaizer has a hard time
            //if(timeout < timOutLen){
            if(timOutLen != timeout) { for(int i = delay; i > 0; i--); }
            //}
            buf[i] = SPI_Send(SPI2, 0xFF);
        } while (buf[i] == 0xFF && timeout-- > 0);

        if(timeout <= 0)
        {
	        sprintf(sendString, "timout cmd (%i): byte %i = 0x%02x\n\r", cmd, buf[i], i); 
	        writeSerialString(USART2, sendString, 0); 
        }
    }
}

void SD_SendACMD(SPI_TypeDef *SPI, uint8_t cmd, uint32_t arg, uint8_t cksum, uint8_t*buf, int dataLen)
{
    // The next command is an application command
    int32_t timeout = 15; 
    do{
        SD_SendCommand(SPI, 55, 0, CMD55_CRC, buf, 1); 
    } while (buf[0] == 0xFF && timeout-- > 0);
    if((buf[0] != 0x01) && (buf[0] != 0x00))
    {
	    sprintf(sendString, "Bad cmd55 Response to cmd%i:  = 0x%02x\n\r", 55, buf[0]); 
	    writeSerialString(USART2, sendString, 0); 
    }

	//writeSerialString(USART2, "CMDxx\n\r", 0); 
    SD_SendCommand(SPI, cmd, arg, cksum, buf, dataLen); 
    //return response;
}


void SD_setSPI_Mode(SPI_TypeDef *SPI)
{
    while ((SPI->SR & SPI_SR_BSY) == SPI_SR_BSY);     // Wait until not BSY is set
    deSelectPeriferal();
    // Init, send > 78 clock pulses with CS high
	writeSerialString(USART2, "Send 80+ Clock pulses\n\r", 0); // add newline and carriage return
    for (int i = 0; i < 10; i++) {
        while ((SPI->SR & SPI_SR_TXE) != SPI_SR_TXE);     // Wait until TXE is set
        *((volatile uint8_t*)&SPI->DR) = 0xFF;  // Send data Do not trigger read
    }

    while ((SPI->SR & SPI_SR_BSY) == SPI_SR_BSY);     // Wait until not BSY is set
}

DRESULT isCardRdy(int timeout)
{
    while ((SPI_Send(SPI, 0XFF)!=0xFF)&&timeout--);
    if (timeout==0)
    {
	    writeSerialString(USART2, "Card not ready\n\r", 0); 
        return RES_NOTRDY;
    }
    return RES_OK;

}

DSTATUS SD_RX_Block(uint8_t*buff, uint16_t count)
{
    //Wait for SD card to send back data initiation token 0xFE
    uint8_t initTok = 0xFE;
    uint8_t timeout = 0xFF;
    while ((SPI_Send(SPI, 0XFF)!=initTok)&&timeout--);


    if (timeout==0)
    {
	    sprintf(sendString, "Error: data initializtion tken not retured error= 0x%02x\n\r", initTok); 
	    writeSerialString(USART2, sendString, 0); 
        return SD_RESPONSE_FAILURE;//Failed to get response   
    }


    // Get counts blocks of data (BLOCKSIZE)
    while(count--)
    {
	    //sprintf(sendString, "Receving block 0x%02x\n\r", count); 
	    //writeSerialString(USART2, sendString, 0); 
        *buff=SPI_Send(SPI, 0xFF);
        buff++;
    }
    SPI_Send(SPI, 0xFF);
    uint8_t response = SPI_Send(SPI, 0xFF);		

    return response;//Correct response
    //return SD_RESPONSE_NO_ERROR;//Correct response
}


DSTATUS SD_TX_Block(const uint8_t*buff, uint8_t command)
{
    // Wait for the card to be ready
    if(isCardRdy(0xFF) != RES_OK){ return SD_RESPONSE_FAILURE;}

    SPI_Send(SPI, command);
    uint8_t response = 0xFF;
    for(uint16_t thisByte = 0;thisByte<BLOCKSIZE;thisByte++)
    {
        response = SPI_Send(SPI, buff[thisByte]);
    }

    uint8_t timeout = 0x1F;
    do{ // Wait for a non 0xFF
	    //sprintf(sendString, "r%i 0x%02x\n\r", timeout, response); 
	    //writeSerialString(USART2, sendString, 0); 
        response = SPI_Send(SPI, 0xFF); 
    }while((response == 0xFF) && timeout-- > 0);

    // 0xE5, 0x00, 0x07
    //also getting a "Card is bussy" that we need to handel

    if((response&0x1F) != 0x05)
    {
	    sprintf(sendString, "write response ERROR!! = 0x%02x\n\r", response); 
	    writeSerialString(USART2, sendString, 0); 
        //return 2;//Response error	
        return RES_OK;
    }

    if(isCardRdy(0xFF) != RES_OK){ return SD_RESPONSE_FAILURE;}

    return RES_OK;
}

// For FatFS
DRESULT SD_disk_read(uint8_t* buff, uint32_t sector, uint32_t blockCount)
{
    uint8_t response[1];
    //sector <<=9; // Non V2HC cards are byte addressed
    selectPeriferal();
    //If not HD Card

    //if(blockCount ==1)
    if(blockCount == 0)
    {
        uint8_t SDCMD = 17;// CMD17: Read a single block
        uint8_t trys = 10;
        SD_SendCommand(SPI, SDCMD, sector, 0x00, response, 1);
        if(response[0] == SD_RESPONSE_NO_ERROR)
        {
            response[0] = SD_RX_Block(buff, BLOCKSIZE);
        }
        else
        {
	        sprintf(sendString, "CMD%i Error: Set block read mode error= 0x%02x\n\r", SDCMD, response[0]); 
	        writeSerialString(USART2, sendString, 0); 
        }
        /*
        do{
            SD_SendCommand(SPI, SDCMD, sector, 0x00, response, 1);
        }while((response[0] != SD_RESPONSE_NO_ERROR) && trys-- > 0);
        if(response[0] == SD_RESPONSE_NO_ERROR)
        {
            response[0] = SD_RX_Block(buff, BLOCKSIZE);
        }
        else
        {
        }
        */
    }
    else
    {
        uint8_t SDCMD = 18;// CMD18: Put in multi byte read mode
        uint8_t trys = 10;
        do 
        {
            SD_SendCommand(SPI, SDCMD, sector, CMD18_CRC, response, 1);
        }while((response[0] != SD_RESPONSE_NO_ERROR) && trys-- > 0);
        if(response[0] != SD_RESPONSE_NO_ERROR)
        {
	        sprintf(sendString, "CMD%i Error: Set multi block read mode error= 0x%02x\n\r", SDCMD, response[0]); 
	        writeSerialString(USART2, sendString, 0); 
        }

        do{ // Get nBlocks
            response[0] = SD_RX_Block(buff, BLOCKSIZE);
            buff += BLOCKSIZE;
        } while(--blockCount && response[0] == SD_RESPONSE_NO_ERROR);
    
        SD_SendCommand(SPI, 12, 0x00, 0x01, response, 1);
    }

    int timeout = 10;
    do{ // Wait for a good response
        response[0] = SPI_Send(SPI, 0xFF); 
	    //sprintf(sendString, "r%i 0x%02x\n\r", timeout, response[0]); 
	    //writeSerialString(USART2, sendString, 0); 
    }while((response[0] != 0x00) && timeout-- > 0);

	//sprintf(sendString, "SD_Disk_Read: reponse 0x%02x\n\r", response[0]); 
	//writeSerialString(USART2, sendString, 0); 

    deSelectPeriferal();
    return response[0];
}

DRESULT SD_disk_write(const uint8_t* buff, uint32_t sector, uint32_t blockCount)
{
    selectPeriferal();

    uint8_t response[1];
    //sector <<=9; // Non V2HC cards are byte addressed
    //sector *=BLOCKSIZE; // Non V2HC cards are byte addressed

    // From: SD_Physical_Layer_Spec.pdf
    //CMD3, Not SPI, Publish Address

    //CMD4, Program DSR, not SPI
    //   9, Send card data (CSD)
    //   10, Send card id (CID)
    //   3, Not SPI, Publish Address

    //CMD7 Set trans state? Not SPI

    //CMD16, Set block len
    //   32, Erase Block start
    //   33, Erase Block end
    //   ACMD6, Not SPI
    //   42, Set, clear card detect 
    //   23, Set number of erase blocks

    //CMD24, Write one block
    //   25, Write multi block mode
    //   26, Not SPI
    //   27, Program CSD
    //   42, Lock/Unlock
    //   56, GEN_CMD, ??, Set read write? 1=read, 0 = write
    //SPI_Send(SPI, 0XFF);
    int singleBlock = 0;
    if(blockCount == 1)
    {
        singleBlock = 1;
	    //writeSerialString(USART2, "Send one block\n\r", 0); 
        SD_SendCommand(SPI, 24, sector, 0x01, response, 1); // Single sector write
        response[0] = SD_TX_Block(buff, 0xFE);
    }
    else
    {
        for(int i = 0; i < 8; i++) { SPI_Send(SPI, 0xFF); } // add a delay
    
        uint8_t SDCMD = 25;// CMD25: Put in multi byte write mode
        SD_SendCommand(SPI, SDCMD, sector, 0x01, response, 1);
        if(response[0] != SD_RESPONSE_NO_ERROR)
        {
	        sprintf(sendString, "CMD%i Error: Set multi block write mode error= 0x%02x\n\r", SDCMD, response[0]); 
	        writeSerialString(USART2, sendString, 0); 
            return response[0];
        }

        // Now send the data
        do{
            response[0] = SD_TX_Block(buff, 0xFC);
            buff += BLOCKSIZE;
        } while(--blockCount && response[0] == SD_RESPONSE_NO_ERROR);

        // Do we need this?
        response[0] = SPI_Send(SPI, 0xFD); // Terminate

        if(singleBlock == 0)
        {
            uint8_t SDCMD = 12;// CMD12: Put out of multi byte write mode
            SD_SendCommand(SPI, SDCMD, sector, 0x00, response, 1);
        }
    }

    delay_s(0.001);

    deSelectPeriferal();

    //return response[0];
    return RES_OK;
}


DRESULT SD_disk_ioctl(uint8_t cmd, void* buff)
{
    /* Generic command (Used by FatFs) */
    selectPeriferal();
    uint8_t SDCMD;

	//sprintf(sendString, "ioctl cmd: 0x%02x\n\r", cmd); 
	//writeSerialString(USART2, sendString, 0); 

    uint8_t returnVal = 0x00;


    if(cmd == CTRL_SYNC)
    {
        //CTRL_SYNC: /* Complete pending write process (needed at FF_FS_READONLY == 0) */
        returnVal = isCardRdy(0xF0); // 0 is Sucsess RES_OK
	    //sprintf(sendString, "Card Redy: 0x%02x\n\r", returnVal); 
	    //writeSerialString(USART2, sendString, 0); 
    }
    else if( cmd == CTRL_TRIM)
    {   // CTRL_TRIM:  Inform device that the data on the block of sectors is no longer used (needed at FF_USE_TRIM == 1) 
    	/* From fatfs/documents/doc/dioctl.html
          Informs the disk I/O layter or the storage device that the data on the block of sectors is no longer 
          needed and it can be erased. 
          The sector block is specified in an LBA_t array {<Start LBA>, <End LBA>} that pointed by buff. 
          This is an identical command to Trim of ATA device. 
          Nothing to do for this command if this funcion is not supported or not a flash memory device. 
          FatFs does not check the result code and the file function is not affected even if the 
          sector block was not erased well. 
          This command is called on remove a cluster chain and in the f_mkfs function. 
          It is required when FF_USE_TRIM == 1.
        */

        //  FF_USE_TRIM == 0 is set in ffconf.h
	    sprintf(sendString, "******* FIRST TIME USING CTRL_TRIM ***********: %i\n\r", cmd); 
	    writeSerialString(USART2, sendString, 0); 
        LBA_t start = ((LBA_t*)buff)[0];
        LBA_t end   = ((LBA_t*)buff)[1];
        uint8_t trimReturn[0];
        SD_SendCommand(SPI, 32, start * 512, 0x00, trimReturn, 1);   // CMD32: Set start address
        SD_SendCommand(SPI, 32, end * 512, 0x00, trimReturn, 1);     // CMD33: Set end address
        SD_SendCommand(SPI, 32, 0, 0x00, trimReturn, 1);             // CMD38: Erase
        returnVal = 1;  // Success

    }
    else 
    {   // The rest are in the CSD Block
        // https://users.ece.utexas.edu/~valvano/EE345M/SD_Physical_Layer_Spec.pdf

        int dataLen = 16+1; // Pluss 1 for the CRC
        uint8_t CSD[dataLen];  
        SDCMD = 9;// CMD9: Get the CSD Block
        SD_SendCommand(SPI, SDCMD, 0x00, 0xAF, CSD, dataLen);

        int CSDVer = 1;
        if((CSD[0] >> 6) == 1) {CSDVer = 2;}
	    sprintf(sendString, "CSD Version: %i\n\r", CSDVer); 
	    writeSerialString(USART2, sendString, 0); 

        // Print out for debuging
        for(int i = 0; i < dataLen; i++)
        {
	        sprintf(sendString, "Byte[%i]= 0x%02x, %i\n\r", i, CSD[i], CSD[i]); 
	        writeSerialString(USART2, sendString, 0); 
        }

        if(cmd == GET_SECTOR_COUNT)
        {  //GET_SECTOR_COUNT: /* Get media size (needed at FF_USE_MKFS == 1) */
            // Device Size C_SIZE in CSD
            DWORD C_SIZE = CSD[9] + 
                            ((DWORD)CSD[8] << 8) + 
                            ((DWORD)(CSD[7] & 63) << 16) + 1;
			*(DWORD*)buff = C_SIZE;

	        //sprintf(sendString, "Sector Count: %i\n\r", secCount); 
	        //writeSerialString(USART2, sendString, 0); 
            returnVal = RES_OK;
        }
        if(cmd == GET_SECTOR_SIZE)
        {   //GET_SECTOR_SIZE: // Get sector size (needed at FF_MAX_SS != FF_MIN_SS) 
            // CSD, READ_BL_LEN
            // 512.. It is always 512
            BYTE read_bl_len = (CSD[5] & 0x0F); // Extract READ_BL_LEN (Bits 83–80)
            *(DWORD*)buff = 1 << read_bl_len;    // Sector size = 2^READ_BL_LEN
	        sprintf(sendString, "Block Len: %i\n\r", read_bl_len); 
	        writeSerialString(USART2, sendString, 0); 
            returnVal = RES_OK;
        }

        if(cmd == GET_BLOCK_SIZE)
        {    // GET_BLOCK_SIZE:  Get erase block size (needed at FF_USE_MKFS == 1) 
            // If SDC V2, SD Status byte 10?
            if (CSD[10] & 0x40) // ERASE_BLK_EN (Bit 46)
            {
                *(DWORD*)buff = 1; // Erase block size = 1 sector
            }
            else
            {
                BYTE sector_size = (CSD[10] & 0x3F); //  Bits 45:39 Byte 10[5:0]
                *(DWORD*)buff = 1 << sector_size;   // Erase block size in sectors
            }
            returnVal = RES_OK;
        }
    }

    deSelectPeriferal();
    return returnVal;
}



void testDriver()
{

    int fileIOInitState = SD_disk_initialize(); // Iniit the filesystem
		if(fileIOInitState != 0)
		{
        	sprintf(sendString, "File IO Init FAILED: 0x%02x\n\r", fileIOInitState); 
			writeSerialString(USART2, sendString, 0); 
		}
		else
		{   // We have a good card
        	sprintf(sendString, "File IO Init Sucess: 0x%02x\n\r", fileIOInitState); 


			sprintf(sendString, "Init Complete"); 
			writeSerialString(USART2, sendString, 1);
			writeSerialString(USART2, "-----------------------------\n\r", 0); 


			uint8_t result;
			LBA_t secCount;
			result = SD_disk_ioctl(GET_SECTOR_COUNT, &secCount);
        	sprintf(sendString, "Sector Count Status: 0x%02x, size: %i\n\r", result, secCount); 
			writeSerialString(USART2, sendString, 0);
			LBA_t secSize;
			result = SD_disk_ioctl(GET_SECTOR_SIZE, &secSize);
        	sprintf(sendString, "Sector Size Status: 0x%02x, size: %i \n\r", result, secSize); 
			writeSerialString(USART2, sendString, 0);

			LBA_t eraseBlockSZ;
			result = SD_disk_ioctl(GET_BLOCK_SIZE, &eraseBlockSZ);
        	sprintf(sendString, "Erase Block Size Status: 0x%02x, size: %i \n\r", result, eraseBlockSZ); 
			writeSerialString(USART2, sendString, 0);

		    int blocks = 3;
            uint32_t sector = 100;

            uint8_t txDATA_Block[BLOCKSIZE*blocks];
            uint8_t rxDATA_Block[BLOCKSIZE*blocks];

			int i;
			txDATA_Block[0] = 0x01;
			for(i = 1; i < BLOCKSIZE*blocks; i++)
			{
				if(i%2) { txDATA_Block[i] = 0x0A; }
				else    { txDATA_Block[i] = 0x05; } // 0th byte first
        		//sprintf(sendString, "0x%02x ", txDATA_Block[i]); 
				//writeSerialString(USART2, sendString, 0); 
			}
			txDATA_Block[i-1] = 0x01;

			writeSerialString(USART2, "Write Data\n\r", 1); 
		    SD_disk_write(txDATA_Block, sector, blocks);
			writeSerialString(USART2, "Done Writeing Data\n\r", 1); 

		    SD_disk_read(rxDATA_Block, sector, blocks);
		    writeSerialString(USART2, "Done reading data \n\r", 1); // add newline and carriage return

			for(i = BLOCKSIZE*2; i < BLOCKSIZE*blocks; i++)
			{
        		sprintf(sendString, "%i:0x%02x ", i, rxDATA_Block[i]); 
				writeSerialString(USART2, sendString, 0); 

				//Be neet
				if(i%10 ==0){ writeSerialString(USART2, "\n\r", 0); }

				if(i%BLOCKSIZE == 0){ writeSerialString(USART2, "\n\r------------------------\n\r", 0); }
			}
        }
}