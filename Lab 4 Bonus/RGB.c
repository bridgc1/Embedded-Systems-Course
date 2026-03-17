//  Written by Christina Bridges
//  Feb 22, 2026
//  Last Update: Feb 22, 2026


#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "PLL.h"
#include "SysTick.h"

#define RED	0b00000100      // PN2
#define GREEN	0b00001000    // PN3
#define BLUE	0b00010000    // PN4


void PortN_Init(void){
	//Use PortN onboard LED	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;				// activate clock for Port N
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};	// allow time for clock to stabilize
	GPIO_PORTN_DIR_R |= 0x1C;        								// make PN2-4 so 0b0001.1100 --> 0x1.C
  GPIO_PORTN_AFSEL_R &= ~0x1C;     								// disable alt funct on PN0
  GPIO_PORTN_DEN_R |= 0x1C;        								// enable digital I/O on PN0
																									// configure PN1 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF0F)+0x00000000;
  GPIO_PORTN_AMSEL_R &= ~0x05;     								// disable analog functionality on PN0		
	
	GPIO_PORTN_DATA_R &= ~0x1C; //start with all 3 LEDs OFF
	
	return;
}


void DutyCycle_Percent(uint8_t red, uint8_t green, uint8_t blue){ // percent = duty cycle * 10
		//float RGB_percent = {red_percent, blue_percent, };	

		int red_percent = (int)(((float)red*1000)/(255));  // 255 for maximum sequence of 8 bits
		int green_percent = (int)(((float)green*1000)/(255));  // 255 for maximum sequence of 8 bits
		int blue_percent = (int)(((float)blue*1000)/(255));  // 255 for maximum sequence of 8 bits

		int percent_int[] = {red_percent, green_percent, blue_percent};
			
		for (int i = 0;i < 1000; i++) {  // this method attempts to minimize time delay due to synchronous program among updating output signal to each RGB pin as the microcontroller goes through each line sequentially
			if (i < red_percent) { // FOR RED
			GPIO_PORTN_DATA_R |= RED;
			}
			else {
			GPIO_PORTN_DATA_R &= ~RED;
			}
			
			if (i < green_percent) { // FOR GREEN
			GPIO_PORTN_DATA_R |= GREEN;
			}
			else {
			GPIO_PORTN_DATA_R &= ~GREEN;
			}
			
			if (i < blue_percent) { // FOR BLUE
			GPIO_PORTN_DATA_R |= BLUE;
			}
			else {
			GPIO_PORTN_DATA_R &= ~BLUE;
			}
			SysTick_Wait10us(1);

		}
		
}

int main(void){
	uint8_t RED_duty = 255;
	uint8_t GREEN_duty = 0;
	uint8_t BLUE_duty = 255;
	
	PLL_Init();																			// Default Set System Clock to 120MHz
	SysTick_Init();																	// Initialize SysTick configuration
	PortN_Init();																		// Initialize Port N 
	
	
	while(0 <= RED_duty && RED_duty <= 255 && 0 <= BLUE_duty && BLUE_duty <= 255 && 0 <= GREEN_duty && GREEN_duty <= 255){  //ensure input percents will produce a safe ratio correlating to the power output
		DutyCycle_Percent(RED_duty, GREEN_duty, BLUE_duty);
	}
}

// if putting r,g,b duty cycle individual function calls the cycles will be prepared syquentially but run in sync with each other



