#ifndef __RTC_H
#define __RTC_H

#include <stdint.h>

void RTC_Clock_Enable(void);    // Enable Clock to RTC
void RTC_Init(void);            // Initialize RTC for our datalogger
uint32_t get_time32(void);      // Function for the SD Card

static uint8_t Decimal_to_BCD(int Dec);
static uint8_t BCD_to_Decimal(uint8_t bcd); 

void get_time(char *time);      // Read Time HH:MM:SS.ss
void get_date(char *date);      // Read Data MM/DD/YYYY

void set_time(int hr, int min, int sec);    // Setting function for Time
void set_date(int months, int day, int year);   // Setting function for date

float get_s();

#endif