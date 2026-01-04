#include <LPC17xx.h>
#include "reg_masks.h"
#include "funciones.h"

#define ALL 2
#define RIGHT 0
#define LEFT 1
#define RED 34
#define PPR 11
#define pi 3.14159265358979323846f
#define DW 2.6
#define DR 27

void enc_config(void){
	LPC_PINCON->PINSEL0 |= (0x03 <<8); //Configuro pin PO.4 como CAP2.0
	LPC_PINCON->PINSEL1 |= (0x03 <<14); //Configuro pin P0.23 como CAP3.0
	LPC_SC->PCONP |= (PCTIM2 | PCTIM3); //Habilito el TIMER 3 y el TIMER 2
	LPC_TIM2->TCR = 0x02; //Reseteo el TIMER 2 y desactivo para poder configurarlo
	LPC_TIM3->TCR = 0x02; // Lo mismo para el TIMER 3
	LPC_TIM2->CTCR &= ~(0x30 <<0); // Lo configuro para que use el CAP2.0
	LPC_TIM2->CTCR |= (0x03 <<0); // Lo configuro para qur TC se incremente tanto en flanco de subida como bajada
	LPC_TIM3->CTCR &= ~(0x30 <<0); // Lo configuro para que use CAP3.0
	LPC_TIM3->CTCR |= (0x03 <<0); // Lo configuro para qeu TC se incremente en ambos flancos
	LPC_TIM2->TCR = 0x01; //Activo TIMER 2
	LPC_TIM3->TCR = 0x01; // Activo TIMER 3
	NVIC_EnableIRQ(TIMER2_IRQn); //Habilito la interrupción TIMER2
	NVIC_EnableIRQ(TIMER3_IRQn); //Habilito la interrupción TIMER3
	NVIC_SetPriority(TIMER2_IRQn,1); //Establezco la prioridad de la interrupción
	NVIC_SetPriority(TIMER3_IRQn,2); //Establezco la prioridad de la interrupción
}
void enc_set_dist_limit(int dist_cm, int wheel){
	float n;
	n=(dist_cm*RED*PPR*2)/(pi*DW); //Calculo el valor de tc
	if(wheel == RIGHT){ //Rueda derecha
		LPC_TIM2->MR0 = n-1; //Establezco el valor MR0 para que sea igual a n
		LPC_TIM2->MCR |=(0x3 <<0); //Se genera una interrupcion cuando tc=MR0 y resetea tc
	}
	else if(wheel == LEFT){ //Rueda izquierda
		LPC_TIM3->MR0 = n-1; //Establezco el valor MR0 para que sea igual a n
		LPC_TIM3->MCR |=(0x3 <<0); //Se genera una interrupcion cuando tc=MR0 y resetea tc
	}
	else if(wheel == ALL) { //Ambas ruedas
		LPC_TIM2->MR0 = n-1;
		LPC_TIM2->MCR |=(0x3 <<0);
		LPC_TIM3->MR0 = n-1;
		LPC_TIM3->MCR |=(0x3 <<0);
	}
}
void enc_set_angle_limit(float angle_robot, int wheel){
	float na;
	na=(angle_robot*RED*2*PPR*DR)/(360*DW); //Calculo el valor de tc
	if(wheel == RIGHT) { //Rueda derecha
		LPC_TIM2->MR0 = na; //Establezco el valor MR0 para que sea igual a na
		LPC_TIM2->MCR |=(0x3 << 0); //Se genera una interrupcion cuando tc=MR0 y resetea tc
	}
	else if(wheel == LEFT) { //Rueda izquierda
		LPC_TIM3->MR0 = na; //Establezco el valor MR0 para que sea igual a na
		LPC_TIM3->MCR |=(0x3 <<0); //Se genera una interrupcion cuando tc=MR0 y resetea tc
	}
	else if(wheel == ALL){ //Ambas ruedas
		LPC_TIM2->MR0 = na;
		LPC_TIM2->MCR |=(0x3 <<0);
		LPC_TIM3->MR0 = na;
		LPC_TIM3->MCR |=(0x3 <<0);
	}
}
float enc_get_dist(int wheel) {
	float distr;
	float nr;
	float distl;
	float nl;
	if(wheel == RIGHT) {
		nr = LPC_TIM2->MR0; //Almaceno el tc en la variable nr
		distr=(pi*DW)/(RED*2*PPR)*nr; //Calculo la distancia recorrida por la rueda derecha
		return distr;
	}
	else if(wheel == LEFT) {
		nl = LPC_TIM3->MR0; //Almaceno el tc en la variable nl
		distl=(pi*DW)/(RED*2*PPR)*nl; //Calculo la distancia recorrida por la rueda izquierda
		return distl;
	}
	else if(wheel == ALL){ //Ambas ruedas
		nr = LPC_TIM2->MR0;
		distr=(pi*DW)/(RED*2*PPR)*nr;
		nl = LPC_TIM3->MR0;
		distl=(pi*DW)/(RED*2*PPR)*nl;
		return distl;
		return distr;
	}
}
