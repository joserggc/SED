#include <LPC17xx.h>
#include "reg_masks.h"
#include "funciones.h"
#define F_CPU (SystemCoreClock)
#define F_PCLK (F_CPU/4)
#define F_pwm 500
#define RUEDA_IZQ 1
#define RUEDA_DCHA 0
#define ALL 2
#define RIGHT 0
#define LEFT 1


void config_pwm(float Tpwm)
{
  LPC_SC->PCONP |= (1<<6); 				// Enciendes el PWM por eso es el 6
  LPC_PINCON->PINSEL3 |=(0x2 << 10) | (0x2 << 16); //P1.21 como PWM1.3 Y P1.24 como PWM1.5	
	LPC_GPIO0->FIODIR |= 0x03;			//Pones pines P0.0 y P0.1 como salida
	LPC_GPIO0->FIOCLR = 0x03;			//Pones los pines de dirección en 0
  LPC_PWM1->MR0 = (F_PCLK*Tpwm) - 1;	// set frequency of PWM1   
  LPC_PWM1->PCR |= (1<<11) | (1<<13);	// Activa PMW1.3 y PMW1.5
  LPC_PWM1->MCR |= (1<<1); 						// reset timer on Match0
  LPC_PWM1->TCR |= (1<<0) | (1<<3); 	// start timer, timer enable si ponemos el bit 1 a 1 no arrancaría
}
void set_period(float Tpwm){
	LPC_PWM1->MR0 = (F_PCLK*Tpwm) - 1;
}
float pwm_get_period(){
	return ((LPC_PWM1->MR0 +1)/F_PCLK);
}
void set_duty_pwm(float izq, float dch)  //funcion para poner velocidad motor y sentido de este
{
	if(izq>=0) //hacia delante, rueda izquierda
	{
		LPC_GPIO0->FIOCLR = 0x02;
		LPC_PWM1->MR3 = LPC_PWM1->MR0 * izq;	//MR3 sera menor que MR0
	}
	else if(izq < 0) //hacia atras, rueda izquierda
	{
		LPC_GPIO0->FIOSET = 0x02;
		LPC_PWM1->MR3 = LPC_PWM1->MR0*(1 + izq);
	}
	if(dch>=0) //hacia delante, rueda derecha
	{
		LPC_GPIO0->FIOCLR = 0x01;
		LPC_PWM1->MR5 = LPC_PWM1->MR0 * dch;
	}
	else if(dch < 0) //hacia detrás, rueda derecha
	{
		LPC_GPIO0->FIOSET = 0x01;
		LPC_PWM1->MR5 = LPC_PWM1->MR0* (1 + dch);
	}
	LPC_PWM1->LER |=(1<<0) | (1<<3) | (1<<5);
}
float pwm_get_duty_cycle(int wheel) 
{
  float duty;
  if(wheel == RUEDA_IZQ) // Si es la rueda izquierda
  { 
    if (LPC_GPIO0->FIOPIN & 0x02)
    {
      // Rueda en sentido antihorario (negativo)
      duty = -((LPC_PWM1->MR0 - LPC_PWM1->MR3) / (float)LPC_PWM1->MR0);
    } 
    else 
    {
      // Rueda en sentido horario (positivo)
      duty = LPC_PWM1->MR3 / (float)LPC_PWM1->MR0;
    }
  } 
  else if(wheel == RUEDA_DCHA) // Si es la rueda derecha
  { 
    if (LPC_GPIO0->FIOPIN & 0x01)
    {
      // Rueda en sentido antihorario (negativo)
      duty = -((LPC_PWM1->MR0 - LPC_PWM1->MR5) / (float)LPC_PWM1->MR0);
    }
    else
    {
      // Rueda en sentido horario (positivo)
      duty = LPC_PWM1->MR5 / (float)LPC_PWM1->MR0;
    }
  }  
  return duty;
}
void para_pwm(int wheel){	//Función para parar el motor
	if (wheel == ALL) {
	set_duty_pwm(0,0);
	LPC_GPIO0->FIOCLR |= 0x01;
	LPC_GPIO0->FIOCLR |=0x02;
	}
	else if(wheel == RIGHT) {
		set_duty_pwm(1,0);
		LPC_GPIO0->FIOCLR |= 0x01;
	}
	else if(wheel == LEFT) {
		set_duty_pwm(0,1);
		LPC_GPIO0->FIOCLR |= 0x02;
	}
}