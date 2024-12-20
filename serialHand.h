/*
 * Serial Communications
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

/*
 * linux: screen /dev/ttyACM0 9600
 *          exit <ctrl>a  then <ctrl>d
 * mac:
 *          exit <ctrl>a and  <ctrl>k
 */

#ifndef __MRM_SERIAL_H__
#define __MRM_SERIAL_H__

#include "stm32l476xx.h"

#include "dIO.h"

#include <stdint.h>
#include <stdio.h>

// Serial port
extern USART_TypeDef *consolSerialPort;// = USART2; // USB to UART

#define BUFFERSIZE 2048

// For testing the different kinds of sends:
//#define USE_BLOCKING
//#define USE_INTERUPT
#define USE_DMA 

#ifdef USE_BLOCKING
    #define writeSerialString USART_writeBlocking
#elif defined(USE_INTERUPT)
    #define writeSerialString USART_writeInterupt
#elif defined(USE_DMA)
    //#define writeSerialString USART_writeInterupt
    #define writeSerialString USART_writeDMA
    //void configure_DMA();
#endif

void init_serialPort(USART_TypeDef * USARTx, int portNum, int baudRate);
void configure_serialPin(GPIO_TypeDef* thisPORT, int pinNum, uint8_t altFunNum);

void USART_readBlocking(USART_TypeDef *USARTx, char *buffer, uint32_t nBytes); // Testing only not safe!

void USART_writeBlocking(USART_TypeDef *USARTx, char *writeString, int writeNLCR);
void USART_writeInterupt(USART_TypeDef * USARTx, char *writeString,int writeNLCR);
void USART_writeDMA(USART_TypeDef * USARTx, char *writeString,int writeNLCR);


#endif
