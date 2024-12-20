/*
 * Handeler Functions for DIO 
 * Joshua Mehlman
 * Engr 478, Spring 2024
 */

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42, phk@FreeBSD.ORG):
 * <iamtheeel> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   MRM
 * ----------------------------------------------------------------------------
 */
#ifndef __DIO_H__
#define __DIO_H__

#include "stm32l476xx.h"

/*
	Port num: A-H = 0 - 7, pinNum 0-15
	Input Mode: 0b00 = 0 = input mode
							0b01 = 1 = output
	            0b10 = 2 = Alternate function
							ob11 = 3 = Analog
							##TODO## the input modes
	Open Drain:	0 = PushPull
							1 = Open Drain
  pUpDOWN:		0b00 = 0 = Floating
							0b01 = 1 = Pull UP
							0b10 = 2 = Pull Down
							0b11 = 3 = Reserved
 */
 // GPIOx->MODER
#define PIN_GPDIN	  (0UL)
#define PIN_GPOUTPUT  (1UL)
#define PIN_ALTERNATE (2UL)
#define PIN_ANALOG    (3UL)
// GPIOx->OTYPER
#define PIN_PUSHPULL  (0UL)
#define PIN_OPENDRAIN (1UL)
// GPIOx-> PUPDR
#define PIN_FLOATING  (0UL)
#define PIN_PULLUP    (1UL)
#define PIN_PULLDOWN  (2UL)
 
GPIO_TypeDef* get_port(int portNum);
GPIO_TypeDef* configure_portNum(int portNum);

void configure_pin(GPIO_TypeDef* thisPORT, int pinNum, int pinMode, int openDrain, int pUpDown, int altFunNum);

void configure_outPin(GPIO_TypeDef* thisPORT, int pinNum, int openDrain); // will call configure_pin
void configure_inPin(GPIO_TypeDef* thisPORT, int pinNum, int pUpDown); // will call configure_pin

int get_pinState(GPIO_TypeDef* thisPORT, int pinNum);
void set_PinState(GPIO_TypeDef* thisPORT, int pinNum, int cmdState);
void toggle_pin(GPIO_TypeDef* thisPORT, int pinNum); 
 
void init_EXTI(GPIO_TypeDef* thisPORT, int portNum, int pinNum, int trigRis, int trigFal);
 
#endif