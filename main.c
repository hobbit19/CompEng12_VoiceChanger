//#############################################################################
//
// FILE:   adc_ex2_soc_epwm.c
//
// TITLE:  ADC ePWM Triggering
//
//! \addtogroup driver_example_list
//! <h1>ADC ePWM Triggering</h1>
//!
//! This example sets up ePWM1 to periodically trigger a conversion on ADCA.
//!
//! \b External \b Connections \n
//!  - A0 should be connected to a signal to convert
//!
//! \b Watch \b Variables \n
//! - \b adcAResults - A sequence of analog-to-digital conversion samples from
//!   pin A0. The time between samples is determined based on the period
//!   of the ePWM timer.
//!
//
//#############################################################################
// $TI Release: F2837xD Support Library v3.05.00.00 $
// $Release Date: Thu Oct 18 15:48:42 CDT 2018 $
// $Copyright:
// Copyright (C) 2013-2018 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

//
// Included Files
//
#include "driverlib.h"
#include "device.h"

//
// Defines
//
#define RESULTS_BUFFER_SIZE     2000
#define EX_ADC_RESOLUTION       12
//#define _LAUNCHXL_F28379D

// 12 for 12-bit conversion resolution, which support (ADC_MODE_SINGLE_ENDED)
// Sample on single pin with VREFLO
// Or 16 for 16-bit conversion resolution, which support (ADC_MODE_DIFFERENTIAL)
// Sample on pair of pins
//
// Globals
//
uint16_t adcAResults1[RESULTS_BUFFER_SIZE];   // Buffer for results
uint16_t adcAResults2[RESULTS_BUFFER_SIZE];   // Buffer for results
uint16_t index1 = 0;
uint16_t index2 = 0;  // Index into result buffer
uint16_t pwm_adc;
uint16_t bufferFlag = 1;                // Flag to indicate buffer
float index_f = 0.0;
float i_inc[11] = {0.5, 0.67, 0.75, 0.8, 0.9, 1.0, 1.1, 1.25, 1.33, 1.66, 2.0};
uint16_t inc_indx = 5;
uint16_t push_count = 0;
uint16_t LED_count = 0;
uint16_t LED_b_count = 0;



//
// Function Prototypes
//
void initADC(void);
void initEPWM(void);
void initADCSOC(void);
void buttons_control (void);
void LED_display (void);

__interrupt void adcA1ISR(void);

//
// Main
//
void main(void)
{
    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pullups.
    //
    Device_initGPIO();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // Interrupts that are used in this example are re-mapped to ISR functions
    // found within this file.
    //
    Interrupt_register(INT_ADCA1, &adcA1ISR);

    //
    // Set up the ADC and the ePWM and initialize the SOC
    //
    initADC();
    initEPWM();
    initADCSOC();

    // Init buttons GPIO

    //
    // Make GPIO34 an input on GPIO34
    //
    GPIO_setPadConfig(1, GPIO_PIN_TYPE_PULLUP);     // Enable pullup on GPIO1
    GPIO_setPinConfig(GPIO_1_GPIO1);               // GPIO1 = GPIO1
    GPIO_setDirectionMode(1, GPIO_DIR_MODE_IN);     // GPIO1 = input
    //
    // Make GPIO34 an input on GPIO34
    //
    GPIO_setPadConfig(2, GPIO_PIN_TYPE_PULLUP);     // Enable pullup on GPIO2
    GPIO_setPinConfig(GPIO_2_GPIO2);               // GPIO2 = GPIO2
    GPIO_setDirectionMode(2, GPIO_DIR_MODE_IN);     // GPIO2 = input


    GPIO_setPadConfig(DEVICE_GPIO_PIN_LED1, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);

    GPIO_setPadConfig(DEVICE_GPIO_PIN_LED2, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);

    GPIO_writePin(DEVICE_GPIO_PIN_LED1, 1);  //off BLUE
    GPIO_writePin(DEVICE_GPIO_PIN_LED2, 1);  //off RED


    //
    // Initialize results buffer
    //
    for(index1 = 0; index1 < RESULTS_BUFFER_SIZE; index1++)
    {
        adcAResults1[index1] = 0;
        adcAResults2[index1] = 0;
    }

    index1 = 0;

    //
    // Enable ADC interrupt
    //
    Interrupt_enable(INT_ADCA1);

    //
    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    //
    EINT;
    ERTM;

    //
    // Loop indefinitely
    //
    while(1)
    {
        //
        // Start ePWM1, enabling SOCA and putting the counter in up-count mode
        //
        EPWM_enableADCTrigger(EPWM1_BASE, EPWM_SOC_A);
        EPWM_setTimeBaseCounterMode(EPWM1_BASE, EPWM_COUNTER_MODE_UP);


        //
        while(1) {}
        //
        // Wait while ePWM1 causes ADC conversions which then cause interrupts.
        // When the results buffer is filled, the bufferFull flag will be set.
//        while(bufferFull == 0)
//        {
//        }
//        bufferFull = 0;     // Clear the buffer full flag

        //
        // Stop ePWM1, disabling SOCA and freezing the counter
        //
        EPWM_disableADCTrigger(EPWM1_BASE, EPWM_SOC_A);
        EPWM_setTimeBaseCounterMode(EPWM1_BASE, EPWM_COUNTER_MODE_STOP_FREEZE);

        //
        // Software breakpoint. At this point, conversion results are stored in
        // adcAResults.
        //
        // Hit run again to get updated conversions.
        //
        ESTOP0;
    }
}

//
// Function to configure and power up ADCA.
//
void initADC(void)
{
    //
    // Set ADCCLK divider to /4
    //
    ADC_setPrescaler(ADCA_BASE, ADC_CLK_DIV_4_0);

    //
    // Set resolution and signal mode (see #defines above) and load
    // corresponding trims.
    //
#if(EX_ADC_RESOLUTION == 12)
    ADC_setMode(ADCA_BASE, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
#elif(EX_ADC_RESOLUTION == 16)
    ADC_setMode(ADCA_BASE, ADC_RESOLUTION_16BIT, ADC_MODE_DIFFERENTIAL);
#endif
    //
    // Set pulse positions to late
    //
    ADC_setInterruptPulseMode(ADCA_BASE, ADC_PULSE_END_OF_CONV);

    //
    // Power up the ADC and then delay for 1 ms
    //
    ADC_enableConverter(ADCA_BASE);
    DEVICE_DELAY_US(1000);
}

//
// Function to configure ePWM1 to generate the SOC.
//
void initEPWM(void)
{
    //
    // Disable SOCA
    //
    EPWM_disableADCTrigger(EPWM1_BASE, EPWM_SOC_A);

    //
    // Configure the SOC to occur on the first up-count event
    //
    EPWM_setADCTriggerSource(EPWM1_BASE, EPWM_SOC_A, EPWM_SOC_TBCTR_U_CMPA);
    EPWM_setADCTriggerEventPrescale(EPWM1_BASE, EPWM_SOC_A, 1);

    //
    EPWM_setClockPrescaler(EPWM1_BASE,
                           EPWM_CLOCK_DIVIDER_1,
                           EPWM_HSCLOCK_DIVIDER_1);
    // Set the compare A value to 1250 and the period to 2500 (100MhZ / (2 * 2500) = 20 kHz)
    //
    EPWM_setCounterCompareValue(EPWM1_BASE, EPWM_COUNTER_COMPARE_A, 1250);
    EPWM_setCounterCompareValue(EPWM1_BASE, EPWM_COUNTER_COMPARE_B, 1250);
    EPWM_setTimeBasePeriod(EPWM1_BASE, 2500);

// set action

    EPWM_setActionQualifierAction(EPWM1_BASE,
                                     EPWM_AQ_OUTPUT_A,
                                     EPWM_AQ_OUTPUT_HIGH,
                                     EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPB);
       EPWM_setActionQualifierAction(EPWM1_BASE,
                                     EPWM_AQ_OUTPUT_A,
                                     EPWM_AQ_OUTPUT_LOW,
                                     EPWM_AQ_OUTPUT_ON_TIMEBASE_PERIOD);

       //set pin
       GPIO_setPadConfig(0, GPIO_PIN_TYPE_STD);
       GPIO_setPinConfig(GPIO_0_EPWM1A);

    //
    // Freeze the counter
    //
    EPWM_setTimeBaseCounterMode(EPWM1_BASE, EPWM_COUNTER_MODE_STOP_FREEZE);
}

//
// Function to configure ADCA's SOC0 to be triggered by ePWM1.
//
void initADCSOC(void)
{
    //
    // Configure SOC0 of ADCA to convert pin A0. The EPWM1SOCA signal will be
    // the trigger.
    //
    // For 12-bit resolution, a sampling window of 15 (75 ns at a 200MHz
    // SYSCLK rate) will be used.  For 16-bit resolution, a sampling window of
    // 64 (320 ns at a 200MHz SYSCLK rate) will be used.
    //
#if(EX_ADC_RESOLUTION == 12)
       ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM1_SOCA,
                    ADC_CH_ADCIN0, 15);
#elif(EX_ADC_RESOLUTION == 16)
       ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER0, ADC_TRIGGER_EPWM1_SOCA,
                    ADC_CH_ADCIN0, 64);
#endif

    //
    // Set SOC0 to set the interrupt 1 flag. Enable the interrupt and make
    // sure its flag is cleared.
    //
    ADC_setInterruptSource(ADCA_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER0);
    ADC_enableInterrupt(ADCA_BASE, ADC_INT_NUMBER1);
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
}

//
// ADC A Interrupt 1 ISR
//
__interrupt void adcA1ISR(void)
{
    //
    // Add the latest result to the buffer
    //
    if  (bufferFlag == 1)

        {
        adcAResults1[index1] = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
        pwm_adc = adcAResults2[index2];
        pwm_adc = pwm_adc + 226;   // 1250 - 1024
        EPWM_setCounterCompareValue(EPWM1_BASE, EPWM_COUNTER_COMPARE_B, pwm_adc);
        index1 ++;

        index_f = index_f + i_inc [inc_indx];
        index2 = (int) index_f ;

        if(RESULTS_BUFFER_SIZE <= index2)
            {
            index2 = 0;
            index_f = 0.0;
            }

        if(RESULTS_BUFFER_SIZE <= index1)
                {
                    index1 = 0;
                    index2 = 0;
                    index_f = 0.0;
                    bufferFlag = 2;
                }



        }
    else
    {
        adcAResults2[index2] = ADC_readResult(ADCARESULT_BASE, ADC_SOC_NUMBER0);
        pwm_adc = adcAResults1[index1];
        pwm_adc = pwm_adc + 226;   // 1250 - 1024
        EPWM_setCounterCompareValue(EPWM1_BASE, EPWM_COUNTER_COMPARE_B, pwm_adc);


        index2 ++;
        index_f = index_f + i_inc [inc_indx];
        index1 = (int) index_f;

        if(RESULTS_BUFFER_SIZE <= index1)
            {
            index1 = 0;
            index_f = 0.0;
            }

        if(RESULTS_BUFFER_SIZE <= index2)
                {
                    index1 = 0;
                    index2 = 0;
                    index_f = 0.0;
                    bufferFlag = 1;

                }

    }


    // Buttons control
    buttons_control();
    LED_display();

    //
    // Clear the interrupt flag and issue ACK
    //
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

void buttons_control(void)
{
if (GPIO_readPin(1) == 0 || GPIO_readPin(2) == 0) push_count++;
if (GPIO_readPin(1) == 1 && GPIO_readPin(2) == 1) push_count = 0;
if (push_count >= 40000)  // 2 seconds
    {
    if (GPIO_readPin(1) == 0 && inc_indx < 10) inc_indx ++;
    if (GPIO_readPin(2) == 0 && inc_indx > 0) inc_indx --;
    push_count = 0;
    }
}
void LED_display (void)
{
    LED_count ++;
    if (LED_count >= 10000)  //0.5 sec
    {
        LED_count = 0;
        LED_b_count ++;
        if (LED_b_count > 20) LED_b_count = 0; // 10 sec period
        if ((inc_indx == 6 && LED_b_count == 1) ||
             (inc_indx == 7 && (LED_b_count == 1 || LED_b_count == 3)) ||
              (inc_indx == 8 && (LED_b_count == 1 || LED_b_count == 3 || LED_b_count == 5)) ||
                (inc_indx == 9 && (LED_b_count == 1 || LED_b_count == 3 || LED_b_count == 5 || LED_b_count == 7)) ||
                  (inc_indx == 10 && (LED_b_count == 1 || LED_b_count == 3 || LED_b_count == 5 || LED_b_count == 7 || LED_b_count == 9)))
                  GPIO_writePin(DEVICE_GPIO_PIN_LED1, 0);//on BLUE
        else GPIO_writePin(DEVICE_GPIO_PIN_LED1, 1);  //off BLUE

        if ((inc_indx == 4 && LED_b_count == 1) ||
                     (inc_indx == 3 && (LED_b_count == 1 || LED_b_count == 3)) ||
                      (inc_indx == 2 && (LED_b_count == 1 || LED_b_count == 3 || LED_b_count == 5)) ||
                        (inc_indx == 1 && (LED_b_count == 1 || LED_b_count == 3 || LED_b_count == 5 || LED_b_count == 7)) ||
                          (inc_indx == 0 && (LED_b_count == 1 || LED_b_count == 3 || LED_b_count == 5 || LED_b_count == 7 || LED_b_count == 9)))
                          GPIO_writePin(DEVICE_GPIO_PIN_LED2, 0);  //on RED
                else GPIO_writePin(DEVICE_GPIO_PIN_LED2, 1);  //off RED


    }
}
