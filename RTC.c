#include "RTC.h"
#include "stm32l476xx.h"
#include <stdint.h>
#include <stdio.h>
//#include "serialHand.h"

uint8_t hours;
uint8_t minutes;
uint8_t seconds;
uint8_t month;
uint8_t day;
uint8_t year;

void RTC_Clock_Enable(){
    // enable write access to the back up domain
    if ((RCC->APB1ENR1 & RCC_APB1ENR1_PWREN) == 0) {
        // Enable the power interface clock
        RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
        // Add a short delay after anabling an RCC peripheral clock
        (void) RCC->APB1ENR1;
    }

    // Select LSE as the RTC Clock Source
    // RTC/LCD clock: (1) LSE is in the backup domain
    //                (2) HSE and LSI are not
    if((PWR->CR1 & PWR_CR1_DBP) == 0){
        // Enable write access to RTC and registers in the backup domain
        PWR->CR1 |= PWR_CR1_DBP;
        // Wait until the back up domain write protection has been disabled
        while ((PWR->CR1 & PWR_CR1_DBP) == 0);
    }

    // Reset LSEOW and LSEBYP bits before configuring LSE
    // BDCR = Backup Domain Control Register
    RCC->BDCR &= ~(RCC_BDCR_LSEON | RCC_BDCR_LSEBYP);

    // RTC clock selection can only be changed if the backup domain is reset
    RCC->BDCR |= RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;

    // wait until the LSE clock is ready
    RCC->BDCR |= RCC_BDCR_LSEON;
    while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0) {; }

    // Select LSE as the RTC Clock source
    // RTCSEL[1:0]: 00 = no clock, 01 = LSE, 10 = LSI, 11 = HSE
    RCC->BDCR &= ~RCC_BDCR_RTCSEL;  // Clear RTCSEL bits
    RCC->BDCR |= RCC_BDCR_RTCSEL_0; // Select LSe as RTC clock


    // Disable power interface clock
    RCC->APB1ENR1 &= ~RCC_APB1ENR1_PWREN;

    // Enable RTC clock
    RCC->BDCR |= RCC_BDCR_RTCEN; // Enable the clock of RTC

    RTC->ISR &= ~RTC_ISR_RSF;        // Clear RSF flag
    while ((RTC->ISR & RTC_ISR_RSF) == 0);

}

void RTC_Init(){
    // Enable RTC clock
    RTC_Clock_Enable(); // Select LSE as the clock

    // Disable write protection of RTC registers by writing disarm keys
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    // Enter initialization mode to program TR and DR registers
    RTC->ISR |= RTC_ISR_INIT;
    while((RTC->ISR & RTC_ISR_INITF) == 0);


    // hour format: 0 = 24 hour/day; 1 = AM/PM hour
    RTC->CR &= ~RTC_CR_FMT;

    // Generate a 1-Hz clock for the RTC time counter
    // LSE = 32.768 kHz = 2^15 Hz
    RTC->PRER = (127U << 16) | 255U; // Set PREDIV_A and PREDIV_S

    // Exit the initialization
    // Wait until INITF has been set
    RTC->ISR &= ~RTC_ISR_INIT;

    // wait for synchroization
    RTC->ISR &= ~RTC_ISR_RSF;
    while ((RTC->ISR & RTC_ISR_RSF) == 0);

    // Enable write protection for RTC registers
    RTC->WPR = 0xFF;
}

static uint8_t BCD_to_Decimal(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// Function to convert decimal numbers to BCD
// this is for setting our values to TR and DR in RTC
static uint8_t Decimal_to_BCD(int Dec){
    uint8_t bcd = 0;
    bcd |= ((Dec / 10) << 4);
    bcd |= (Dec % 10);
    return bcd;
}

uint32_t get_time32 (){
    uint32_t time_word;
    while ((RTC->ISR & RTC_ISR_RSF) == 0);
    uint8_t hr = (RTC->TR >> 16) & 0x3F;
    uint8_t min = (RTC->TR >> 8) & 0x7F;
    uint8_t sec = RTC->TR & 0x7F;
    uint8_t month = (RTC->DR >> 8) & 0x1F;
    uint8_t day = RTC->DR & 0x3F;
    uint8_t year = (RTC->DR >> 16) & 0xFF;

    hr = BCD_to_Decimal(hr);
    min = BCD_to_Decimal(min);
    sec = BCD_to_Decimal(sec);
    month = BCD_to_Decimal(month);
    day = BCD_to_Decimal(day);
    year = BCD_to_Decimal(year);

    int actual_year = (2000 + (int)year) - 1980;

    time_word |=  (actual_year << 25) | (month << 21) | (day << 16) | (hr << 11) | (min << 5) | sec >> 1;

    return  time_word;
}

void get_time(char *time){

    while ((RTC->ISR & RTC_ISR_RSF) == 0);
    uint8_t hr = (RTC->TR >> 16) & 0x3F;
    uint8_t min = (RTC->TR >> 8) & 0x7F;
    uint8_t sec = RTC->TR & 0x7F;
    float sub =  ((float)((RTC->PRER & RTC_PRER_PREDIV_S_Msk) - ((RTC->SSR) & RTC_SSR_SS_Msk)) / ((RTC->PRER & RTC_PRER_PREDIV_S_Msk) + 1));

    hr = BCD_to_Decimal(hr);
    min = BCD_to_Decimal(min);
    sec = BCD_to_Decimal(sec);

    sprintf(time, "%02d:%02d:%02d.%03.0f", hr, min, sec, (sub*1000));
}

void get_date(char *date){

    while ((RTC->ISR & RTC_ISR_RSF) == 0);
    uint8_t month = (RTC->DR >> 8) & 0x1F;
    uint8_t day = RTC->DR & 0x3F;
    uint8_t year = (RTC->DR >> 16) & 0xFF;

    month = BCD_to_Decimal(month);
    day = BCD_to_Decimal(day);
    year = BCD_to_Decimal(year);

    sprintf(date,"%02d/%02d/%02d", month, day, year);
}

// Set time for RTC TR
void set_time(int hh, int mm, int ss){
    // Check values in range 
    if( hh >= 0 && hh <= 24 && mm >= 0 && mm <= 60 && ss >= 0 && ss <= 60){
        // Convert Decimal to BCD
        hours = Decimal_to_BCD(hh);
        minutes = Decimal_to_BCD(mm);
        seconds = Decimal_to_BCD(ss);
    }

    // Disable write protection
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    // Enter initialization mode
    RTC->ISR |= RTC_ISR_INIT;
    while ((RTC->ISR & RTC_ISR_INITF) == 0);

    // Write date to RTC_DR register
    RTC->TR = (hours << RTC_TR_HU_Pos) | (minutes << RTC_TR_MNU_Pos) | seconds;

    // Exit initialization mode
    RTC->ISR &= ~RTC_ISR_INIT;

    // Wait for synchronization
    RTC->ISR &= ~RTC_ISR_RSF;
    while ((RTC->ISR & RTC_ISR_RSF) == 0);

    // Re-enable write protection
    RTC->WPR = 0xFF;
}
// Set date for RTC DR
void set_date(int mm, int dd, int yy){
    // check if in range
    if (mm >= 1 && mm <= 12 && dd >= 1 && dd <= 31 && yy >= 0 ){
        // Convert decimal to BCD
        month = Decimal_to_BCD(mm);
        day = Decimal_to_BCD(dd);
        year = Decimal_to_BCD(yy);
    }

    // Disable write protection
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    // Enter initialization mode
    RTC->ISR |= RTC_ISR_INIT;
    while ((RTC->ISR & RTC_ISR_INITF) == 0);

    // Write date to RTC_DR register
    RTC->DR = (year << 16) | (month << 8) | day;

    // Exit initialization mode
    RTC->ISR &= ~RTC_ISR_INIT;

    // Wait for synchronization
    RTC->ISR &= ~RTC_ISR_RSF;
    while ((RTC->ISR & RTC_ISR_RSF) == 0);

    // Re-enable write protection
    RTC->WPR = 0xFF;

}

float get_s()
{
    RTC->ISR &= ~RTC_ISR_RSF; // Clear RSF to start synchronization
    while ((RTC->ISR & RTC_ISR_RSF) == 0); // Sync the shadow rgeister

    //char sendString[128];
    //sprintf(sendString, "RTC: sec: %d\r\n", sec); 
	//writeSerialString(consolSerialPort, sendString, 0); // add newline and carriage return

    uint32_t this_tr = RTC->TR;

    // Note: Must include day, month, year also if we want to be up for that long
    uint8_t hr = (this_tr >> 16) & 0x3F;
    uint8_t min = (this_tr >> 8) & 0x7F;
    uint8_t sec = this_tr & 0x7F;

    hr = BCD_to_Decimal(hr);
    min = BCD_to_Decimal(min);
    sec = BCD_to_Decimal(sec);

    float sub =  ((float)((RTC->PRER & RTC_PRER_PREDIV_S_Msk) - ((RTC->SSR) & RTC_SSR_SS_Msk)) / ((RTC->PRER & RTC_PRER_PREDIV_S_Msk) + 1));
    float seconds = (float)(hr*60*60) + (float)(min * 60) + (float)sec + sub;

    return seconds;
}