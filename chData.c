/*
 * SFSU Engr 498 Final Project
 * 
 * Joshua Mehlman, Cannek Heredia, Vrishika Vijay Mohite
 *  Fall, 2024
 *  Data Logger
 */
/*
 * Data formating and ch configurations
 */

#include "chData.h"
#include "serialHand.h"
#include "timing.h"
#include "fatfs/source/ff.h"
#include "RTC.h"

#include <string.h>
#include <stdlib.h>
// Serial Io

// Data structure
#define NUMDATACH 8 // How many data channels
struct data_format_struct chData[NUMDATACH];
uint8_t dataCh[NUMDATACH]; // Which ch goes where


// FileSystem
FATFS FatFs;        // FileSystem Object
FIL fil;            // File System object
FRESULT fr;         // File System return code
char fileName[13];  // FAT filelen 8.3
// File IO indicator
GPIO_TypeDef * ioLED_PORT = GPIOC; // The btn is port C, so is UART2
const unsigned long ioLED_pin = 0;

// Start stop btn
GPIO_TypeDef * startStopBtnPORT = GPIOC; // The btn is port C, so is UART2
int saveDataMode = 0;


void init_fileSystem()
{
    saveDataMode = set_runMode(RUNMODE_SOFF);

    char sendString[BUFFERSIZE];

    sprintf(sendString, "init_chData: Init Filesystem\n\r"); 
	writeSerialString(consolSerialPort, sendString, 0); 

    
    // *** File System
    // Mount the card
    fr = f_mount(&FatFs, "", 0); // Pointer, Drive Number, Immidiatly Mount
    if(fr != FR_OK) { fs_error(fr); }
    sprintf(sendString, "init_chData: Filesystem mount return: %i\n\r", fr); 
	writeSerialString(consolSerialPort, sendString, 1); 

    // Set up the ch data
    config_chStruct();

    // Get the next available file
    get_fileName(fileName);


    // Open the file
    fr = f_open(&fil, fileName, FA_CREATE_ALWAYS | FA_WRITE);
    if(fr != FR_OK) { fs_error(fr); }
    sprintf(sendString, "init_chData: Filesystem Open file (%s) return: %i\n\r", fileName, fr); 
	writeSerialString(consolSerialPort, sendString, 0); 

    // Write the headder
    // Write data to the file
    char hdrStr[512];
    get_hdrStr(hdrStr);
    fr = f_puts(hdrStr, &fil);
    if(fr < 0) { fs_error(fr); }
    sprintf(sendString, "init_chData:  File f_puts num chars written: %i\n\r", fr); 
	writeSerialString(consolSerialPort, sendString, 0); 

/*  Example of f_printf
    fr = f_printf(&fil, "f_printf: string %d, %.3f\n", 22, 3.14159);
    sprintf(sendString, "init_chData:  File f_printf num chars written: %i\n\r", fr); 
	writeSerialString(consolSerialPort, sendString, 0); 
*/

    fr = f_close(&fil);
    if(fr != FR_OK) { fs_error(fr); }
    sprintf(sendString, "init_chData:  File close return: %i\n\r", fr); 
	writeSerialString(consolSerialPort, sendString, 0); 

}
void init_Interupts()
{
    configure_inPin(GPIOC, 11, PIN_PULLUP);
    init_EXTI(GPIOC, 2, 11, 0, 1); // Set for falling trigger

    // The file save LED
    configure_outPin(ioLED_PORT, ioLED_pin, PIN_PUSHPULL);  // the clock for port C is already on

    char sendString[BUFFERSIZE];
    sprintf(sendString, "chData: init_Interupts done: \n\r" ); 
	writeSerialString(consolSerialPort, sendString, 0); 
}

void get_fileName(char *fName)
{
    // Get the next available file
    int fileNumber = 0; // Start with 0
    char delim[] = ".";
	writeSerialString(consolSerialPort, "init_chData: Get the next available filename\r\n", 0); 
    //TODO: Move to function
    int trys = 1000;
    DIR dj;         // Directory object 
    FILINFO fno;    // File information 
    FRESULT fr = f_findfirst(&dj, &fno, "", "*.CSV"); //Start to search for CSV files 
    // This times out on failure

    while( (fr == FR_OK && fno.fname[0]) && (trys-- > 0))
    {    // Repeat while an item is found 
        // Get the number of this file
        char *token;
        token = strtok(fno.fname, delim);

        // Convert the file name to an int
        char *endptr;
        int lastFileNum = (int)strtol(token, &endptr, 10); // Base 10
        if(*endptr == 0) // ends with nullchar, is good 
        {
            if(fileNumber <= lastFileNum)
            {
                fileNumber = lastFileNum +1;
            }
        }

        //char sendString[BUFFERSIZE];
        //sprintf(sendString, "init_chData: filename: %s, %s, %i\n\r", fno.fname, token, fileNumber); 
	    //writeSerialString(consolSerialPort, sendString, 0); 

        fr = f_findnext(&dj, &fno);     // Search for next item 
    }
    f_closedir(&dj);

    if(trys > 0)
    {
        sprintf(fName, "%i.CSV", fileNumber);
    }
    else
    {
        fs_error(0);
	    writeSerialString(consolSerialPort, "Files not found after 1000 trys", 0); 
    }
}

void get_hdrStr(char *hdrStr)
{
    strcpy(hdrStr, "Date, Time");
    for(int i=0; i < NUMDATACH; i++)
    {
        // Ch Names
        sprintf(hdrStr, "%s, %s", hdrStr, chData[i].Name);
    }

    strcat(hdrStr, "\nMM/DD/YY, hh:mm:ss.SSS");
    for(int i=0; i < NUMDATACH; i++)
    {
        // Ch V/eU
        sprintf(hdrStr, "%s, %s", hdrStr, chData[i].Eu);
    }
    strcat(hdrStr, "\n");
}

void set_chData(int ch, float data)
{
    chData[ch].value = data / chData[ch].calValue - chData[ch].offset;
}


FRESULT write_chData(int echo, int write)
{
	//writeSerialString(consolSerialPort, "Write ch\n\r", 0); 

    char chDataStr[512];
    char dateStr[10];
    char timeStr[16];
    get_date(dateStr);
    get_time(timeStr);

    sprintf(chDataStr, "%s, %s", dateStr, timeStr);
    for(int i=0; i < NUMDATACH; i++)
    {
        // Ch V/eU
        sprintf(chDataStr, "%s, %f", chDataStr, chData[i].value);
    }
    strcat(chDataStr, "\n");

    if(echo)
    {
	    writeSerialString(consolSerialPort, chDataStr, 2); // add the cr, but not the nl
    }


    if(write)
    {
        set_PinState(ioLED_PORT, ioLED_pin, 1);
        if(fr != FR_OK){return fr;} // Check for last file op sucess
        char debugStr[128];
        
        fr = f_open(&fil, fileName, FA_OPEN_APPEND | FA_WRITE);
        if(fr != FR_OK)
        {
            fs_error(fr);
            sprintf(debugStr, "Open File: %s,  filesystem status %i\r\n", fileName, fr);
            writeSerialString(consolSerialPort, debugStr, 0);
        }

        //  **** Write the file **** //
        fr = f_puts(chDataStr, &fil);
        if(fr < 0) { fs_error(fr); }

        fr = f_close(&fil);
        //sprintf(debugStr, "Close File: %s,  filesystem status %i\r\n", fileName, fr);
        //writeSerialString(consolSerialPort, debugStr, 0);
        set_PinState(ioLED_PORT, ioLED_pin, 0);
        return fr;
    }

    // No write called, assume ok
    return FR_OK;
}

void config_chStruct()
{
    char debugStr[128];

    int thisCh = 0;
    strcpy(chData[thisCh].Name, "Batt");
    strcpy(chData[thisCh].Eu, "V");
    chData[thisCh].value = 0;
    chData[thisCh].offset = 0;
    chData[thisCh].calValue = 1;
    chData[thisCh].vDiv_r1 = 20000;
    chData[thisCh].vDiv_r2 = 10000;
    //sprintf(debugStr, "Ch %i: r1 = %.1f, r2 = %.1f\r\n", thisCh, chData[thisCh].vDiv_r1, chData[thisCh].vDiv_r2);
    //writeSerialString(consolSerialPort, debugStr, 0);
    if((chData[thisCh].vDiv_r1 != 0) && (chData[thisCh].vDiv_r2 != 0))
    {
        float vDiv = chData[thisCh].vDiv_r2/ (chData[thisCh].vDiv_r1 + chData[thisCh].vDiv_r2);
        chData[thisCh].calValue = chData[thisCh].calValue * vDiv;

        //sprintf(debugStr, "Ch %i: vDiv = %.3f, cal = %.3f\r\n", thisCh, vDiv, chData[thisCh].calValue);
        //writeSerialString(consolSerialPort, debugStr, 0);
    }

    thisCh++;
    strcpy(chData[thisCh].Name, "Accl x");
    strcpy(chData[thisCh].Eu, "g");
    chData[thisCh].value = 0;
    chData[thisCh].calValue = 1;
    chData[thisCh].offset = 0;
    chData[thisCh].vDiv_r1 = 0;
    chData[thisCh].vDiv_r2 = 0;
    if((chData[thisCh].vDiv_r1 != 0) && (chData[thisCh].vDiv_r2 != 0))
    {
        float vDiv = (chData[thisCh].vDiv_r1 + chData[thisCh].vDiv_r2) /chData[thisCh].vDiv_r2;
        chData[thisCh].calValue = chData[thisCh].calValue * vDiv;
    }

    thisCh++;
    strcpy(chData[thisCh].Name, "Accl y");
    strcpy(chData[thisCh].Eu, "g");
    chData[thisCh].value = 0;
    chData[thisCh].calValue = 1;
    chData[thisCh].offset = 0;
    chData[thisCh].vDiv_r1 = 0;
    chData[thisCh].vDiv_r2 = 0;
    if((chData[thisCh].vDiv_r1 != 0) && (chData[thisCh].vDiv_r2 != 0))
    {
        float vDiv = (chData[thisCh].vDiv_r1 + chData[thisCh].vDiv_r2) /chData[thisCh].vDiv_r2;
        chData[thisCh].calValue = chData[thisCh].calValue * vDiv;
    }

    thisCh++;
    strcpy(chData[thisCh].Name, "Accl Z");
    strcpy(chData[thisCh].Eu, "g");
    chData[thisCh].value = 0;
    chData[thisCh].calValue = 1;
    chData[thisCh].offset = 0;
    chData[thisCh].vDiv_r1 = 0;
    chData[thisCh].vDiv_r2 = 0;
    if((chData[thisCh].vDiv_r1 != 0) && (chData[thisCh].vDiv_r2 != 0))
    {
        float vDiv = (chData[thisCh].vDiv_r1 + chData[thisCh].vDiv_r2) /chData[thisCh].vDiv_r2;
        chData[thisCh].calValue = chData[thisCh].calValue * vDiv;
    }

    thisCh++;
    strcpy(chData[thisCh].Name, "Distance");
    strcpy(chData[thisCh].Eu, "cm");
    chData[thisCh].value = 0;
    chData[thisCh].calValue = 1;
    chData[thisCh].offset = 0;
    chData[thisCh].vDiv_r1 = 0;
    chData[thisCh].vDiv_r2 = 0;
    if((chData[thisCh].vDiv_r1 != 0) && (chData[thisCh].vDiv_r2 != 0))
    {
        float vDiv = (chData[thisCh].vDiv_r1 + chData[thisCh].vDiv_r2) /chData[thisCh].vDiv_r2;
        chData[thisCh].calValue = chData[thisCh].calValue * vDiv;
    }

    thisCh++;
    strcpy(chData[thisCh].Name, "Radiation");
    strcpy(chData[thisCh].Eu, "mili sV");
    chData[thisCh].value = 0;
    chData[thisCh].calValue = 1;
    chData[thisCh].offset = 0;
    chData[thisCh].vDiv_r1 = 0;
    chData[thisCh].vDiv_r2 = 0;
    if((chData[thisCh].vDiv_r1 != 0) && (chData[thisCh].vDiv_r2 != 0))
    {
        float vDiv = (chData[thisCh].vDiv_r1 + chData[thisCh].vDiv_r2) /chData[thisCh].vDiv_r2;
        chData[thisCh].calValue = chData[thisCh].calValue * vDiv;
    }

    thisCh++;
    strcpy(chData[thisCh].Name, "sensor a");
    strcpy(chData[thisCh].Eu, "mV");
    chData[thisCh].value = 0;
    chData[thisCh].calValue = 1;
    chData[thisCh].offset = 0;
    chData[thisCh].vDiv_r1 = 0;
    chData[thisCh].vDiv_r2 = 0;
    if((chData[thisCh].vDiv_r1 != 0) && (chData[thisCh].vDiv_r2 != 0))
    {
        float vDiv = (chData[thisCh].vDiv_r1 + chData[thisCh].vDiv_r2) /chData[thisCh].vDiv_r2;
        chData[thisCh].calValue = chData[thisCh].calValue * vDiv;
    }

    thisCh++;
    strcpy(chData[thisCh].Name, "temp");
    strcpy(chData[thisCh].Eu, "C");
    chData[thisCh].value = 0;
    chData[thisCh].calValue = 1;
    chData[thisCh].offset = 0;
    chData[thisCh].vDiv_r1 = 0;
    chData[thisCh].vDiv_r2 = 0;
    if((chData[thisCh].vDiv_r1 != 0) && (chData[thisCh].vDiv_r2 != 0))
    {
        float vDiv = (chData[thisCh].vDiv_r1 + chData[thisCh].vDiv_r2) /chData[thisCh].vDiv_r2;
        chData[thisCh].calValue = chData[thisCh].calValue * vDiv;
    }
}

void fs_error(FRESULT err)
{
    TIM2->CR1  &= ~TIM_CR1_CEN; // disable timer

    set_runMode(-1);
    char debugStr[128];
    sprintf(debugStr, "!!!FILE IO ERROR:  %i!!!\r\n", err);
    writeSerialString(consolSerialPort, debugStr, 0);
}


/***      Interupts       ***/
void EXTI15_10_IRQHandler(void)
{
    int runMd = get_runMode();
    char debugStr[128];

	if((EXTI->PR1 & EXTI_PR1_PIF11) == EXTI_PR1_PIF11) 
	{
        if(runMd == RUNMODE_SOFF){
            runMd = set_runMode(RUNMODE_NORM);
            TIM2->CR1  |= TIM_CR1_CEN; // Enable timer

		    writeSerialString(consolSerialPort, "Start Data Save\r\n", 0);
        }
        else if(runMd == RUNMODE_NORM){
            runMd = set_runMode(RUNMODE_SOFF);
            TIM2->CR1  &= ~TIM_CR1_CEN; // disable timer

		    writeSerialString(consolSerialPort, "Stop Data Save\r\n", 0);

        }
		EXTI->PR1 = EXTI_PR1_PIF11; // Clear pin interupt
	}

	if((EXTI->PR1 & EXTI_PR1_PIF13) == EXTI_PR1_PIF13) // Built in btn
    {
        // Shutdown, set both LEDs high
        set_PinState(GPIOA, 5, 1);
        set_PinState(GPIOC, 0, 1);

        int shutDownHoldTime_s = 1;
        sprintf(debugStr, "Shutdown! Hold Btn for %i sec\r\n", shutDownHoldTime_s);
		writeSerialString(consolSerialPort, debugStr, 0);

        for(int i = 4e5; i > 0; i--){;} // 4e6 is 16 sec
        //delay_s(1); // wtf? This is hanging
        if(get_pinState(GPIOC, 13))
        {
            set_PinState(GPIOC, 0, 0);
		    writeSerialString(consolSerialPort, "Shutdown Cancled\r\n", 0);
            for(int i = 2e4; i > 0; i--){;}
        }
        else
        {
            char *shutdown_messeage = "\nSystem will shutdown. goodbye world!\0";
		    writeSerialString(consolSerialPort, shutdown_messeage, 0);
            for(int i = 2e4; i > 0; i--){;}

            call_Shutdown();
        }

		EXTI->PR1 = EXTI_PR1_PIF13; // Clear pin interupt
    }

	NVIC_ClearPendingIRQ(EXTI15_10_IRQn); // Clear the interupt group
}