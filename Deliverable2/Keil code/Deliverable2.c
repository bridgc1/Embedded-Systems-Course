// Project Deliverable 2
 
// Written by Christina Bridges
// March 5, 2026
// Last updated: March 27, 2026
/* Student parameters:
		BUS SPEED = 26 MHz
		GPIO PINS:
					Measurement status PF4
					UART Tx            PN1
					Additional status	 PN0
*/
 
//INCLUDE: activate pin to validate PLL with AD3
 
/************************************************** LINKED FILES ********************************************************************************/
 
#include <stdint.h>
#include <math.h> // for trig functions
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"
//COM7 UART
 
/************************************************** PARAMETER DEFINITIONS ********************************************************************************/
#define CW	1
#define	CCW 0
#define NUM 32
#define PI 3.141592654
#define X 3 // 3 scans along x-axis
 
int32_t steps_from_home = 0;      // indicator for number of steps that occured since the home position
 
/************************************************** I2C DEFINITIONS ********************************************************************************/
 
// I2C define for us:
#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable
 
#define MAXRETRIES              5           // number of receive attempts before giving up
 
 
/************************************************** PORT INITIALIZATIONS  ********************************************************************************/
 
void PortF_Init(void){
	//Use PortF pin PF4 for measurement status LED
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;				// activate clock for Port F
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};	// allow time for clock to stabilize
	GPIO_PORTF_DIR_R |= 0x10;        								// configure PF0 and PF4 as output
	GPIO_PORTF_AFSEL_R &= ~0x10;     								// disable alt funct on Port F pins
  GPIO_PORTF_DEN_R |= 0x10;        								// enable digital I/O on Port F pins
	GPIO_PORTF_AMSEL_R &= ~0x10;     								// disable analog functionality on Port F pins
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

void PortM_Init(void){
	//Use PortM pins (PM0) for checking bus speed
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;				// activate clock for Port M
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R11) == 0){};	// allow time for clock to stabilize
	GPIO_PORTM_DIR_R |= 0x01;        								// configure Port M pins (PM0) as output
  GPIO_PORTM_AFSEL_R &= ~0x01;     								// disable alt funct on Port M pins (PM0)
  GPIO_PORTM_DEN_R |= 0x01;        								// enable digital I/O on Port M pins (PM0)
																									// configure Port M as GPIO
  GPIO_PORTM_AMSEL_R &= ~0x01;     								// disable analog functionality on Port M	pins by bit masking (PM0)	

	return;
}

void PortN_Init(void){
 
	//Use PortN pins (PN0-PN1) for Additional and UART Tx status, respectively
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;				// activate clock for Port N
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};	// allow time for clock to stabilize
		GPIO_PORTN_DIR_R |= 0x03;        								// configure PN 1:0 as output
		GPIO_PORTN_AFSEL_R &= ~0x03;     								// disable alt funct on Port N pins
		GPIO_PORTN_DEN_R |= 0x03;        								// enable digital I/O on Port N pins (PN0-PN1)
		GPIO_PORTN_AMSEL_R &= ~0x03;     								// disable analog functionality on Port N	pins
	return;
}

void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?
 
    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only (SDA) required for I2C
 
    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3
 
                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
//    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
}
 
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
 
void measurement_Status(void){
	GPIO_PORTF_DATA_R |= 0x10;							// PF4 is on-board D3
	SysTick_Wait10ms(1);
	GPIO_PORTF_DATA_R &= ~0x10;
}
 
void UART_LED(void){
	GPIO_PORTN_DATA_R |= 0x02;							// PN1 is on-board D1
	SysTick_Wait10ms(1);
	GPIO_PORTN_DATA_R &= ~0x02;
}
 
void additional_status(void){
	GPIO_PORTN_DATA_R |= 0x01;							// PN0 is on-board D2
	SysTick_Wait10ms(1);
	GPIO_PORTN_DATA_R &= ~0x01;
}
 
void VL53L1X_XSHUT(void){
 
		//XSHUT     This pin is an active-low shutdown input; 
		//					the board pulls it up to VDD to enable the sensor by default. 
		//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
}
 
/************************************************** CHECK WHICH BUTTONS ARE PRESSED W/ DEBOUNCING ********************************************************************************/
 
uint8_t bus_button(void){ //PJ1 - initiate transmit data to pc (button 2)
    if ((GPIO_PORTJ_DATA_R & 0x02) == 0x0){ // individually checks if button 1 was pressed (input = 0 at PJ1)
        SysTick_Wait10ms(5);                  // wait 50 ms for debounce
        if ((GPIO_PORTJ_DATA_R & 0x02) == 0x0) { // confirm still pressed
            while((GPIO_PORTJ_DATA_R & 0x02) == 0x0); // wait until button released
            return 1; // button pressed
        }
    }
    return 0; // button NOT pressed
}
 
uint8_t sampling_button(void){ // PJ0 - start and stop the motor and ToF rotation and measurement(button 1)
    if ((GPIO_PORTJ_DATA_R & 0x01) == 0x0){ // individually checks if the button 0 was pressed (input = 0 at PJ0)
        SysTick_Wait10ms(5);                  // wait 50 ms for debounce
        if ((GPIO_PORTJ_DATA_R & 0x01) == 0x0) { // confirm still pressed
            while((GPIO_PORTJ_DATA_R & 0x01) == 0x0); // wait until button released
            return 1; // button pressed
        }
    }
    return 0; // button NOT pressed
}

/************************************************** VALIDATE BUS SPEED 26.67 MHz ********************************************************************************/

void BusClock_TestSignal(void){ // CANNOT EXIT WITHOUT RESET
    while(1){
        GPIO_PORTM_DATA_R ^= 0x01;   // toggle PM0 to AD3 for visual validation
        SysTick_Wait(1333);          // half-period delay to acheve 10 kHz --> f_signal = f_sysclk/(2*N) = (26.67 MHz/(2*1333) ~ 10,000 Hz
    }
}
 
/************************************************** MOTOR ROTATION HELPER FUNCTIONS ********************************************************************************/
 
void spin(){																			// Complete function spin to implement the Full-step Stepping Method
	uint32_t delay = 1;
	uint32_t factor = 1000;
	uint32_t step_num = 16; // 360 deg/ 512/16 measurements = 11.25 deg per measurement    and    512 steps/16 measurements = 32 steps per measurement
	steps_from_home = steps_from_home % 512;
	for(uint32_t i = 0; i < step_num; i++){	// rotate in CW direction during data acquisition
		GPIO_PORTL_DATA_R = 0b00000011;								// 512 iterations x 4 coil activations/iteration --> 2048 motor steps/shaft revolution
		SysTick_Wait10us(delay*factor);
		GPIO_PORTL_DATA_R = 0b00000110;	
		SysTick_Wait10us(delay*factor);
		GPIO_PORTL_DATA_R = 0b00001100;	
		SysTick_Wait10us(delay*factor);
		GPIO_PORTL_DATA_R = 0b00001001;
		SysTick_Wait10us(delay*factor);
		steps_from_home++;
	}
		GPIO_PORTL_DATA_R = 0b00000000;
}
 
void go_Home(void) {																						// CAN CHANGE BACK TO ORIGINAL GO HOME FUNCTION IF CALIBRATION IS POOR
 
	uint32_t delay = 1;
	uint32_t factor = 250;
	uint32_t step_num = 512; // rotate a full 360 degrees back to the initial position;   4096 /4 = 512 steps
 
 
	for(uint32_t i = 0; i < step_num; i++){	// rotate in CCW direction to return to home position
		if (step_num % 64 == 0){
			//*********************************** STATUS LED *******************************
			additional_status(); // blink LED as the motor returns home every 45 deg since 512/64=32 and 360/64=45 deg (by ratios)
		}
		GPIO_PORTL_DATA_R = 0b00001001;	
		SysTick_Wait10us(delay*factor);	
		GPIO_PORTL_DATA_R = 0b00001100;		
		SysTick_Wait10us(delay*factor);
		GPIO_PORTL_DATA_R = 0b00000110;
		SysTick_Wait10us(delay*factor);
		GPIO_PORTL_DATA_R = 0b00000011;
		SysTick_Wait10us(delay*factor);
		}
		GPIO_PORTL_DATA_R = 0b00000000;
	}

 
 
	
uint16_t	dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral
int status=0;
 
void visualize(void) {
	uint8_t dataReady;
  uint16_t Distance;
  uint8_t RangeStatus;
	uint8_t sensorState=0;
 
	uint32_t distance_meas[NUM];
	float y[NUM];
	float z[NUM];
	int y_int[NUM];
	int z_int[NUM];
 
	while(sensorState==0){ // step is ESSENTIAL
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(10); // WAIT FOR SENSOR TO FINISH REBOOTING
  }
	status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
  /* 2 Initialize the sensor with the default setting  */
  status = VL53L1X_SensorInit(dev); // SET TO DEFAULT SETTING (MEDIUM RANGE?)
 
	 /* 3 Optional functions to be used to change the main ranging parameters according the application requirements to get the best ranging performances */
		//status = VL53L1X_SetDistanceMode(dev, 1); /* 1=short, 2=long */
//  status = VL53L1X_SetTimingBudgetInMs(dev, 100); /* in ms possible values [20, 50, 100, 200, 500] */
//  status = VL53L1X_SetInterMeasurementInMs(dev, 200); /* in ms, IM must be > = TB */

	
	
   // 4 What is missing -- refer to API flow chart
   status = VL53L1X_StartRanging(dev);   // This function has to be called to enable the ranging (one measurement cycle to stabilize; writes to sensor and then gets status)
 // USED TO MAKE A DISTANCE MEASUREMENT
	
		for (uint16_t i = 0; i < NUM; i++){
			spin();
			dataReady = 0; // polling method
			while (dataReady == 0){
		  status = VL53L1X_CheckForDataReady(dev, &dataReady); //internal initialization and calibrations   ***READS GPIO STATUS REGISTER W/ I2C
          VL53L1_WaitMs(dev, 5); // USED TO CHECK IF RANGING DATA ARE READY
			}
				//7 read the data values from ToF sensor
			status = VL53L1X_GetRangeStatus(dev, &RangeStatus);		
			status = VL53L1X_GetDistance(dev, &Distance);					//The Measured Distance value
			distance_meas[i] = Distance;
			//if (RangeStatus == 1){
			//	distance_meas[i] = 0;
			//}
			
			//****************************************************** STATUS CHECK******************************************************
			measurement_Status(); // flash LED D2
			status = VL53L1X_ClearInterrupt(dev); /* 8 clear interrupt has to be called to enable next interrupt*/ // if not called, the next measurement is not triggered		
 
			
			// print the resulted readings to UART
			float angle = (float)i*PI/16; // 11.25 deg
			y[i] = (float)distance_meas[i]*cos(angle);
			z[i] = (float)distance_meas[i]*sin(angle);
			y_int[i] = (int)y[i];
			z_int[i] = (int)z[i];
			sprintf(printf_buffer,"%d, %d\r\n", y_int[i], z_int[i]);
			UART_printf(printf_buffer);
			//****************************************************** STATUS CHECK******************************************************
			UART_LED();
 
			SysTick_Wait10ms(50);
			}
	VL53L1X_StopRanging(dev);		
}
 
int main(void){
	//int input = 0;
	//uint8_t current_scan = 0;
	//initialize
	PLL_Init();	
	SysTick_Init();
	I2C_Init(); //communications between TOF and microcontroller
	UART_Init(); //commuications between microcontroller and PC; can use UART to print beginning of program to ensure functionality (debugging mehod)
	PortL_Init(); // motor
	PortF_Init(); // led
	PortJ_Init(); // buttons
	PortN_Init(); // led
	PortM_Init(); // output to check bus speed --> AD3
	
	
	while(1){
	//wait for the right transmition initiation code
			if (bus_button()){
					UART_printf("Bus test mode\r\n");
					BusClock_TestSignal();   // infinite loop until reset
			}
		
			if (sampling_button()){ // button press
				UART_printf("s\r\n"); // send s to MATLAB to start transmission
				visualize();
				go_Home();
				SysTick_Wait10ms(1);   // 10 ms pause between lines
			}
		}
}