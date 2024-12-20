#include "DMA.h"
#include <stdlib.h>

//Time2, Ch1 is DMA1, Ch5
void DMA_Configruation(void)
{
	// USART_TX: DMA1, CH7, 0b010
	// USART_RX: DMA1, CH6, 0b010

	// Enable the clock for DMA1
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN; 

	// Disable Enable DMAx to make settings changes
	DMA1_Channel7->CCR &= ~DMA_CCR_EN;
	//DMA1_Channel5->CCR &= MEM2MEM
	//DMA1_Channel5->CCR &= Priority level
	// Peripheral/Memory Data Size: 8 Bits
	// 00: 8 Bits
	// 01: 16 Bits
	// 10: 32 Bits
	// 11: Reserved
	DMA1_Channel7->CCR &= ~DMA_CCR_MSIZE;
	DMA1_Channel7->CCR &= ~DMA_CCR_PSIZE;
	// Periferal/Memory Increemnt mode: 0 = off, 1 = 0n
	// We will be sending multiple bytes 32/8 = we can do 2 at a time
	//DMA1_Channel7->CCR &= ~DMA_CCR_MINC;
	DMA1_Channel7->CCR |= DMA_CCR_MINC;
	DMA1_Channel7->CCR &= ~DMA_CCR_PINC;

	// Circular mode: 0 off, 1 on
	DMA1_Channel7->CCR &= ~DMA_CCR_CIRC;//Circlular mode
	//DMA1_Channel7->CCR |= DMA_CCR_CIRC;//Circlular mode

	// data transfer direction: 0 = read from peripheral, 1 = read from memory
	DMA1_Channel7->CCR |= DMA_CCR_DIR;

	// How much data to transfer.
	// This will be called from Serial Hand
	//DMA1_Channel7->CNDTR = 6;

	// Where do we want it to go
	// USART2 Transmit data register: 16 Bits size
	DMA1_Channel7->CPAR = (uint32_t)&(USART2->TDR);

	// This will be done in the timer register
	//DMA1_Channel7->CMAR = (uint32_t)Buffer_T;

	// Which DMA1 ch for TIM2_Ch1
	DMA1_CSELR->CSELR &= ~DMA_CSELR_C7S;
	DMA1_CSELR->CSELR |= 0b0010<<4*(7-1);

	// Transfer interupt enable
	//DMA1_Channel5->CCR |= DMA_CCR_TEIE;
	// Halv transfer interupt enable
	//DMA1_Channel5->CCR |= DMA_CCR_HTIE;

	// Transfer compleate inteupt enable
	// We will let the serial port know when to tx
	DMA1_Channel7->CCR |= DMA_CCR_TCIE;

	NVIC_EnableIRQ(DMA1_Channel7_IRQn);
	//NVIC_SetPriority(DMA1_Channel7_IRQn, 1);
	// We will enable in the timer
	//DMA1_Channel7->CCR |= DMA_CCR_EN; 
}

// Note, we never go here, the serial clears the interupts
void DMA1_Channel7_IRQHandler(void)
{
	// USART2_TX
	// Has our globla interup flag droped
	if((DMA1->ISR&DMA_ISR_GIF7) == DMA_ISR_GIF7)
	{
		// has our transfer compleat flag dropped?
		if((DMA1->ISR&DMA_ISR_TCIF7) == DMA_ISR_TCIF7)
		{
		    DMA1->IFCR |= DMA_IFCR_CTCIF7; // Clear the tx compleat
		    DMA1_Channel7->CCR &= ~DMA_CCR_EN; // Disable the DMA
	    }
	}
}