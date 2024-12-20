/*
 * SFSU Engr 498 Final Project
 * Fall, 2024
 * Joshua Mehlman, Cannek Heredia, Vrishika Vijay Mohite
 *
 *  Data Logger
 */

#include "dIO.h"
#include "stm32l476xx.h"

#include "timing.h"
#include "serialHand.h"
#include "aIO.h"
#include "chData.h"
#include "SD_SPI_Driver.h"
#include "SPI.h"
#include "ff.h"
#include "dataCapTimer.h"
#include "RTC.h"

#define VERSIONNUM 0

// TODO: Set these global, or in an initializer
SPI_TypeDef *SPI_FS = SPI2;

USART_TypeDef *consolSerialPort = USART2; // USB to UART

// Safe btn
GPIO_TypeDef * safeBtnPORT = GPIOC; // The btn is port C, so is UART2
const int safeBtn_portNum = 2;
const unsigned long safeBtn_pin = 13;

// ADC
ADC_TypeDef *ADC_Module = ADC1;

int main() {
    char sendString[BUFFERSIZE];

	int sysRunMode = RUNMODE_SOFF;
  	configure_portNum(safeBtn_portNum); // Turn on the clock for the port
	configure_inPin(safeBtnPORT, safeBtn_pin, PIN_FLOATING);
  	sysRunMode = get_pinState(safeBtnPORT, safeBtn_pin);

	// The LED pin is configured here as the heartbeat
  	init_sysTiming(sysRunMode); // Start the SYSTICK Clock

	if(sysRunMode != RUNMODE_SAFE) // If we are not in safe mode do the rest of the init
	{
		// **** RTC  *** //
		// If we boot with the data start btn held, set the clock. Otherwise don't
    	configure_inPin(GPIOC, 11, PIN_PULLUP);
		if((RTC->ISR & RTC_ISR_INITS) == 0)
		{	// Do we have RTC already?
			RTC_Init();
		}

		// Do we want to set the clock
		if(get_pinState(GPIOC, 11) == 0)
		{
			RTC_Init();
			set_date(12, 13, 24);
			set_time(11, 12, 13);
		}

		sysRunMode = set_runMode(RUNMODE_SOFF);

	    // Initialize USART2 for communication with a PC via serial terminal.
	    // Configuration details:
	    // - Data format: 8 data bits, no parity, 1 start bit, and 1 stop bit
    	// USART2 tx = PA2, rx = PA3: PC Terminal
		float sToWait = 0.050;
		int br = 115200;
		consolSerialPort = USART2;
  		configure_portNum(0); // Turn on the clock for the port
    	configure_serialPin(GPIOA, 2 , 0x7);// tx pin
    	configure_serialPin(GPIOA, 3 , 0x7); // rx pin
		init_serialPort(consolSerialPort, 2, br);

		writeSerialString(consolSerialPort, "-----------------------------\n\r", 0);

		sprintf(sendString, "Starting DataLogger: v%i\r\n", VERSIONNUM); 
		writeSerialString(consolSerialPort, sendString, 0); 
		delay_s(sToWait);


        /***    Init ADC ****/
        sprintf(sendString, "Init ADC: ADC12_IN 6, PA1\r\n");
        writeSerialString(consolSerialPort, sendString, 0);
		delay_s(sToWait);

		configure_portNum(0); // turn the clock on for Port A
        configure_ADCPin(GPIOA, 1); // set up the pin
        ADC_Config(ADC_Module, 6);  // set up the adc


		// **** Shutdown() ***//
        sprintf(sendString, "Init Shutdown\r\n"); 
        writeSerialString(consolSerialPort, sendString, 0);
		delay_s(sToWait);
		init_Shutdown(); 		// initialize Shutdown items

		/***    Init The chData  ***/
		// File IO SPI is called by FatFS
		#ifndef DONOTMOUNT
        writeSerialString(consolSerialPort, "Init FS\n\r", 0);
		delay_s(sToWait);
		init_fileSystem();
		delay_s(sToWait);
		#endif

		//testDriver();

        writeSerialString(consolSerialPort, "Init Btn Interupts\n\r", 0);
		delay_s(sToWait);
		init_Interupts();
		delay_s(sToWait);

		// Don't init the timer untill last
        writeSerialString(consolSerialPort, "Init Data Timer\n\r", 0);
		delay_s(sToWait);
		init_DataTimer();
		delay_s(sToWait);

        sprintf(sendString, "Done with Init. Run Mode: %i\r\n", get_runMode()); 
        writeSerialString(consolSerialPort, sendString, 0);
		delay_s(sToWait);

		writeSerialString(consolSerialPort, "-----------------------------\n\r", 0); 
		delay_s(sToWait);
    }

    while(1) { __asm("WFI"); } // Go to sleep untill you are needed 
}
