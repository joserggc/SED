#include <LPC17xx.h>
#include <stdio.h>
#include <math.h>
#include "reg_masks.h"
#include "funciones.h"
#include "GLCD.h"

#define F_cpu         100e6        // Frecuencia del sistema
#define F_pclk        F_cpu/4      // PCLK configurado por defecto
#define voltageDiv    2.568
#define Vref          3.3
#define M_PI          3.14159265358979323846f
#define N_POINTS      16
#define DAC_N_BITS    10
#define DAC_N_LEVELS (1U << DAC_N_BITS)
#define DAC_MID_RANGE (1U << (DAC_N_BITS-1))

static int16_t alarm_table[N_POINTS];
static int alarm_idx;
volatile float battery_voltage;
char batBuffer[4];

void float_to_char(float number, char *buffer) {
    int int_part = (int)number;
    float fractional_part = number - int_part;
    char *ptr = buffer;
    int fractional_as_int = (int)((fractional_part * 1000000) + 0.5);
    int i;

    ptr += sprintf(ptr, "%d", int_part);
    *ptr++ = '.';

    for (i = 0; i < 6; i++) {
        *ptr++ = '0' + (fractional_as_int / 100000);
        fractional_as_int %= 100000;
        fractional_as_int *= 10;
    }

    *ptr = '\0';
}

void battery_sampling_init(float Ts)
{
  LPC_SC->PCONP |= PCADC; //Habilita ADC
  LPC_PINCON->PINSEL1 &= ~(3 << 16); // Borrar bits para P0.24
  LPC_PINCON->PINSEL1 |= (1 << 16); // Configurar P0.24 como AD0.1
  LPC_PINCON->PINMODE1 &= ~(3 << 16); // Borrar bits de modo
  LPC_PINCON->PINMODE1 |= (2 << 16); // Modo sin pull-up ni pull-down

  LPC_ADC->ADCR = (1<<1) | (1<<8) | (1<<21) | (0x4<<24); // PDN=1, Seleccionamos canal 1, CLKDIV = 2, conversion se inicia con MAT0.1

  LPC_ADC->ADINTEN |= ADINTEN1;                // AD0.1 interrupción
	
	LPC_SC->PCONP |= PCTIM0;	//Habilito el TIMER0
	LPC_TIM0->MCR = (1<<4); //Reset MR1
	LPC_TIM0->EMR = (0x3<<6); //Pongo modo Toggle;
	LPC_TIM0->MR1 = (F_pclk * Ts/2) - 1; //Configuro tiempo en Ts
	LPC_TIM0->TCR |= 1; //Enciendo el TIMER 0
	
  NVIC_SetPriority(ADC_IRQn,10); //Pongo maxima prioridad al ADC
  NVIC_EnableIRQ(ADC_IRQn); //Habilito IRQ de ADC
}

void battery_sampling_stop(void) {
	LPC_ADC->ADCR &= ~(1<<21);
}

void alarm_init(void) {
	int i;
	float x;
	LPC_PINCON->PINSEL1 |= (2<<20); //DAC salida = P0.26 (AOUT)
	LPC_PINCON->PINMODE1 |= (2<<20); //deshabilitas pull up y pull down
	LPC_DAC->DACCTRL = 0;
	LPC_SC->PCONP |= PCTIM1; //Habilito timer 1
	LPC_TIM1->MCR = (1<<0) | (1<<1); //Reset en MR0 e interrupcion
	NVIC_EnableIRQ(TIMER1_IRQn); //Habilito IRQ de TIMER1
	
	for(i=0; i < N_POINTS; i++) {																				//Inicializo la variable alarm_table
		x = DAC_MID_RANGE+(DAC_MID_RANGE-1)*sinf((2*M_PI/N_POINTS)*i);
		alarm_table[i] = ((uint16_t)x) << 6;
	}
	alarm_idx = 0;
}

void alarm_gen_sample(void) {
	LPC_DAC->DACR = (uint32_t) alarm_table[alarm_idx];        
	alarm_idx = (alarm_idx == (N_POINTS-1))? 0 : alarm_idx+1;
}

void alarm_set_freq(int freq_hz) {
	if(F_pclk / LPC_TIM1->MR0 != freq_hz) {
		LPC_TIM1->TCR |= (3<<0); //Reset Timer1 y Stop
		if (freq_hz != 0) {
			LPC_TIM1->TCR &= ~(3<<0); //Clear
			LPC_TIM1->MR0 = (F_pclk / freq_hz);
			LPC_TIM1->TCR |= (1<<0); //Iniciar TIMER1
		}
		else LPC_TIM1->TCR |= (3<<0);
	}
}

void TIMER1_IRQHandler(void) {
	LPC_TIM1->IR = MR0_INT; //Clear flag
	alarm_gen_sample();
}

void alarm_enable(int enable) {
	if(enable)
		LPC_TIM1->TCR |= CNT_EN; //Count enabled
	else
		LPC_TIM1->TCR &= ~CNT_EN; //Count disabled
}
void ADC_IRQHandler(void) {
	battery_voltage = ((LPC_ADC->ADGDR >>4) & 0x0FFF)*Vref*voltageDiv/4096.0;  //Cojo el valor de la bateria de la conversion del ADC
	float_to_char(battery_voltage,batBuffer);
	GUI_Text(172,60,(uint8_t *)batBuffer,Blue,Black);
}