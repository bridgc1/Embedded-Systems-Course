// This program illustrates the interfacing of the Stepper Motor with the microcontroller

// Name: Christina Bridges    Date: Last modified Feb 20, 2026

#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "PLL.h"
#include "SysTick.h"

#define CW	1
#define	CCW 0

void PortL_Init(void){
	//Use PortL pins (PL0-PL3) for output
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R10;				// activate clock for Port L
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R10) == 0){};	// allow time for clock to stabilize
	GPIO_PORTL_DIR_R |= 0x0F;        								// configure Port L pins (PL0-PL3) as output
  GPIO_PORTL_AFSEL_R &= ~0x0F;     								// disable alt funct on Port L pins (PL0-PL3)
  GPIO_PORTL_DEN_R |= 0x0F;        								// enable digital I/O on Port L pins (PL0-PL3)
																									// configure Port L as GPIO
  GPIO_PORTL_AMSEL_R &= ~0x0F;     								// disable analog functionality on Port L	pins (PL0-PL3)	
	return;
}


void spin(uint8_t direction, uint32_t step_num){																			// Complete function spin to implement the Full-step Stepping Method
	uint32_t delay = 1;															// Does your motor spin clockwise or counter-clockwise?
	float factor = 140; //control to experimentally determine the smallest time delay possible between steps --> discovered as 1.0 factor exactly, so 1.4 ms delay is the minimum
	
	for(uint32_t i = 0; i < step_num; i++){												// What should the upper-bound of i be for one complete rotation of the motor shaft?  i = 2048/4 steps = 512 steps
		if (direction == CW) {
			GPIO_PORTL_DATA_R = 0b00000011;								// get through 512 teeth for one change of the poles
			SysTick_Wait10us(delay*factor);											// What if we want to reduce the delay between steps to be less than 10 ms?
			GPIO_PORTL_DATA_R = 0b00000110;													// Complete the missing code.
			SysTick_Wait10us(delay*factor);
			GPIO_PORTL_DATA_R = 0b00001100;													// Complete the missing code.
			SysTick_Wait10us(delay*factor);
			GPIO_PORTL_DATA_R = 0b00001001;													// Complete the missing code.
			SysTick_Wait10us(delay*factor);
		}
		else if (direction == CCW){
			GPIO_PORTL_DATA_R = 0b00001001;								// get through 512 teeth for one change of the poles
			SysTick_Wait10us(delay*factor);											// What if we want to reduce the delay between steps to be less than 10 ms?
			GPIO_PORTL_DATA_R = 0b00001100;													// Complete the missing code.
			SysTick_Wait10us(delay*factor);
			GPIO_PORTL_DATA_R = 0b00000110;													// Complete the missing code.
			SysTick_Wait10us(delay*factor);
			GPIO_PORTL_DATA_R = 0b00000011;													// Complete the missing code.
			SysTick_Wait10us(delay*factor);
		
		}
		GPIO_PORTL_DATA_R = 0b00000000;
	}
}


int main(void){
	uint32_t steps = 512;														// need to use iunsigned int type of 32 bits here to fit 512 (same on line 30 as a result)
	
	PLL_Init();																			// Default Set System Clock to 120MHz
	SysTick_Init();																	// Initialize SysTick configuration
	PortL_Init();																		// Initialize Port l
	spin(CW, steps);																					// Call function spin
	SysTick_Wait10ms(100); //waits 10ms*100 = 1s
	spin(CW, steps);																					// Call function spin
	SysTick_Wait10ms(100); //waits 10ms*100 = 1s
	spin(CCW, steps);																					// Call function spin

	return 0;
}