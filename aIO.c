/*
 * Handeler Functions for DIO 
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

#include "aIO.h"
#include "dIO.h"
//#include "serialHand.h"
#include "timing.h"

volatile uint16_t battVoltCounts = 0;

float getBattVolts()
{
    // Volts = VREF * Counts /(2^n - 1)
    const int ADCBits = 12;
    const float vRef = 3.3;
    int ADCMax = (1 << ADCBits) - 1;
    float volts = vRef * battVoltCounts / ADCMax;

    return volts;
}

uint16_t getBattCounts()
{
    return battVoltCounts;
}

uint16_t readADC(ADC_TypeDef* ADCModule)
{
    if (ADCModule->ISR & ADC_ISR_OVR) {
        ADCModule->ISR |= ADC_ISR_OVR; 
    } // Clear overrun flag

    ADCModule->CR |= ADC_CR_ADSTART; // start the ADC conversion
    while(!(ADCModule->ISR & ADC_ISR_EOC)){;} // Wait for the ADC conversion to be ready


    battVoltCounts = ADCModule->DR; // Page 606
    

    return battVoltCounts;
}

void configure_ADCPin(GPIO_TypeDef* thisPORT, int pinNum)
{
    // Pushpull is the default
	configure_pin(thisPORT, pinNum, PIN_ANALOG, PIN_PUSHPULL, PIN_FLOATING, 0);

}

// Set up for ADC1_IN 6, PA1
    //DIFSEL register for single or differential
    // 0 = Single, 1 = Diff
    //ADCx->DIFSEL
void ADC_Config(ADC_TypeDef* ADCModule, int ADCCh)
{
    // Enable the internal voltage refernece
    // Page 683
    VREFBUF->CSR &= ~VREFBUF_CSR_ENVR; // Enable the internal voltage reference
    VREFBUF->CSR |= VREFBUF_CSR_VRS;  // set = 2.5V
    //VREFBUF->CSR &= ~VREFBUF_CSR_VRS;  // clear = 2.048

    ADC_Init();

    //RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN; // 
    ADC_ModuleInit(ADCModule, ADCCh);


}

void ADC_Init()
{
    // RCC_CR: Page 223
    // HS Clock: 0 = HSC oscillator OFF, 1 = ON
    RCC->CR |= RCC_CR_HSION; // Turn on HSI clock
    RCC->CCIPR &= ~(0b11 << 28);
    RCC->CCIPR |= 0b01 << 28;
    RCC->CCIPR |= RCC_CCIPR_ADCSEL_1;
    while(!(RCC->CR & RCC_CR_HSIRDY)){;} // Wait for the clock to be ready

    // ADC Clock: Page 251: 0 = Disabled, 1 = Enabled

    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
}

void ADC_ModuleInit(ADC_TypeDef* ADCModule, int ADCCh)
{
    //// Set up for ADC3_IN 7, PF4 (NO! ADC3 is pain in rear)
    // Set up for ADC1_IN 6, PA1

    // Disable ADCx
    ADCModule->CR &= ~ADC_CR_ADEN;

    //    Split this off to do once, not per pin
    // Page 316
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_BOOSTEN; // Enable I/O analog switches voltage booster
    // Page 613
    ADC123_COMMON->CCR |= ADC_CCR_VREFEN;       // Enable conversion of interlal channels
    ADC123_COMMON->CCR &= ~ADC_CCR_PRESC;    // Set presaler to 0 = Not divided
    ADC123_COMMON->CCR |= ADC_CCR_CKMODE_0;     // 1 = Syncronous HCLK/1
    ADC123_COMMON->CCR &= ~ADC_CCR_DUAL;         // 0 = Independent ch

    // From ADCx_Wakeup() in text
    // ADC_CR: ADC Controll Register. Page 589
    // DEEPPWE = 0, not in deep power-down, 1 = In deep power-down
    if((ADCModule->CR & ADC_CR_DEEPPWD) == ADC_CR_DEEPPWD){ ADCModule->CR &= ~ADC_CR_DEEPPWD;} // Take the ADC Out of sleep
    // ADVREGEN: ADC Voltage regulator: Page 515
    ADCModule->CR |= ADC_CR_ADVREGEN;
    // Delay to let it settle
    delay_s(0.001);

    // Set resulution
    // ADC_CFGR page 592
    // 00 = 12-bits, 01 = 10-bits, 10 = 8-bits, 11 = 6-bits
    ADCModule->CFGR &= ~ADC_CFGR_RES;   // 00 = 12-bits
    ADCModule->CFGR &= ~ADC_CFGR_ALIGN; // 0 = right aligh, 1 = left
    ADCModule->CFGR &= ~ADC_CFGR_CONT; // Set discontinuous mode // CFGR Page 592

    // Page 602
    // regular ch conversion, How many chanles.  Set to N - 1
    // L[3:0]. 4 bits, so 8 ch Max
    ADCModule->SQR1 &= ~ADC_SQR1_L; // Clear the length field   
    ADCModule->SQR1 |= ADC_SQR1_L; // Clear the length field   

    // Set which ch is the first conversion
    // Cler the ch conversion sequence
    ADC1->SQR1 &= ~(ADC_SQR1_SQ1 | ADC_SQR1_SQ2 | ADC_SQR1_SQ3); // Clear SQ1, SQ2, SQ3
    ADCModule->SQR1 |= ADCCh<<ADC_SQR1_SQ1_Pos; // set chx for conversion

    // Page 610; 0 = single ended, 1 = differential
    ADCModule->DIFSEL &= ADC_DIFSEL_DIFSEL_6; // Change this to not define the port
    //ADCModule->DIFSEL &= ADC_DIFSEL_DIFSEL_7; // Change this to not define the port

    // Page 522, 598
    // Sample time: Bits 9:0, 
    // todo, don't hard code
    ADCModule->SMPR1 |= ADC_SMPR1_SMP6_2;  // Sample Time = 12.5 clock cycles
    // Trigger
    ADCModule->CFGR &= ~ADC_CFGR_EXTEN; // Software trigger


    // Apply calibration

    // Enable run
    ADCModule->CR |= ADC_CR_ADEN;
    while(!(ADCModule->ISR & ADC_ISR_ADRDY)){;} // Wait for the ADC to be ready
}
