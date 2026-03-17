// COMPENG 2DX3 Lab 5

//  Written by Christina Bridges and Alissa Guagliano
//  Feb 28, 2026
//  Last Update: March 2, 2026

#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "Systick.h"
#include "PLL.h"

/*
QUESTIONS
- why would we set the direction one at a time instead of the data (output value) only
- prior design had all PortE DIR enabled for output and checked for entire 8 bits of portM and the x[j] values
  which could be sensitive to variations in the entire output PortE sequence if there's any discrepancy with the output values
- are the pull-up resistors initialized in PortE_Init() necessary?
*/

#define ROWS	4 // number of rows and columns in keypad
#define	COLUMNS 4

volatile uint8_t binary_value = 0;			// INITIALIZATION of binary value (stored in memory and global variable for future milestone)

void PortE_Init(void){	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;		              // Activate the clock for Port E
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R4) == 0){};	      // Allow time for clock to stabilize
  
	GPIO_PORTE_DIR_R = 0b00001111;														// CHANGED: Initially set direction for all pins to inputs and switch to output only when setting row to 0
	GPIO_PORTE_DEN_R = 0b00001111;                        		// Enable PE0 to PE1 as digital pins
	//GPIO_PORTE_PUR_R = 0x0F;																	// ADDED: pull-up resistors so floating pins (in scan function below) are pulled high
	return; // DEBUGGER CURRENTLY HITTING HardFault_Handler (infinite loop B) bc an instruction tries to access invalid memory
	}

void PortM_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;                 // Activate the clock for Port M
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R11) == 0){};      // Allow time for clock to stabilize
		
	GPIO_PORTM_DIR_R = 0b00000000;       								      // Enable PM0 and PM3 as inputs 
  GPIO_PORTM_DEN_R = 0b00001111;														// Enable PM0 to PM3 as digital pins
	GPIO_PORTM_PUR_R = 0x0F;																	// pull-up resistors so input pins are initially pulled HIGH
	return;
}

void PortF_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;                 // Activate the clock for Port M
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5) == 0){};      // Allow time for clock to stabilize
		
	GPIO_PORTF_DIR_R = 0b00010001;       								      // Enable PM0 and PM3 as inputs 
  GPIO_PORTF_DEN_R = 0b00010001;														// Enable PM0 to PM3 as digital pins
	return;
}

void PortN_Init(void){
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;                 // Activate the clock for Port M
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R12) == 0){};      // Allow time for clock to stabilize
		
	GPIO_PORTN_DIR_R = 0b00000011;       								      // Enable PM0 and PM3 as inputs 
  GPIO_PORTN_DEN_R = 0b00000011;														// Enable PM0 to PM3 as digital pins
	return;
}

uint8_t KeyCodeToBinary(int code){ 		// remember to define functions before calling them lol   DONT RLLY USE BESIDES DEMO
	switch(code){												// NOTE THAT TABLE WRITES 3:0 NOT 0:3 --> need to REDO?
		case 0xBE: return 0x0;  // 0
    case 0x77: return 0x1;  // 1
    case 0xB7: return 0x2;  // 2
    case 0xD7: return 0x3;  // 3
    case 0x7B: return 0x4;  // 4
    case 0xBB: return 0x5;  // 5
    case 0xDB: return 0x6;  // 6
    case 0x7D: return 0x7;  // 7
    case 0xBD: return 0x8;  // 8
    case 0xDD: return 0x9;  // 9
    case 0xE7: return 0xA;  // A
    case 0xEB: return 0xB;  // B
    case 0xDE: return 0xC;  // C
    case 0xEE: return 0xD;  // D
    case 0x7E: return 0xE;  // *
    case 0xED: return 0xC;  // #
		default: return 0x0;         // NOTE: initialization for keycode is 0, so if nothing is pressed it will seem as if the 0 key was pressed
    }
}

void lightON(int pressed_key){ // light sequence N1 N0 F4 F0
	if ((pressed_key & 0b00000001) == 1 && (pressed_key & 0b00000010) == 2){
		GPIO_PORTF_DATA_R = 0b00010001; // turn on D3 and D4
	}
	else if ((pressed_key & 0b00000001) == 1 || (pressed_key & 0b00000010) == 2){
		if((pressed_key & 0b00000001) == 1)
		{
			GPIO_PORTF_DATA_R = 0b00000001; // turn on D4
		}
		else{
			GPIO_PORTF_DATA_R = 0b00010000; // turn on D3
		}
	}
	else{
			GPIO_PORTF_DATA_R = 0b00000000;  // turn off both
		}

	if ((pressed_key & 0b00000100) == 4 && (pressed_key & 0b00001000) == 8){
		GPIO_PORTN_DATA_R = 0b00000011;  // turn on D1 and D2
	}
	else if ((pressed_key & 0b00000100) == 4 || (pressed_key & 0b00001000) == 8)
	{
		if ((pressed_key & 0b00000100) == 4){
			GPIO_PORTN_DATA_R = 0b00000001;  // turn on D1
		}
		else{
			GPIO_PORTN_DATA_R = 0b000000010;  // turn on D2
		}
		
	}
	else{
			GPIO_PORTN_DATA_R = 0b00000000;  // turn off both
		}
	
}

char scan(){ // SCANNING ROWS 1-4 by setting output bit for that port to 0
	uint8_t x[4] = {0b00001110, 0b00001101, 0b00001011, 0b00000111}; 						// CHANGED: modified the set to activate one direction at a time (output corresponding to bit) for PE pins
	uint8_t y[4] = {0b00000001, 0b00000010, 0b00000100, 0b00001000}; 						// for bitwise comparison with input
	volatile int row_location = -1;		// CHANGED: good practice, set the index to a value outside of the array range
	volatile int column_location = -1;
	
	const char key[4][4]={{1,2,3,'A'},{4,5,6,'B'},{7,8,9,'C'},{'*',0,'#','D'}}; // array associating each key identity to the mapped i and j array
	const uint8_t binary_table[4][4] = {{0x1, 0x2, 0x3, 0xA},{0x4, 0x5, 0x6, 0xB},{0x7, 0x8, 0x9, 0xC},{0xE, 0x0, 0xF, 0xD}};
		
	volatile char watch_key = 'N';			// ADDED: if no key is pressed, return N     CHANGED: volatile type
	int flag = 0;		// ADDED: if a key is found, set to 1 and break out of the outer loop (rows loop)
		
	uint8_t input;
		
	for (int i = 0; i < ROWS; i++){
		// Enable PE0 to PE1 as outputs one at a time
		
		GPIO_PORTE_DATA_R = x[i]; 			// CHANGED: DATA TO DIR to enable 
		//GPIO_PORTE_DATA_R = 0b00000000;		// ADDED: set all bits to 0. Since only one pin is enabled for output the other values should be high
		SysTick_Wait10ms(1); 						// ADDED: "debouncing", added time delay of 10 ms to wait for register to update
		
		
		for (int j = 0; j < COLUMNS; j++){
			input = GPIO_PORTM_DATA_R;
			if ((input & y[j]) == 0){			// SINCE x[] IS NOW CHANGED (1's and 0's reversed) --> sets all other bits to 0 and if pin input data is 0 at bit we are checking then enters loop 
				row_location = i; 
				column_location = j;
				flag = 1;
				break; // added a break here in case two keys are pressed simultanesouly but program runs so fast that likely doesn't matter
				}
			}
			if (flag == 1){
				break;
			}
		
	
		
	}
	if ((row_location != -1) && (column_location != -1)){ 			// ADDED: ensure not accessing out of bounds of array if a key is not pressed
					watch_key = key[row_location][column_location]; 			//access character in 4 by 4 array
			
				int key_code = ((input & 0b00001111) << 4) | x[row_location]; // D1 --> DE
				// row location determines which PortE output pin was active when the key was pressed (4 LSBs for binary data)
				// bitwise AND operator isolates PM(3:0) only, then shifts the data to the left by 4 positions so 4 MSBs of key_code make up the PortM (input) binary data
				binary_value = KeyCodeToBinary(key_code);
				uint8_t pressed_key_bin = binary_table[row_location][column_location]; // Accesses the unique binary code corresponding to the key code
				lightON(pressed_key_bin);
		}
	return watch_key;
}


int main(void){
	SysTick_Init();
	PortE_Init();
	PortM_Init();
	PortF_Init();
	PortN_Init();
	
	GPIO_PORTF_DATA_R = 0b00000000;
	GPIO_PORTN_DATA_R = 0b00000000;
	
	volatile char pressed_key;			// ADDED: if no key is pressed, return N

	while(1){// Keep checking if a button is pressed 
		pressed_key = scan(); 
		SysTick_Wait10ms(1);
		// code for other milestones, what happened when pressed?
	}		
}