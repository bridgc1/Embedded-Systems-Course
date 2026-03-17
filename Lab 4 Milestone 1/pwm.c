//   USE 1 K RESISTOR
//  Written by Christina Bridges
//  Last Update: Feb 20, 2026


#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "PLL.h"
#include "SysTick.h"


void PortN_Init(void){
	//Use PortN onboard LED	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;				// activate clock for Port N
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};	// allow time for clock to stabilize
	GPIO_PORTN_DIR_R |= 0x05;        								// make PN0 out (PN0 built-in LED1)
  GPIO_PORTN_AFSEL_R &= ~0x05;     								// disable alt funct on PN0
  GPIO_PORTN_DEN_R |= 0x05;        								// enable digital I/O on PN0
																									// configure PN1 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF0F)+0x00000000;
  GPIO_PORTN_AMSEL_R &= ~0x05;     								// disable analog functionality on PN0		
	
	GPIO_PORTN_DATA_R ^= 0b00000001; 								//hello world!
	SysTick_Wait10ms(10);														//.1s delay
	GPIO_PORTN_DATA_R ^= 0b00000001;	
	return;
}


void DutyCycle_Percent(int percent_of_duty){ // percent = duty cycle * 10
	
	
	GPIO_PORTN_DATA_R |= 0b00000100; // we are using Port N2 as output to the LED
	SysTick_Wait10us(percent_of_duty);// TEST AGAIN WITH 10ms instead of 10us it'll have a larger delay but might not be that chaotic
	GPIO_PORTN_DATA_R &= ~0b00000100;
	SysTick_Wait10us(1000-percent_of_duty);
}

void IntensitySteps(){
	float percent;
	int percent_int;
	
	// INCREASING DUTY CYCLE FROM 0% TO 100%
	for (int duty = 0; duty <= 255; duty+=25)  // output 11 duty cycles each 10 times which is achieved by incrementing by 25 steps from 0 to 255 // what didn't work: my other optio was 0 to 10 and i++ so max with < 110 conditional so will always truncate max current_cycle = 109/10 = 10
	{
		percent = ((float)duty*1000)/255;  // 255 for maximum sequence of 8 bits
		percent_int = (int)percent;
		for (int repeat = 0; repeat < 100; repeat++){
			DutyCycle_Percent(percent_int);
		}
	}
	
	// DECREASING DUTY CYCLE FROM 100% TO 0%

	for (int duty = 255; duty >= 25; duty-=25)  // output 11 duty cycles each 10 times so 10*11 = 110
	{
		percent = ((float)duty*1000)/255;  // 255 for maximum sequence of 8 bits
		percent_int = (int)percent;
		for (int repeat = 0; repeat < 100; repeat++){
			DutyCycle_Percent(percent_int);
		}
	}
}

int main(void){
	
	PLL_Init();																			// Default Set System Clock to 120MHz
	SysTick_Init();																	// Initialize SysTick configuration
	PortN_Init();																		// Initialize Port N 
	
	while(1){
		IntensitySteps(); 
	}
}





