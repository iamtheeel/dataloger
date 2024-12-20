/*
 * The data capture timer
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


#include "dataCapTimer.h"
#include "serialHand.h"
#include "chData.h"
#include "timing.h"
#include "aIO.h"

void init_DataTimer()
{
    //1. Enable the clock to TIM2 by configuring RCC_APB1ENR1 register
    RCC->APB1ENR1   |= RCC_APB1ENR1_TIM2EN;

    //2. Configure TIM2 counting mode to upcounting (TIMx_CR1) 
    TIM2->CR1 &= ~TIM_CR1_DIR;

    //3. Configure TIM2 Prescalar (TIMx_PSC) and ARR (TIMx_ARR) to generate desired counter period 
    TIM2->PSC = 39;         // Prescaler = 3 
    TIM2->ARR = 100000-1;   // Note: there are many possible combinations of PSC and ARR. This is just one solution

    //4. Configure TIMx_CCMR1 CC1S[1:0] bits to output mode for TIM2 Channel 1
    TIM2->CCMR1 &= ~TIM_CCMR1_CC1S;

    //5. Configure TIMx_CCMR1 OC1M[3:0] bits to Toggle mode ('0011') for TIM2 Channel 1
    TIM2->CCMR1 &= ~TIM_CCMR1_OC1M;  // Clear ouput compare mode bits for channel 1
    TIM2->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0; // OC1M = 0011 for Toggle Mode output on ch1
    TIM2->CCR1  = 0;

    //6. Set TIMx_CCER CC1E bit to enable output signal on Channel 1
    TIM2->CCER |= TIM_CCER_CC1E;

    // 7. Enable related interrupts by
    //      i. configuring TIMx_DIER register 
    //    ii. enable TIM2 interrupt source in NVIC
    TIM2->DIER |= TIM_DIER_CC1IE;       // Enable Capture/Compare interrupts for channel 1
    //TIM2->DIER |= TIM_DIER_UIE;           // Enable update interrupts
    NVIC_EnableIRQ(TIM2_IRQn);      // Enable TIM4 interrupt in NVIC

    //TIM2->CR1  |= TIM_CR1_CEN; // Enable timer is done when we want to start taking data
    //writeSerialString(consolSerialPort, "Done with timer2, ch 1\r\n", 0); delay_s(.1);
}



//TIM2 ISR
void TIM2_IRQHandler(void) {
    // Check if CC1 interrupt is triggered
    if((TIM2->SR & TIM_SR_CC1IF) == TIM_SR_CC1IF) 
    {
        int sysRunMode = get_runMode();
        char debugStr[128];
        //sprintf(debugStr, "Data Send Interupt: runMode = %i\r\n", sysRunMode);
        //writeSerialString(consolSerialPort, debugStr, 0); 

        readADC(ADC1);
        set_chData(0, getBattVolts());

        int echoData = 1;
        int writeData = 1;
        #ifdef DONOTMOUNT
        writeData = 0;
        #endif
        FRESULT writeStatus = write_chData(echoData, writeData); 
        if(writeStatus != FR_OK)
        {
            fs_error(writeStatus);
            sprintf(debugStr, "FileSystem Write Fail: %i \r\n", writeStatus);
            writeSerialString(consolSerialPort, debugStr, 0); // add newline and carriage return
        }
        //else { writeSerialString(consolSerialPort, "write_chData returened OK\r\n", 0);} // add newline and carriage return 
            
        TIM2->SR &= ~TIM_SR_CC1IF; // Clear the UIF Flag
    }
}