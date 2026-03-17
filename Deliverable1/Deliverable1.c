// Written by Christina Bridges
// March 5, 2026

// Last updated: March 8, 2026

#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "PLL.h"
#include "SysTick.h"

/************************************************** STATE DEFINITIONS ********************************************************************************/
#define	STOPPED 0
#define	RUNNING 1
#define RETURN_HOME 2

/************************************************** PARAMETER DEFINITIONS ********************************************************************************/
// define operation of motor as ON or OFF state
#define ON 1
#define OFF 0

// define motor direction indicators
#define CW	1
#define	CCW 0

// define small (11.25 deg) or big (45 deg) angle points for LED output
#define small_angle 1
#define large_angle 0 // can later change boolean to a counter in the loop

// define the overall action state of the system, whether it is active, or set to return to the home position
#define HOME 1
#define ACTIVE 0

/************************************************** GLOBAL VARIABLE INITIALIZATIONS ********************************************************************************/

uint8_t system_state = STOPPED;
uint8_t b0_on = OFF;							// ON/OFF state container. Default state of system OFF
uint8_t b1_dir = CW;							// motor direction container. Default state of system CW
uint8_t b2_angle = small_angle;		// Indicates if LED turns on at 11.25 (small) or 45 (large) degree points around during a motor's revolution; default set to small_angle
uint8_t b3_home = ACTIVE;						// indicator to return to home position (at 0 deg); by default ACTIVE
int32_t steps_from_home = 0;      // indicator for number of steps that occured since the home position


/************************************************** PORT INITIALIZATIONS  ********************************************************************************/

void PortN_Init(void){
	//Use PortN pins (PN0-PN1) for output
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;				// activate clock for Port N
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};	// allow time for clock to stabilize
		GPIO_PORTN_DIR_R |= 0x03;        								// configure PN 1:0 as output
		GPIO_PORTN_AFSEL_R &= ~0x03;     								// disable alt funct on Port N pins
		GPIO_PORTN_DEN_R |= 0x03;        								// enable digital I/O on Port N pins (PN0-PN1)
		 
		GPIO_PORTN_AMSEL_R &= ~0x03;     								// disable analog functionality on Port N	pins
	return;
}

void PortF_Init(void){
	//Use PortF pins (PF0, PF4) for output
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;				// activate clock for Port F
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};	// allow time for clock to stabilize
	GPIO_PORTF_DIR_R |= 0x11;        								// configure PF0 and PF4 as output
	GPIO_PORTF_AFSEL_R &= ~0x11;     								// disable alt funct on Port F pins
  GPIO_PORTF_DEN_R |= 0x11;        								// enable digital I/O on Port F pins
		
	GPIO_PORTF_AMSEL_R &= ~0x11;     								// disable analog functionality on Port F pins
	return;
}

void PortM_Init(void){
	//Use PortM pins (PM0-PM1) for input
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;				// activate clock for Port M
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R11) == 0){};	// allow time for clock to stabilize
	GPIO_PORTM_DIR_R &= ~0x03;        								// configure Port M pins (PM0-PM1) as input by clearing the bits
  GPIO_PORTM_AFSEL_R &= ~0x03;     								// disable alt funct on Port M pins (PM0-PM1)
  GPIO_PORTM_DEN_R |= 0x03;        								// enable digital I/O on Port M pins (PM0-PM1)
																									// configure Port M as GPIO
  GPIO_PORTM_AMSEL_R &= ~0x03;     								// disable analog functionality on Port M	pins by bit masking (PM0-PM1)	
	GPIO_PORTM_PUR_R |= 0x03;												// set pull-up resistors for pins (PM0-PM1)

	return;
}

void PortJ_Init(void){																	
	//Use PortJ pins (PJ0-PJ1) for input
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8;				// activate clock for Port J
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R8) == 0){};	// allow time for clock to stabilize
	GPIO_PORTJ_DIR_R &= ~0x03;        								// configure Port J pins (PJ0-PJ1) as input by clearing the bits
	GPIO_PORTJ_AFSEL_R &= ~0x03;     								// disable alt funct on Port J pins
  GPIO_PORTJ_DEN_R |= 0x03;        								// enable digital I/O on Port J pins PJ0-PJ1
		
	GPIO_PORTJ_AMSEL_R &= ~0x03;     								// disable analog functionality on Port J	pins
	GPIO_PORTJ_PUR_R |= 0x03;												// set pull-up resistors for pins (PJ0-PJ1)

	return;
}

void PortH_Init(void){
	//Use PortH pins (PH0-PH3) for output
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;				// activate clock for Port H
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){};	// allow time for clock to stabilize
	GPIO_PORTH_DIR_R |= 0x0F;        								// configure Port H pins (PH0-PH3) as output
	GPIO_PORTH_AFSEL_R &= ~0x0F;     								// disable alt funct on Port H pins
  GPIO_PORTH_DEN_R |= 0x0F;        								// enable digital I/O on Port H pins (PH0-PH3)
	
	GPIO_PORTH_AMSEL_R &= ~0x0F;     								// disable analog functionality on Port H pins
	GPIO_PORTH_DATA_R = 0x00;
		
	return;
}

/************************************************** CHECK WHICH BUTTONS ARE PRESSED W/ DEBOUNCING ********************************************************************************/
// ACTIVE LOW CONFIGURATION

uint8_t button0_check(void){
    if ((GPIO_PORTJ_DATA_R & 0x01) == 0x0){ // individually checks if the button 0 was pressed (input = 0 at PJ0)
        SysTick_Wait10ms(5);                  // wait 50 ms for debounce
        if ((GPIO_PORTJ_DATA_R & 0x01) == 0x0) { // confirm still pressed
            while((GPIO_PORTJ_DATA_R & 0x01) == 0x0); // wait until button released
            return 1; // button pressed
        }
    }
    return 0; // button NOT pressed
}

uint8_t button1_check(void){
    if ((GPIO_PORTJ_DATA_R & 0x02) == 0x0){ // individually checks if button 1 was pressed (input = 0 at PJ1)
        SysTick_Wait10ms(5);                  // wait 50 ms for debounce
        if ((GPIO_PORTJ_DATA_R & 0x02) == 0x0) { // confirm still pressed
            while((GPIO_PORTJ_DATA_R & 0x02) == 0x0); // wait until button released
            return 1; // button pressed
        }
    }
    return 0; // button NOT pressed
}

uint8_t button2_check(void){
    if ((GPIO_PORTM_DATA_R & 0x02) == 0x0){ // individually checks if button 2 was pressed (input = 0 at PM1)
        SysTick_Wait10ms(5);                  // wait 50 ms for debounce
        if ((GPIO_PORTM_DATA_R & 0x02) == 0x0) { // confirm still pressed
            while((GPIO_PORTM_DATA_R & 0x02) == 0x0); // wait until button released
            return 1; // button pressed
        }
    }
    return 0; // button NOT pressed
}

uint8_t button3_check(void){
    if ((GPIO_PORTM_DATA_R & 0x01) == 0x0){ // individually checks if button 3 was pressed (input = 0 at PM0)
        SysTick_Wait10ms(5);                  // wait 50 ms for debounce
        if ((GPIO_PORTM_DATA_R & 0x01) == 0x0) { // confirm still pressed
            while((GPIO_PORTM_DATA_R & 0x01) == 0x0); // wait until button released
            return 1; // button pressed
        }
    }
    return 0; // button NOT pressed
}

/************************************************** ACTIVATE OUTPUT LEDs ********************************************************************************/

void LED_ON(uint8_t on_state){						// LED0 is on when the motor state is ON										**STATUS OUTPUT 0
	if (on_state == ON){
		GPIO_PORTN_DATA_R |= 0x02;							// PN1 is LED0 / on-board D1
		// note the OR operation to avoid overwriting other output pins for GPIOM
	}
	else{
		GPIO_PORTN_DATA_R &= ~0x02;
	}
}

void LED_CW(uint8_t direction){					// LED1 is on when the motor state is CW											**STATUS OUTPUT 1
	if (direction == CW)
	{
		GPIO_PORTN_DATA_R |= 0x01;							// PN0 is LED2 / on-board D2
		// note the OR operation to avoid overwriting other output pins for GPIOM
	}
	else{
		GPIO_PORTN_DATA_R &= ~0x01;
	}
}

void LED_angle_size(uint8_t angle_size){					//LED2 is on when the concerned angle is 11.25 degrees; is off when the angle is 45 degrees
	if (angle_size == small_angle){									//																									**STATUS OUTPUT 2
			GPIO_PORTF_DATA_R |= 0x10;							// PF4 is LED2 / on-board D3
			// note the OR operation to avoid overwriting other output pins for GPIOM
	}
	else{
		GPIO_PORTF_DATA_R &= ~0x10;
	}
}

void LED_blink_motor(void){
	GPIO_PORTF_DATA_R |= 0x01;							// PF0 is LED3 / on-board D4											**STATUS OUTPUT 3
	SysTick_Wait10ms(2);																			// wait 20 ms before toggling LED
	GPIO_PORTF_DATA_R &= ~0x01;							// AND operator with the bitwise NOT of 0x01 to clear bit 1 of Port F (bit masking)
}

void clear_LEDs(void){													// clear all LEDs; called when the system is in the OFF state
	GPIO_PORTN_DATA_R &= ~0x03;				// clear D1 and D2
	GPIO_PORTF_DATA_R &= ~0x10;				// clear D3

}

/************************************************** MOTOR ROTATION HELPER FUNCTIONS ********************************************************************************/
// FULL STEP CHOSEN DUE TO HIGHER TORQUE --> WILL NEED FOR TOF SENSOR

void step_CW(){
	uint32_t delay = 1;
	uint32_t factor = 1000; // smallest time delay possible between steps ** CONTROL THIS TO CONTROL MOTOR SPEED FOR LATER DELIVERABLES	
	
	GPIO_PORTH_DATA_R = 0b00000011;								// 512 iterations x 4 coil activations/iteration --> 2048 motor steps/shaft revolution
	SysTick_Wait10us(delay*factor);
	GPIO_PORTH_DATA_R = 0b00000110;	
	SysTick_Wait10us(delay*factor);
	GPIO_PORTH_DATA_R = 0b00001100;	
	SysTick_Wait10us(delay*factor);
	GPIO_PORTH_DATA_R = 0b00001001;
	SysTick_Wait10us(delay*factor);
}

void step_CCW(){
	uint32_t delay = 1;
	uint32_t factor = 1000; // smallest time delay possible between steps ** CONTROL THIS TO CONTROL MOTOR SPEED FOR LATER DELIVERABLES	
	
	GPIO_PORTH_DATA_R = 0b00001001;	
	SysTick_Wait10us(delay*factor);	
	GPIO_PORTH_DATA_R = 0b00001100;		
	SysTick_Wait10us(delay*factor);
	GPIO_PORTH_DATA_R = 0b00000110;
	SysTick_Wait10us(delay*factor);
	GPIO_PORTH_DATA_R = 0b00000011;
	SysTick_Wait10us(delay*factor);
	
}

/************************************************** RETURN TO HOME POSITION FUNCTION (Simpler Version) ********************************************************************************/
void go_Home(void) {
    uint8_t counter_angle;
    uint32_t steps_to_home;
    uint8_t direction;

    // Normalize steps_from_home into (-511, 511) range
    int32_t pos = steps_from_home % 512;

    // Find shortest path by comparing sign of the position
    if (pos > 0) {
        if (pos <= 256) {         // going back via CCW is closer to home position
            steps_to_home = pos;
            direction = CCW;       // go backward to home
        } else {                  // CW long path
            steps_to_home = 512 - pos;
            direction = CW;
        }
    } else if (pos < 0) {
        int32_t neg = -pos;       // make positive
        if (neg <= 256) {         // going back via CW is closer to home position (less negative/less CCW at spot)
            steps_to_home = neg;
            direction = CW;       // go backward to home
        } else {                  // going back via CCW is faster
            steps_to_home = 512 - neg;
            direction = CCW;
        }
    } else {                      // already at home
        steps_to_home = 0;
    }

    // LED step counter based on angle
    counter_angle = (b2_angle) ? 16 : 64;

    // Step motor to home
    for (uint32_t i = 0; i < steps_to_home; i++) {
        if ((i + 1) % counter_angle == 0){
					LED_blink_motor();
				}
        if (direction == CW){
					step_CW();
				}
        else step_CCW();
    }

    // Reset global state to STOPPED state
    steps_from_home = 0;
    GPIO_PORTH_DATA_R = 0x00;
    system_state = STOPPED;
    clear_LEDs();
}


/************************************************** MOTOR CONTROL ********************************************************************************/

void spin(uint8_t direction, uint32_t step_num){		// Full-step Stepping Method
	uint8_t counter_angle;
	
	steps_from_home = steps_from_home % 512;		// need to include in case total steps_from_home exceeds the 32 bits and is truncated if enough rotations occur during run time

	// ANGLE LED STEP COUNTER
	counter_angle = (b2_angle)? 16:64;	// if angle is 11.25 deg then set angle counter to 16, else 64 for the 45 deg angle
	
	
	
	for(uint32_t i = 0; i < step_num; i++){												// What should the upper-bound of i be for one complete rotation of the motor shaft?  i = 2048/4 steps = 512 steps
		
		// UPDATE BUTTON STATUS-------------------------
		if(button0_check()){				// check to see if button 0 was pressed (to indicate OFF)
			b0_on ^= 1;
			system_state = STOPPED;
			break;
		}
		
		// home function check and activation
		b3_home = button3_check();
		if (b3_home){
			go_Home();
			break;
		}
		
		// LED3 blinks for each step of rotation (i.e. 11.25 or 45 deg depending on set angle)
			if ((i+1) % counter_angle == 0){								// using (i+1) ensures the LED blinks only after the angle is reached (not at 0 deg)
				LED_blink_motor();
			}
			
		// UPDATE STEPPER MOTOR POSITION----------------
		if (direction == CW) {
			
			step_CW();
			
			steps_from_home++;														// update number of steps since the program started (in positive direction)	
			}
			
		else if (direction == CCW){
			
			step_CCW();
			
			steps_from_home--;														// update number of steps since the program started (in negative direction)

			}
		
		}	
	GPIO_PORTH_DATA_R = 0b00000000;
	system_state = STOPPED; 				// after 1 full rotation, set the system state to OFF
	clear_LEDs(); 			// turn off all LEDs
}
/************************************************** MAIN FUNCTION ********************************************************************************/

int main(void){
	uint32_t steps = 512;							// need to use iunsigned int type of 32 bits here to fit 512 (same on line 30 as a result)
	
	
	PLL_Init();																			// Default Set System Clock to 120MHz
	SysTick_Init();																	// Initialize SysTick configuration
	PortN_Init();																		// Initialize Port N
	PortF_Init();																		// Initialize Port F
	PortM_Init();																		// Initialize Port M
	PortJ_Init();																		// Initialize Port J
	PortH_Init();																		// Initialize Port H
	
	while (1){
		
		
		// UPDATE THE STATES------------------------------
		uint8_t b0_pressed = button0_check();
		uint8_t b1_pressed = button1_check();
		uint8_t b2_pressed = button2_check();
		uint8_t b3_pressed = button3_check();

		if (system_state == STOPPED){
			if (b0_pressed){												// check twice if button is pressed to ensure valid press
				// if the button is pressed and the system is currently STOPPED, set the system state to RUNNING
					system_state = RUNNING;
				}
			}
			
		if (b1_pressed){	
			
				b1_dir ^= 1;														// toggles to direction between CW and CCW
		}
		
		if(b2_pressed){
			b2_angle ^=1;														// toggles the direction between the 11.25 and 45 degree parameters (small and large angle)
		}
		
		if(b3_pressed){												// if the button is pressed the system enters the RETURN_HOME state and returns to the starting position
				system_state = RETURN_HOME;
		}
		
		
		// PERFORM ACTION DEPENDING ON SYSTEM STATE---------
		switch(system_state){
			case STOPPED:
				clear_LEDs();
				break;
			case RUNNING:
				// update LED output status
				LED_ON(ON);
				LED_CW(b1_dir);
				LED_angle_size(b2_angle);
				spin(b1_dir, steps);			// call motor function
			break;
			case RETURN_HOME:
				go_Home();
				system_state = STOPPED;
				break;
		}
	}
	return 0;
}