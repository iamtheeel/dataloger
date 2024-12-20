/*
 * Timing Functions, using SYSTICK
 * Joshua Mehlman
 */

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42, phk@FreeBSD.ORG):
 * <iamtheeel> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   MRM
 * ----------------------------------------------------------------------------
 */

#ifndef __MRM_TIME_H__
#define __MRM_TIME_H__

#include "stm32l476xx.h"

#define MAINLOOPFREQ 1000.0 //Hz

#define RUNMODE_CMDE -1 // There was an error
#define RUNMODE_SAFE 0 // pressing the btn reads 0
#define RUNMODE_NORM 1 // Pin is pullup
#define RUNMODE_SOFF 2 // Sensors off, but all else on

extern int clockFreq;

void set_sysTick_time(unsigned int time_ms);
void set_sysTick_freq(unsigned int freq_hz);
void init_sysTiming(int runMode);

void run100HzTasks();
void run25HzTasks();
void run5HzTasks();
void run1HzTasks();


void delay_s(float sToDelay);

int set_runMode(int rMode);
int get_runMode();


void init_Shutdown(void);
void call_Shutdown(void);

#endif
