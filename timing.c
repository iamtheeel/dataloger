/*
 * Timing Functions, using SYSTICK
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

#include "timing.h"
#include "dIO.h"
#include "RTC.h"
#include "serialHand.h"

int clockFreq;

// The heartbeat
GPIO_TypeDef *timeTestPORT = GPIOA;   // The LED is port A
const int timeTestPort_num = 0;
const int hbLED_pin = 5;        // PA5 is hardwired to the green led
const int timeMeas_pin = 10;    // PA10 for ease of access

// Run mode
int runMode = RUNMODE_SOFF;

// Task Clocks
volatile int counter_100Hz = 0;
volatile int counter_25Hz = 0;
volatile int counter_5Hz = 0;
volatile int counter_1Hz = 0;
volatile int counter_SAFEMODE = 0;

void set_sysTick_freq(unsigned int freq_hz)
{
	clockFreq = 4e6; 	        // Main Clock at 4Mhz
	unsigned long ticks = clockFreq/freq_hz; //time in ms, clock in Hz
	SysTick->LOAD = ticks -1; // how many counts till interrupt
}
void set_sysTick_time(unsigned int time_ms)
{
	/*
	 * If the clock is 1Mhz, 1000 ticks = 1 mS
	 * ticks/Clock = time
	 * ticks = time * clock
	 * max ticks = 2^32 -1
	 * 
	 * From debug clock = 8Mhz
	 * 2 clock tics/cycle
	 */
	unsigned long ticks = time_ms*clockFreq/1000; //time in ms, clock in Hz
	
	SysTick->LOAD = ticks -1; // how many counts till interrupt
}

void init_sysTiming(int runMode_)
{
    runMode = runMode_;

    // Set the loop timing
    //delayTime = 1000*(MAINLOOPFREQ);
    //delayTime = 100* delayTime; // debug
    // Init the HB pin
    configure_portNum(timeTestPort_num);        // Our timing tests are all on PORTA
    configure_outPin(timeTestPORT, hbLED_pin, PIN_PUSHPULL);
    configure_outPin(timeTestPORT, timeMeas_pin, PIN_PUSHPULL);

	SysTick->CTRL = 0; 			// Start from all off
	set_sysTick_freq(MAINLOOPFREQ);  // Set systick in terms of ms
	
	NVIC_SetPriority(SysTick_IRQn, (1<<__NVIC_PRIO_BITS) -1); 
	
	SysTick->VAL = 0; 								// Set empty
	SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk; 	// 1 = Use processor clock
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;   	// 1 = call an interrupt when we get to 0
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;		// Eable the timer
}

int set_runMode(int rMode)
{
    runMode = rMode;
    return runMode;
}
int get_runMode() { return runMode; }

void delay_s(float sToDelay)
{
    float s_start = get_s();
    float s_end = s_start + sToDelay;

    //char sendString[128];
    //sprintf(sendString, "s_start: %f, sToDelay: %f, end time: %f\r\n", s_start, sToDelay ,s_end); 
	//writeSerialString(consolSerialPort, sendString, 0); // add newline and carriage return

    while(get_s() < s_end){;}

    //sprintf(sendString, "s_end: %f\r\n", get_s()); 
	//writeSerialString(consolSerialPort, sendString, 0); // add newline and carriage return
}

void run100HzTasks(){
    // 100hz in SYSTICK interupt code here
    if(runMode < 0)
    { toggle_pin(timeTestPORT, hbLED_pin); } // Blink the HeartBeat LED
}
void run25HzTasks() { 
    // 25hz in SYSTICK interupt code here
    if(runMode == RUNMODE_CMDE)
    { toggle_pin(timeTestPORT, hbLED_pin); } // Blink the HeartBeat LED
}
void run5HzTasks() { 
    // 5hz in SYSTICK interupt code here
    if(runMode == RUNMODE_NORM)
    { toggle_pin(timeTestPORT, hbLED_pin); } // Blink the HeartBeat LED
}
void run1HzTasks(){
    // 1hz in SYSTICK interupt code here
    if(runMode == RUNMODE_SOFF)
    { toggle_pin(timeTestPORT, hbLED_pin); } // Blink the HeartBeat LED
}

void SysTick_Handler(void)
{
    // systick at 1kHz
    counter_SAFEMODE++;

    if(runMode != RUNMODE_SAFE) // We are not in safe mode
    {
	    set_PinState(timeTestPORT, timeMeas_pin, 1); // Set pin to measure timing
        counter_100Hz++;
        counter_25Hz++;
        counter_5Hz++;
        counter_1Hz++;

        if(counter_100Hz >= 10)
        {
            counter_100Hz = 0;
            run100HzTasks();
        }
        if(counter_25Hz >= 40)
        {
            counter_25Hz = 0;
            run25HzTasks();
        }
        if(counter_5Hz >= 200)
        {
            counter_5Hz = 0;
            run5HzTasks();
        }
        if(counter_1Hz >= 1000)
        {
            counter_1Hz = 0;
            run1HzTasks();
        }
	    set_PinState(timeTestPORT, timeMeas_pin, 0); // Set pin to measure timing
    }
    else // ******* SAFEMODE ***********//
    {
        // THe only thing we do is blink the LED fast
        if(counter_SAFEMODE >= 25) // 40Hz
        {
            counter_SAFEMODE = 0;
	        toggle_pin(timeTestPORT, hbLED_pin); // Blink the HeartBeat LED
        }
    }

}

void init_Shutdown(void){
    // enable write access to the back up domain
	init_EXTI(GPIOC, 2, 13, 0, 1);		// Enable EXTI15_10
}

void call_Shutdown(void)
{
    if ((RCC->APB1ENR1 & RCC_APB1ENR1_PWREN) == 0) {
        // Enable the power interface clock
        RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
        // Add a short delay after anabling an RCC peripheral clock
        (void) RCC->APB1ENR1;
    }

    // Set LPMS[2:0] to Shutdown mode 1xx
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; // Set the SLEEPDEEP bit
    PWR->CR1 |= PWR_CR1_LPMS_SHUTDOWN;
}