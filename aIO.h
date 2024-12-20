/*
 * Handeler Functions for AIO 
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
#ifndef __MRM_AIO_H__
#define __MRM_AIO_H__

#include "dIO.h"
#include "stm32l476xx.h"

void configure_ADCPin(GPIO_TypeDef* thisPORT, int pinNum);

void ADC_Config(ADC_TypeDef* ADCModule, int ADCCh);
void ADC_Init();
void ADC_ModuleInit(ADC_TypeDef* ADCModule, int ADCCh);

uint16_t getBattCounts();
float getBattVolts();

uint16_t readADC(ADC_TypeDef* ADCModule);

#endif
