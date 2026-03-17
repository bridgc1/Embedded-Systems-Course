//  Modified by Christina Bridges
//  Last Update: February 9, 2026

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "tm4c1294ncpdt.h"
#include "PLL.h"
#include "SysTick.h"

#define SAMPLE_NUM 800
#define SAMPLE_DELAY 3600000

volatile uint32_t ADCvalue;

//-ADC0_InSeq3-
// Busy-wait analog to digital conversion. 0 to 3.3V maps to 0 to 4095 
// Input: none 
// Output: 12-bit result of ADC conversion 
uint32_t ADC0_InSeq3(void){
	uint32_t result;
	
	ADC0_PSSI_R = 0x0008;														// 1) initiate SS3   
	while((ADC0_RIS_R&0x08)==0){}										// 2) wait for conversion done   
	result = ADC0_SSFIFO3_R&0xFFF;									// 3) read 12-bit result   
	ADC0_ISC_R = 0x0008;														// 4) acknowledge completion   
	
	return result; 
} 

void ADC_Init(void){
	//config the ADC from Valvano textbook
	SYSCTL_RCGCGPIO_R |= 0b00010000;								// 1. activate clock for port E
	while ((SYSCTL_PRGPIO_R & 0b00010000) == 0) {}	//		wait for clock/port to be ready
	GPIO_PORTE_DIR_R &= ~0x10;											// 2) make PE4 input   note that the &= ~ is a clearing op
	GPIO_PORTE_AFSEL_R |= 0x10;											// 3) enable alternate function on PE4   
	GPIO_PORTE_DEN_R &= ~0x10;											// 4) disable digital I/O on PE4   
	GPIO_PORTE_AMSEL_R |= 0x10;											// 5) enable analog function on PE4   
	SYSCTL_RCGCADC_R |= 0x01;												// 6) activate ADC0 via bit 0   (ADC0 channel Ain9)
	ADC0_PC_R = 0x01;																// 7) maximum speed is 125K samples/sec   
	ADC0_SSPRI_R = 0x0123;													// 8) Sequencer 3 is highest priority   (1 FIFO depth)   higher priority in LSB
	ADC0_ACTSS_R &= ~0x0008;												// 9) disable sample sequencer 3 (controlled by bit 3)    need to disable prior to avoid undefined behaviour
	ADC0_EMUX_R &= ~0xF000;													// 10) seq3 is software trigger     conversion starts when writing ADC0_PSSI_R
	ADC0_SSMUX3_R = 9;															// 11) set SS3 to channel Ain9 (PE4)  --> physical pin to ADC sequencer 
	ADC0_SSCTL3_R = 0x0006;													// 12) no TS0 (not temp sensor) D0 (not differential), yes IE0 (interrupt flag set when done) END0 (end of sequence) 
	ADC0_IM_R &= ~0x0008;														// 13) disable SS3 interrupts   using polling not interrupts as is expected in sampling
	ADC0_ACTSS_R |= 0x0008;													// 14) enable sample sequencer 3   now operational
	//==============================================================================	
	
	return;
}

void PortN_Init(void){
	//Use PortN onboard LED	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;				// activate clock for Port N
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};	// allow time for clock to stabilize
	GPIO_PORTN_DIR_R |= 0x01;        								// make PN0 out (PN0 built-in LED1)
  GPIO_PORTN_AFSEL_R &= ~0x01;     								// disable alt funct on PN0
  GPIO_PORTN_DEN_R |= 0x01;        								// enable digital I/O on PN0
																									// configure PN1 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF0F)+0x00000000;
  GPIO_PORTN_AMSEL_R &= ~0x01;     								// disable analog functionality on PN0		
	
	GPIO_PORTN_DATA_R ^= 0b00000001; 								//hello world!
	SysTick_Wait10ms(10);														//.1s delay
	GPIO_PORTN_DATA_R ^= 0b00000001;	
	return;
}


uint32_t func_m2[SAMPLE_NUM]; 											// array used to store realtime data 	; global variable because its outside the function and should be in memory when you are complete
float frequency = 0;

int main(void){
  				
	uint32_t 	delay = 1000; 												// this is a multiplier used for SysTick = half of delay CHANGE TO NS function  ; make a systick_waitus functin so this constant is an int
	
	PLL_Init();																			// Default Set System Clock to 120MHz
	SysTick_Init();																	// Initialize SysTick configuration
	ADC_Init();																			// Initialize ADC as per Vavano text
	PortN_Init();	
	// Initialize Port N GPIO

	for (uint32_t i = 0; i < SAMPLE_NUM; i++){ //remove a loop? 
		func_m2[i] = ADC0_InSeq3();
		SysTick_Wait1us(delay);									// halfdelay*10ms;   acceptable delay for DC but not fast enough for AC;   *0.03125 = 3.125 ms corresponds to 1/320 Hz  --> we are 160 Hz assigned so double that for Nyquist
		}			// Acquire sample	 or use systick_wait(sample_delay) instead of us func
		GPIO_PORTN_DATA_R ^= 0b00000001;							// toggle LED for visualization of process (can't include in loop will be too fast)
	
		uint32_t  max = func_m2[1]; //choose starting value for comparison
		uint32_t  min = func_m2[0];
		uint32_t  index_max, index_min;
		
		if ((max-min) >= 0){ //then increasing
			for (uint32_t index_max = 0; index_max < SAMPLE_NUM-1; index_max++){
				if (func_m2[index_max+1]>max){
					max = func_m2[index_min+1];
				}
				else{
					break;
				}
				
				for (uint32_t index_min = index_max; index_min < SAMPLE_NUM-1; index_min++){
				if (func_m2[index_min+1]<min){
					min = func_m2[index_min+1];
				}
				else{
					break;
				}
			}
		}
	}
		else {
			for (uint32_t index_min = 0; index_min < SAMPLE_NUM-1; index_min++){
				if (func_m2[index_min+1]<min){
					min = func_m2[index_min+1];
				}
				else{
					break;
				}
				
				for (uint32_t index_max = index_min; index_max < SAMPLE_NUM-1; index_max++){
				if (func_m2[index_max+1]>max){
					max = func_m2[index_min+1];
				}
				else{
					break;
				}
			}
		}
	}
				
		float period_between_max_min = 0;
		
		int diff = index_max-index_min;
		period_between_max_min = 2*abs(diff)*0.01;
		frequency = 1/period_between_max_min;
}

