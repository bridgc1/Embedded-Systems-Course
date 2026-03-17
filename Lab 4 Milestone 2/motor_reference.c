#include <stdint.h>
#include "msp432e401y.h"
#include "PLL.h"
#include "SysTick.h"

#define CW  1
#define CCW 0

#define STEPS_PER_ROTATION 512

void PortL_Init(void);
void Stepper_Step(uint32_t steps, uint8_t direction);
void delay_ms(uint32_t ms);

/* ---------------- PORT L INIT ---------------- */
void PortL_Init(void){
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R10;     // Enable clock for Port L
    while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R10)==0){};

    GPIO_PORTL_DIR_R |= 0x0F;      // PL3-PL0 output
    GPIO_PORTL_DEN_R |= 0x0F;      // Digital enable
    GPIO_PORTL_AFSEL_R &= ~0x0F;   // GPIO mode
    GPIO_PORTL_AMSEL_R &= ~0x0F;   // Disable analog
}

/* ---------------- DELAY FUNCTION ---------------- */
void delay_ms(uint32_t ms){
    while(ms--){
        SysTick_Wait1ms();
    }
}

/* ---------------- STEPPER FUNCTION ---------------- */
/* Full-step dual-coil sequence:
   1001
   0011
   0110
   1100
*/

void Stepper_Step(uint32_t steps, uint8_t direction){

    uint8_t sequence[4] = {0x09, 0x03, 0x06, 0x0C};
    uint32_t i, j;

    for(i = 0; i < steps; i++){

        for(j = 0; j < 4; j++){

            if(direction == CW)
                GPIO_PORTL_DATA_R = sequence[j];
            else
                GPIO_PORTL_DATA_R = sequence[3 - j];

            delay_ms(5);   // ?? Replace 5 with your experimentally determined minimum
        }
    }

    GPIO_PORTL_DATA_R = 0x00;  // Turn off coils
}

/* ---------------- MAIN ---------------- */
int main(void){

    PLL_Init();
    SysTick_Init();
    PortL_Init();

    // 2 rotations clockwise
    Stepper_Step(2 * STEPS_PER_ROTATION, CW);

    delay_ms(1000);

    // 1 rotation counter-clockwise
    Stepper_Step(STEPS_PER_ROTATION, CCW);

    while(1);
}