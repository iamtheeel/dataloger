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

// screen /dev/ttyACM0

#include <stdint.h>
#include <string.h>

#include "serialHand.h"
#include "dIO.h"
#include "DMA.h"
#include "timing.h"


#include "stm32l476xx.h"



void init_serialPort(USART_TypeDef * USARTx, int portNum, int baudRate)
{   
    /*
     *  Untill we need another setting:
     *  Data Bits   = 8
     *  Parity      = None
     *  Stop Bits   = 1
     */
    // Note, change this to just take the port and figure it out from there

    if(portNum == 1)
    {   // USART1EN 
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;  // USART1 is bit 14
    }
    else 
    {
        // Enable the USARTx clock
        RCC->APB1ENR1 |= 1UL << (15 + portNum);  // USART2 is bit 17
    }

    // Set the system clock as the rource for UARTx
    // CCIPR port num bits, USART Clock Selection: PCLK (00), SYSCLK (01), HSI16 (10), LSE (11)
    RCC->CCIPR &= ~(3UL << (2*(portNum-1))); 
    RCC->CCIPR |=  (1UL << (2*(portNum-1)));

    // Serial Port Configurations: STM32L47xxx_Reference Manual_1903_pages.pdf
    // CR1: Controll 1, Page 1384 
    // CR2: Controll 2, Page 1387 
    // CR3: Controll 3, Page 1391 
    // BRR: Baud Rate, Page 1395, Also See Text 198.1.2, and Serial Tab of notes
    // ISR: Innterupt and Status, Page 1399 

    // CR1 Bit 1 UE, Enable Mode : 0 = Disabled, 1 = Enabled
    USARTx->CR1 &= ~USART_CR1_UE; // Disable the port

    // CR1 Bits 12, 28, M0, M1:  DataBits: 00 = 8, 01 = 9, 10 = 7
    USARTx->CR1 &= ~USART_CR1_M; // Databits = 8
    // CR1 Bit 9, PS, Parity: 0 = Even, 1 = Odd
    // CR1 Bit 10, PCE, Parity Enable: 0 = None, 1 = Use Parity (PS to select even/odd)
    USARTx->CR1 &= ~USART_CR1_PCE; // Parity = None
    // CR2 Bits 12, 13, STOP:  Stop Bits: 00 = 1, 01 = 0.5, 10 = 2, 11 = 1.5
    USARTx->CR2 &= ~USART_CR2_STOP; // Stop Bits = 1

    // CR1 Bit 15, OVER8, Oversample rate: 0 = 16, 1 = 8
    USARTx->CR1 &= ~USART_CR1_OVER8; // Oversampling = 16

    // BRR: (1+OVER8)*clockSpeed/baudRate
    int USARTDIV = clockFreq/baudRate;
    USARTx->BRR = USARTDIV; 

#ifndef USE_DMA
    // Interupt
    // We need this blocking or not
    // Enable rx not empty interupt
    USARTx->CR1 |= USART_CR1_RXNEIE;  
    // Disable tx register empty. We enable it when we want to send something
    USARTx->CR1 &= ~USART_CR1_TXEIE;  
    if(portNum < 4)      {NVIC_EnableIRQ(USART1_IRQn-1 + portNum);}
    else if(portNum == 4){NVIC_EnableIRQ(UART4_IRQn);}
#else
	USARTx->CR3 |= USART_CR3_DMAT; // Enabling DMA for transmission
    //USARTx->CR3 |= USART_CR3_DMAR; // Enabling DMA for reception

	//// Enabling USART after configuration is completed
#endif
    // Done with config, enable the port
    // CR1 Bit 3, TE, Transmitter : 0 = Disabled, 1 = Enabled
    // CR1 Bit 2, RE, Receiver : 0 = Disabled, 1 = Enabled
    USARTx->CR1 |= (USART_CR1_TE); // Enable TX
    USARTx->CR1 |= (USART_CR1_RE); // Enable RX
    USARTx->CR1 |= USART_CR1_UE; // Enable the port

    while((USARTx->ISR & USART_ISR_TEACK) == 0); // Wait for TX Ready: Bit 21
    while((USARTx->ISR & USART_ISR_REACK) == 0); // Wait for RX Ready: Bit 22

#ifdef USE_DMA
    // TODO: Config for each ch
    // Right now only USART2
    if(USARTx == USART2){ DMA_Configruation();}
#endif
}

void configure_serialPin(GPIO_TypeDef* thisPORT, int pinNum, uint8_t altFunNum)
{
	configure_pin(thisPORT,  pinNum, PIN_ALTERNATE, PIN_OPENDRAIN, PIN_PULLUP, altFunNum);

	// GPIOx->OSPEEDR, Speed: Low (00), Medium (01), High (10), Very High (11)
	thisPORT->OSPEEDR |= 0b10<<(2*pinNum);
}


void USART_readBlocking(USART_TypeDef *USARTx, char *buffer, uint32_t nBytes)
{
    for(int i = 0; i < nBytes; i++)
    {
        // Wait for a RXNE (Recive Not Empty)
        // An interrupt is generated if RXNEIE=1 in the USART_CR1 register.
        while(!(USARTx->ISR & USART_ISR_RXNE)) {;}
        buffer[i] = USARTx->RDR;  // The byte we have to read, clears RXNE
    }
}

void USART_writeBlocking(USART_TypeDef *USARTx, char *writeString, int writeNLCR)
{
    uint32_t nBytes = strlen(writeString);

	if(writeNLCR)
	{
    	sprintf(writeString, "%s\n\r", writeString); // add newline and carriage return
	}

	int i = 0;
	while(writeString[i]!='\0')
    {
        // Wait for a TX Enable
        while(!(USARTx->ISR & USART_ISR_TXE));
        // Write the bite in question
        USARTx->TDR = writeString[i++] & 0xFF;
    }
}

//need a buffer for each port
//int USART2_strLen = 0;
volatile int USART1_rxCounter = 0;
volatile int USART2_rxCounter = 0;
volatile int USART3_rxCounter = 0;
volatile int USART4_rxCounter = 0;
volatile int USART5_rxCounter = 0;

char USART2_Buffer_Tx[BUFFERSIZE*20]; 
char USART2_Buffer_Rx[BUFFERSIZE];
volatile int USART2_txCounter = 0;
volatile int USART2_rxDataStart = 0;

char USART4_Buffer_Tx[BUFFERSIZE+2];
char USART4_Buffer_Rx[BUFFERSIZE];
volatile int USART4_txCounter = 0;
volatile int USART4_rxDataStart = 0;

void USART_writeDMA(USART_TypeDef *USARTx, char *writeString, int writeNLCR)
{
	// Wait until the DMA interupt is not set (the last string is still being worked on)
    // We should not have to do this, we should be able to use strcat

    int timeout = 0xFF;
    //while((!(DMA1->ISR & DMA_ISR_TCIF7)) && timeout--);
    

	if(writeNLCR == 1) { sprintf(writeString, "%s\n\r", writeString); } // add newline and carriage return
	else if(writeNLCR == 2) { sprintf(writeString, "%s\r", writeString); } // add carriage return

    // TODO: Make a double buffer
    // Must make second string, cuz we going to move on and will overwrite the old
    strcpy(USART2_Buffer_Tx, writeString); // change to pointer in the call

    // How many bytes are we goint to send
    uint32_t nBytes_str = strlen(USART2_Buffer_Tx);
    DMA1_Channel7->CNDTR = nBytes_str;

    // Were is the data coming from
    DMA1_Channel7->CMAR = (uint32_t)USART2_Buffer_Tx;

    // Enable the DMA interupt
    DMA1_Channel7->CCR |= DMA_CCR_EN;
}

// Load the string into the buffer for the interupt
void USART_writeInterupt(USART_TypeDef *USARTx, char *writeString, int writeNLCR)
{
	// Wait until the TXE (Transmit Data Register Empty) flag is set, signaling that USART is ready to transmit new data.
    uint8_t timeout = 0xFF;
	while(!(USARTx->ISR & USART_ISR_TXE) && timeout--); 

    //don't seem to needthis 
    //while(USARTx->CR1 & USART_CR1_TXEIE); //wait untill the TX Interupt is cleared

    // add newline and carriage return
	if(writeNLCR == 1) { sprintf(writeString, "%s\n\r", writeString); }
	else if(writeNLCR == 2) { sprintf(writeString, "%s\r", writeString); } // add carriage return

    USARTx->CR1 |= USART_CR1_TXEIE; // enable the tx interupt
    //USARTx->TDR = writeString[0]; // The interupt will trigger on the first byte send
    if(USARTx == USART2) // there is a better way
    {
        // TODO: Check buffer lenth
        //strcpy(USART2_Buffer_Tx, writeString); // change to pointer in the call
        strcat(USART2_Buffer_Tx, writeString); // we have not finished the last string
        USARTx->TDR = writeString[USART2_txCounter++] & 0xFF; // The interupt will trigger on the first byte send
    }
    else if(USARTx == UART4)
    {
        strcpy(USART4_Buffer_Tx, writeString); // change to pointer in the call
        //USARTx->TDR = writeString[0] & 0xFF; // The interupt will trigger on the first byte send
    }

    //USARTx->TDR = writeString[0] & 0xFF; // The interupt will trigger on the first byte send
}

// Called from the interupt
void send_serialString(USART_TypeDef* USARTx, char* buffer, volatile int* counter)
{
    /*
     * From TextBook
     */
    //(*counter)++; // we already wrote 0
    if(USARTx->ISR & USART_ISR_TXE)
    {
	    if(buffer[*counter]!='\0')
        {
            USARTx->TDR = buffer[*counter] & 0xFF;
            (*counter)++; // we already wrote 0
        }
        else {
            strcpy(buffer, ""); // Zero out the buffer in fin
            (*counter) = 0;
            USARTx->CR1 &= ~USART_CR1_TXEIE;// Disable the tx interupt
        }
    }

}

void read_serialString(USART_TypeDef* USARTx, char* buffer, volatile int* counter, int sensorType)
{
    /*
     * From TextBook
     */

     /* 
      * USART_ISR RXNE It is cleared by a read to the USART_RDR register. 
      *  The RXNE flag can also be cleared by writing 1 to the RXFRQ in the USART_RQR register.
      */
     if(USARTx->ISR & USART_ISR_RXNE)
     {
        buffer[*counter] = USARTx->RDR; // Reading clears the RXNE flag
        (*counter)++;
        if((*counter) >= BUFFERSIZE) {*counter = 0;}

     }
}

void USART2_IRQHandler(void)
{
    // Do we have anything to send?
    if(USART2->CR1 &USART_CR1_TXEIE)
    {
        send_serialString(USART2, USART2_Buffer_Tx, &USART2_txCounter);
    }

	uint8_t data;
	// Check if the RXNE (Receive Not Empty) interrupt is triggered, indicating new data is available in the USART_RDR register.
	if(USART2->ISR & USART_ISR_RXNE) {	
		// Read data from the receiver data register (RDR), which also clears the RXNE flag.		
		data = USART2->RDR;         
		
		// Wait until the TXE (Transmit Data Register Empty) flag is set, signaling that USART is ready to transmit new data.
		while(!(USART2->ISR & USART_ISR_TXE));
		
		// Do something
	} 


    // CLear the OverRun
    if(USART2->ISR & USART_ISR_ORE)
    {
        USART2->ICR &= ~USART_ICR_ORECF_Msk; 
    }

    NVIC_ClearPendingIRQ(USART2_IRQn);
}

void UART4_IRQHandler(void)
{
    if(UART4->ISR & USART_ISR_ORE)
    {
        // Overrun Error
        UART4->ICR &= ~USART_ICR_ORECF_Msk; // CLear the OverRun
    }
    NVIC_ClearPendingIRQ(UART4_IRQn);  //NVIC->ICPR
}
