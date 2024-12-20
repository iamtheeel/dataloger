/*
* Handler Functions for SPI
* Cannek Heredia
* ENGR 478, Fall 2024
*/

#include "SPI.h"
#include "dIO.h"
#include "stm32l476xx.h"
#include "serialHand.h"

void SPI_Init(int SPI_int){
    // Change to hand the port, not the port number
    switch (SPI_int) {
        case 1: 
            // Dev as needed
            //configure_SPI_Pin(SPI_int);
            break;
        case 2:
            RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN;       // Enable the SPI clock
            RCC->APB1RSTR1 |= RCC_APB1RSTR1_SPI2RST;    // Reset SPI2
            RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_SPI2RST;   // Clear the Reset SPI2

            RCC->AHB2ENR |= 1L<<1; // Enable the clock for GPIOB
            RCC->AHB2ENR |= 1L<<2; // Enable the clock for GPIOC
            // Note: MicroSD Breakout has pullups on all pins
            configure_SPI_Pin(GPIOB, 10, PIN_FLOATING, 5); // SCK
            configure_SPI_Pin_In(GPIOC, 2, PIN_FLOATING, 5); // POCI
            configure_SPI_Pin(GPIOC, 3, PIN_FLOATING, 5); // PICO
            configure_outPin(GPIOB, 9, PIN_FLOATING); // Cant set HW for the file IO

            configure_SPI(SPI2);

            break;

    }
}

// GPIOx->OSPEEDR, Speed: Low (00), Medium (01), High (10), Very High (11)
void configure_SPI_Pin(GPIO_TypeDef* thisPORT, int pinNum, int puUpDw, uint8_t altFunNum)
{
    configure_pin(thisPORT,  pinNum, PIN_ALTERNATE, PIN_PUSHPULL, puUpDw, altFunNum);
	thisPORT->OSPEEDR |= 0b10<<(2*pinNum);
}
void configure_SPI_Pin_In(GPIO_TypeDef* thisPORT, int pinNum, int puUpDw, uint8_t altFunNum)
{
    configure_pin(thisPORT,  pinNum, PIN_ALTERNATE, PIN_OPENDRAIN, puUpDw, altFunNum);
	thisPORT->OSPEEDR |= 0b10<<(2*pinNum);
}

void configure_SPI(SPI_TypeDef* SPI)
{
    // Configure SPI
    // Settings for the FileIO
    SPI->CR1 &= ~SPI_CR1_SPE; // Disable the SPI

    // Phase and polarity must be matched
    // For basic card reader: 0,0 
    // For + card reader    :
    // Clock Phase polarity, Bit 0, 0
    SPI->CR1 &= ~SPI_CR1_CPHA; // 0, 0
    SPI->CR1 &= ~SPI_CR1_CPOL;
    //SPI->CR1 &= ~SPI_CR1_CPHA; // 0, 1
    //SPI->CR1 |= SPI_CR1_CPOL ;
    //SPI->CR1 |= SPI_CR1_CPHA;  // 1,0
    //SPI->CR1 &= ~SPI_CR1_CPOL;
    //SPI->CR1 |= SPI_CR1_CPHA;  // 1,1
    //SPI->CR1 |= SPI_CR1_CPOL ;
    //SPI->CR1 &= ~(SPI_CR1_CPHA |SPI_CR1_CPOL)  ;// Clock Phase polarity, Bit 0, 0
    SPI->CR1 |= SPI_CR1_MSTR;             // Master mode
    // For 4Mhz fPCLK, 16 or 32 for 100khz< rate< 400khz
    SPI->CR1 &= ~SPI_CR1_BR_Msk;             // BR bits 5:3
    //SPI->CR1 |= SPI_CR1_BR_1;//fPCLK/8  too fast for Logic Ana // BR bits 5:3
    SPI->CR1 |= 0b011 << 3;//fPCLK/16             // BR bits 5:3
    //SPI->CR1 |= 0b100 << 3;//fPCLK/32             // BR bits 5:3
    // Bit 6 is enable
    SPI->CR1 &= ~SPI_CR1_LSBFIRST;        // Bit order LSBFIRST bit 7: MSBFirst = 0
    SPI->CR1 |= SPI_CR1_SSI | SPI_CR1_SSM;  // Software periferal management Bits 8, 9
    SPI->CR1 &= ~SPI_CR1_RXONLY;             // 0 = Full Duplex mode, Bit 10
    // CRC bits 14:11
    SPI->CR1 &= ~SPI_CR1_CRCEN;    //Disable hardware CRC, Bit 13
    //SPI->CR1 |= SPI_CR1_BIDIOE;             // Enable bidirectional mode, Bit 14; 1 = output enabled
    SPI->CR1 &= ~SPI_CR1_BIDIMODE;             // Bidirectional mode, Bit 15: 0 = 2line

    SPI->CR1 &= ~ SPI_CR1_CRCEN;                //Disable hardware CRC, Bit 13
    SPI->CR1 |= SPI_CR1_BIDIOE;                 // Enable bidirectional mode, Bit 14, 1 = output enabled
    SPI->CR1 &= ~SPI_CR1_BIDIMODE;              // Bidirectional mode, Bit 15, 0 = 2-line, unidirectional

    //SPI->CR2 |= ;       // , Bit 
    SPI->CR2 &= ~SPI_CR2_NSSP;       // NSS pulse management, Bit 3: 0 = no pulse
    SPI->CR2 &= ~SPI_CR2_FRF;//Basic=0       // Frame format, Bit 4: 0=Motorola, 1 = TI
    SPI->CR2 |= SPI_CR2_ERRIE;      // Overrun Interupt Enable: 1 = enabled
    //
    SPI->CR2 &= ~SPI_CR2_DS;       // Data Size, Bit 11:8
    SPI->CR2 |= 0b0111 << 8;       // Data Size, 0b0111=8-bit
    SPI->CR2 |= SPI_CR2_FRXTH;       //RXNE flag , Bit 12: 0=16bit, 1=8bit

    SPI->CR1 |= SPI_CR1_SPE; // Enable the SPI

    volatile uint8_t temp = SPI->DR;            // Read to clear any residual data
}

uint8_t SPI_Send(SPI_TypeDef *SPI, uint8_t data)
{
    while ((SPI->SR & SPI_SR_TXE) != SPI_SR_TXE);     // Wait until TX Buffer Empty is set
    *((volatile uint8_t*)&SPI->DR) = data;  // Send data (triggers read)

    uint32_t timeout = 0xFFFF; // Set a timeout value
    while (((SPI->SR & SPI_SR_RXNE) != SPI_SR_RXNE) && --timeout); // Wait until RXNE is set
    if(timeout == 0) writeSerialString(USART2, "SPI_Send: Timeout\n\r", 0); 

    uint8_t read = *((volatile uint8_t*)&SPI->DR);

    while ((SPI->SR & SPI_SR_BSY) == SPI_SR_BSY);     // Wait until not BSY is set
    return read;                      // Return received data
}


void SPI2_IRQHandler(void)
{
    // I have never seen this hit
    writeSerialString(USART2, "*** SPI2 IRQ ***\n\n", 0); 
    if (SPI2->SR & SPI_SR_OVR) {
        uint8_t temp = 0;
        // Clear OVR by reading DR and SR
        temp = SPI2->DR;  
        temp = SPI2->SR;
    }
}

