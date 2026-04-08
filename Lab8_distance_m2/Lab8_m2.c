/* The VL53L1X is run with default firmware settings.


Last updated: March 22, 2026
Christina Bridges
lab 8 milestone 2 modified from studio code

*/
#include <stdint.h>
#include <math.h> // for trig functions
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"
//COM7 UART

#define CW	1
#define	CCW 0
#define NUM 8
#define PI 3.141592654

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

//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}

//XSHUT     This pin is an active-low shutdown input; 
//					the board pulls it up to VDD to enable the sensor by default. 
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}

void spin(uint8_t direction, uint32_t step_num){																			// Complete function spin to implement the Full-step Stepping Method
	uint32_t delay = 1;															// Does your motor spin clockwise or counter-clockwise?
	//float factor = 140; //control to experimentally determine the smallest time delay possible between steps --> discovered as 1.0 factor exactly, so 1.4 ms delay is the minimum
	
	for(uint32_t i = 0; i < step_num; i++){												// What should the upper-bound of i be for one complete rotation of the motor shaft?  i = 2048/4 steps = 512 steps
		if (direction == CW) {
			GPIO_PORTL_DATA_R = 0b00000011;								// get through 512 teeth for one change of the poles
			SysTick_Wait10ms(delay);											// What if we want to reduce the delay between steps to be less than 10 ms?
			GPIO_PORTL_DATA_R = 0b00000110;													// Complete the missing code.
			SysTick_Wait10ms(delay);
			GPIO_PORTL_DATA_R = 0b00001100;													// Complete the missing code.
			SysTick_Wait10ms(delay);
			GPIO_PORTL_DATA_R = 0b00001001;													// Complete the missing code.
			SysTick_Wait10ms(delay);
		}
		else if (direction == CCW){
			GPIO_PORTL_DATA_R = 0b00001001;								// get through 512 teeth for one change of the poles
			SysTick_Wait10ms(delay);											// What if we want to reduce the delay between steps to be less than 10 ms?
			GPIO_PORTL_DATA_R = 0b00001100;													// Complete the missing code.
			SysTick_Wait10ms(delay);
			GPIO_PORTL_DATA_R = 0b00000110;													// Complete the missing code.
			SysTick_Wait10ms(delay);
			GPIO_PORTL_DATA_R = 0b00000011;													// Complete the missing code.
			SysTick_Wait10ms(delay);
		
		}
		GPIO_PORTL_DATA_R = 0b00000000;
	}
}

//*********************************************************************************************************
//*********************************************************************************************************
//***********					MAIN Function				*****************************************************************
//*********************************************************************************************************
//*********************************************************************************************************
uint16_t	dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral
int status=0;

int main(void) {
  uint8_t byteData, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
  uint16_t wordData;
  uint16_t Distance;
  uint16_t SignalRate; // useful signal rate; indicates the received signal power (want this number to be HIGH to be confident about our measurement)
  uint8_t RangeStatus;
  uint8_t dataReady;
	uint8_t meas_num = 0;
	
	uint16_t distance_meas[NUM];
	float x[NUM];
	float y[NUM];
	int x_int[NUM];
	int y_int[NUM];

	//initialize
	PLL_Init();	
	SysTick_Init();
	onboardLEDs_Init();
	I2C_Init(); //communications between TOF and microcontroller
	UART_Init(); //commuications between microcontroller and PC; can use UART to print beginning of program to ensure functionality (debugging mehod)
	PortL_Init();

	UART_printf("Hello, UART working!\r\n");
		int mynumber = 1;
	sprintf(printf_buffer,"2DX ToF Program Studio Code %d\r\n",mynumber);
	UART_printf(printf_buffer);
	// 1 Wait for device booted
	while(sensorState==0){ // step is ESSENTIAL
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(10);
  }
	FlashAllLEDs();
	UART_printf("ToF Chip Booted!\r\n Please Wait...\r\n");
	
	status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
	
  /* 2 Initialize the sensor with the default setting  */
  status = VL53L1X_SensorInit(dev);
	Status_Check("SensorInit", status);

	
  /* 3 Optional functions to be used to change the main ranging parameters according the application requirements to get the best ranging performances */
//  status = VL53L1X_SetDistanceMode(dev, 2); /* 1=short, 2=long */
//  status = VL53L1X_SetTimingBudgetInMs(dev, 100); /* in ms possible values [20, 50, 100, 200, 500] */
//  status = VL53L1X_SetInterMeasurementInMs(dev, 200); /* in ms, IM must be > = TB */

    // 4 What is missing -- refer to API flow chart
    status = VL53L1X_StartRanging(dev);   // This function has to be called to enable the ranging (one measurement cycle to stabilize; writes to sensor and then gets status)

	
	// Get the Distance Measures 50 times
	for(int i = 0; i < NUM; i++) {
		meas_num++;
		spin(CCW, 64); 		// 512/8 = 45 deg angles
		// 5 wait until the ToF sensor's data is ready
		dataReady = 0; // polling method
	  while (dataReady == 0){
		  status = VL53L1X_CheckForDataReady(dev, &dataReady); //internal initialization and calibrations   ***READS GPIO STATUS REGISTER W/ I2C
          FlashLED3(1);
          VL53L1_WaitMs(dev, 5);
	  }
	  
		//7 read the data values from ToF sensor
		status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
	  status = VL53L1X_GetDistance(dev, &Distance);					//The Measured Distance value
		status = VL53L1X_GetSignalRate(dev, &SignalRate);
		distance_meas[i] = Distance;
    
		FlashLED4(1);

	  status = VL53L1X_ClearInterrupt(dev); /* 8 clear interrupt has to be called to enable next interrupt*/ // if not called, the next measurement is not triggered
		
		float angle = (float)i*(PI/4.0); // for 45 deg angle use 4.0 to get radians version
		x[i] = (float)distance_meas[i]*cos(angle);
		y[i] = (float)distance_meas[i]*sin(angle);
		x_int[i] = (int)x[i];
		y_int[i] = (int)y[i];
		
		// print the resulted readings to UART
		sprintf(printf_buffer,"Status: %u, meas_num: %u, Distance: %u mm, x = %d mm, y = %d mm\r\n", RangeStatus, meas_num, Distance, x_int[i], y_int[i]);
		UART_printf(printf_buffer);
	  SysTick_Wait10ms(50);
  }
  
	VL53L1X_StopRanging(dev);
  while(1) {}

}

