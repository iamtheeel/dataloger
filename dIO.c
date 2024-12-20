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
 * this stuff is worth it, you can buy me a beer in return.   -- Theeel
 * ----------------------------------------------------------------------------
 */
 
#include "dIO.h"
#include "stm32l476xx.h"

int btnCount = 0;

/*****          Configurations        ***********/
GPIO_TypeDef* get_port(int portNum){
	GPIO_TypeDef * thisPORT;
	switch (portNum){
		case 0: // Port A
			thisPORT = GPIOA; // GPIOA      --> #define GPIOA ((GPIO_TypeDef *) GPIOA_BASE)
					  // GPIOA_BASE --> #define GPIOA_BASE (AHB2PERIPH_BASE + 0x0000U)
			break;
		case 1: thisPORT = GPIOB;break;
		case 2: thisPORT = GPIOC;break;
		case 3: thisPORT = GPIOD;break;
		case 4: thisPORT = GPIOE;break;
		case 5: thisPORT = GPIOF;break;
		case 6: thisPORT = GPIOG;break;
		case 7: thisPORT = GPIOH;break;
		//default:
			// unexpedted port
	}
	
	return thisPORT;
}

GPIO_TypeDef* configure_portNum(int portNum)
{
	GPIO_TypeDef *thisPORT = get_port(portNum); //Set which port we are using
 
	// 1. Enable the clock to GPIO Port x	
	// The closck is set in the RC_AHB2ENR register
	//RCC->AHB2ENR |= 0x00000001;					  // Hard code to known value
	//RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;  //Using define in stm32l476xx.h
	RCC->AHB2ENR |= 1L<<portNum;						//Use the port number passed as an argument
 return thisPORT;
}
 
void configure_pin(GPIO_TypeDef* thisPORT, int pinNum, int pinMode, int openDrain, int pUpDown, int altFunNum) {
    unsigned long clearMask1bit = 1UL << pinNum; // 0b1 shifted Pin as each pin uses 1 bit
    unsigned long clearMask2bit = 3UL << 2*pinNum; // 0b11 shifted 2xPin as each pin uses 2 bits
		
	// GPIO->MODER: Input(00), Output(01), AlterFunc(10), Analog(11)
	// Default = Analog(11) 
	thisPORT->MODER &= ~clearMask2bit; // Clear whatever is in both bits we are after by AND with the inverse of the clear mask
	thisPORT->MODER |= (unsigned long)pinMode << 2*pinNum;;		// Set what we want to by OR with the set mask

	// GPIOx_OTYPER : Output push-pull (0), Output open drain (1) 
	// Default = push-pull (0)
	thisPORT->OTYPER &= ~clearMask1bit;
	thisPORT->OTYPER |= (unsigned long)openDrain<<pinNum;

	// GPIOx_OSPEEDR: Low Speed (00), Meduim (01), High (10), Very High (11)
	
	// GPIOx_PUPDR: floating (00), Pull-up (01), Pull-down (10), Reserved (11)
	// Default = 
	thisPORT->PUPDR &= ~clearMask2bit; // Clear whatever is in both bits we are after by AND with the inverse of the clear mask
	thisPORT->PUPDR |= (unsigned long)pUpDown << 2*pinNum;

	// GPIOx_LCKR, Configuration lock register

	// GPIOx_AFRL, Alternate Function Low register
	// GPIOx_AFRH, Alternate Function Low register
	if(altFunNum)
	{
		int afrPort = 0;
		int afrPin =  pinNum;
		if(pinNum > 7) {
			afrPort = 1;
			afrPin -= 8;
		}
		thisPORT->AFR[afrPort] &= ~(0xFUL<<(4*afrPin)); // Clear
		thisPORT->AFR[afrPort] |=  (altFunNum<<(4*afrPin));
	}

	// GPIOx_BRR, Port bit reset 

	// GPIOx_ASCR: Port Analog Switch Control, 0 = Disconnect, 1 = Connect
	// Default = 0: Disconnect from ADC
	if(pinMode == PIN_ANALOG)
	{
		thisPORT->ASCR |= (1U<<pinNum);
	}
}

void configure_outPin(GPIO_TypeDef* thisPORT, int pinNum, int openDrain) {
	// GPIOx_MODER: to 'Output': Output(01) : PIN_GPOUTPUT
	// GPIOx_PUPDR: floating (00) : PIN_FLOATING
	configure_pin(thisPORT, pinNum, PIN_GPOUTPUT, openDrain, PIN_FLOATING, 0);
}

void configure_inPin(GPIO_TypeDef* thisPORT, int pinNum, int pUpDown) {
	// GPIOx_MODER: to 'Input': Input(00) : PIN_GPIN
	// GPIOx_OTYPER : Output push-pull (0) : PIN_PUSHPULL
	configure_pin(thisPORT, pinNum, PIN_GPDIN, PIN_PUSHPULL, pUpDown, 0);
}

/*****          Interupts        ***********/
void init_EXTI(GPIO_TypeDef* thisPORT, int portNum, int pinNum, int trigRis, int trigFal)
{
    // Enable the interupt clock
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Set the trigger source
	int EXTIRC_num = 0; 					// EXTIRC1: EXTICR[0] Pins 0:3
	if(pinNum >= 12) {EXTIRC_num = 3;} 		// EXTIRC3: EXTICR[3] Pins 12:15
	else if(pinNum >= 8) {EXTIRC_num = 2;} 	// EXTIRC3: EXTICR[2] Pins 8:11
	else if(pinNum >= 4) {EXTIRC_num = 1;} 	// EXTIRC2: EXTICR[1] Pins 4:7
	int EXTICR_SHIFT = 0;
	switch(pinNum)
	{
		// 12, 8, 4, 0 are shift 0U
		case 13: case 9: case 5: case 1:
			EXTICR_SHIFT = (4U); break;
		case 14: case 10: case 6: case 2:
			EXTICR_SHIFT = (8U); break;
		case 15: case 11: case 7: case 3:
			EXTICR_SHIFT = (12U); break;
	}

	int SYSCFG_EXTICR_EXTI = 0b1111UL<<EXTICR_SHIFT;

    SYSCFG->EXTICR[EXTIRC_num] &= ~SYSCFG_EXTICR_EXTI; // Clear the block 0111 << pin group
    SYSCFG->EXTICR[EXTIRC_num] |=  portNum<<EXTICR_SHIFT; // The port portNum<<PinGroupShift


    // Enable raising/falling edge add edge to function call
	if(trigRis) {EXTI->RTSR1 |= (1UL<<pinNum);}// Enable raising edge 
    if(trigFal) {EXTI->FTSR1 |= (1UL<<pinNum);} // Enable falling edge

	// Interupt Mask Register
	// Page 406:IMR1 IM0 - IM31
	// Page 409:IMR2 IM32 - IM40
    EXTI->IMR1 |= (1UL << pinNum);    // Enable the interrupt register for pin 

    // Set the priority
    NVIC_SetPriority(EXTI15_10_IRQn, 1);

    // Enable the interupt for pins 10 through 15
	int irqNum = 6; // Ch0 IRQn = 6
	if(pinNum <= 4){irqNum += pinNum;}
	else if(pinNum < 10){irqNum = EXTI9_5_IRQn;} // 5-9 shared
	else{irqNum = EXTI15_10_IRQn;}				// 10-15 shared
	NVIC_EnableIRQ(irqNum);
}


/*****          Get pin states   ***********/
void set_PinState(GPIO_TypeDef* thisPORT, int pinNum, int cmdState){	
	// BSRR sets just that pin, this is faster
	if(cmdState)thisPORT->BSRR = (1UL<<  pinNum); //Turn on pinNum
	else        thisPORT->BSRR = (1UL<< (pinNum + 16)); //Turn off pinNum
}

// Modular function to toggle the LD2 LED.
void toggle_pin(GPIO_TypeDef* thisPORT, int pinNum){	
	thisPORT->ODR ^= 1UL<<pinNum; //Toggle off pinNum
}

int get_pinState(GPIO_TypeDef* thisPORT, int pinNum){
	unsigned long inputMask  = 1UL<<pinNum;
	unsigned long pinStatus = thisPORT->IDR & inputMask; // Read the pinstate value

	return pinStatus == inputMask; // if equal to mask, is on
}
