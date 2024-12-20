/*
* Handler Functions for SPI
* Cannek Heredia
* ENGR 478, Fall 2024
*/

#ifndef __SPI_H__
#define __SPI_H__

#include "stm32l476xx.h"


void SPI_Init(int SPI_int);
void configure_SPI_Pin(GPIO_TypeDef* thisPORT, int pinNum, int puUpDw, uint8_t altFunNum);
void configure_SPI_Pin_In(GPIO_TypeDef* thisPORT, int pinNum, int puUpDw, uint8_t altFunNum);
void configure_SPI(SPI_TypeDef* SPI);

uint8_t SPI_Send(SPI_TypeDef *SPI, uint8_t data);

#endif