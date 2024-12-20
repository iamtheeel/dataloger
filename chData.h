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

#ifndef __DATALOG_DATA_H__
#define __DATALOG_DATA_H__
#include <stdlib.h>
#include "stm32l476xx.h"
#include "ff.h"

typedef enum {
	DACT_ANALOG = 0,        // Analog Ch
	DACT_SERIAL,			// Serial Port
    DACT_PWM                // PWM 
} DATA_ACK_TYPE;

struct data_format_struct
{
    uint8_t         chNum;      // Wich ch
    float           calValue;   // V/Eu
    float           offset;     // V of offset
    char            Eu[10];     // Engineering Units
    char            Name[20];   // Name of the ch
    float           value;      // Current value
    DATA_ACK_TYPE   sourceType; // Where is the data coming from
    int             sourceCh;   // the chan
    float           vDiv_r1;   // the chan
    float           vDiv_r2;   // the chan
};


void init_fileSystem();
void init_Interupts();

void fs_error(FRESULT err);

void set_chData(int ch, float data);
FRESULT write_chData(int echo, int write);

void get_fileName(char *fName);
void get_hdrStr(char *hdrStr);
void config_chStruct();



#endif