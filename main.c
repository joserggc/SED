#include <LPC17xx.h>
#include "reg_masks.h"
#include "funciones.h"
#define F_CPU       (SystemCoreClock)
#define F_PCLK      (F_CPU/4)

#include "uart.h"
#include "GLCD.h" 
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static char lcd_buffer[256];
#define FONT_W  8
#define FONT_H  16
#define ALL 2
#define RIGHT 0
#define LEFT 1

char msg1[] = "Pon el comando:\n\r";
char rx_buffer[128];
char msg2[] = "Tu mensaje es:\n\r";
volatile uint8_t u8_enconder_token;
volatile uint8_t buttonPressed;

int char_to_int(const char *str)
{
  int result = 0;
  int i = 0;
  int length= strlen(str);
// Convertir cada carácter al entero correspondiente
  while (str[i] >= '0' && str[i] <= '9') 
  {
      result = result * 10 + (str[i] - '0');
      i++;
  }
  if(length != i) return -1;
  else return result;
}

void delimitedChar(const char* str, char* returnPointer, uint8_t from, uint8_t to)
{
  int i,j = 0;
  for(i = from; i <= to ; i++)
  {
    returnPointer[j] = str[i];
    j++;
  }
  returnPointer[j++] = '\0';
}

void EINT2_config(void) {
    // Configura el pin P2.11 como EINT2
    LPC_PINCON->PINSEL4 &= ~(3 << 24); // Limpia bits 22 y 23
    LPC_PINCON->PINSEL4 |= (1 << 24);  // Configura P2.12 como EINT2

    // Configura el tipo de interrupción en el flanco de bajada
    LPC_SC->EXTMODE |= (1 << 2);       // Configura EINT2 como interrupción por flanco
    LPC_SC->EXTPOLAR |= (1 << 2);      // Configura flanco de subida

    // Habilita la interrupción de EINT2 en el NVIC
    NVIC_EnableIRQ(EINT2_IRQn);

    // Limpia cualquier flag previo
    LPC_SC->EXTINT |= (1 << 2);
}
int main() {

	int length;
  int ret;
	int index = 0;
	char strbuffer[30];
	int velocidad;
	float veloreal;
	int distancia;
	int distang;
	
	
	config_pwm(1); //Configuro PWM a mayor freceuncia posible
	EINT2_config(); //Configuro el resto de perifericos
	para_pwm(ALL);
	enc_config();
	LCD_Initializtion();
	alarm_init();
  // Serial port initialization:
  ret = uart0_cfg(9600);
  if(ret < 0) {
    return -1;
	}
 ret = uart0_send_string(msg1); //Pregunta por el comando que quiere utilizar
  if (!ret) {
    // Wait until the string is transfered!
    while(uart0_get_tx_busy());
  }
 
 ret = uart0_recv_string(rx_buffer); //Escribes el proceso que quieres que ejecute el robot
	if (!ret) {
    while(1) {
      // Check if a new string has been received
      if (!uart0_get_rx_busy()) {
        break;
      }
    }
  }
 ret = uart0_send_string(msg2); //Mensaje de comprobación del comando
	if (!ret) {
    // Wait until the string is transfered!
    while(uart0_get_tx_busy());
  }
	ret = uart0_send_string(rx_buffer); //Comprobación del comando
	if (!ret) {
    // Wait until the string is transfered!
    while(uart0_get_tx_busy());
  }
	length=strlen(rx_buffer); //Guardas la longitud del comando
	ret = uart0_send_string("\n");
	if (!ret) {
    // Wait until the string is transfered!
    while(uart0_get_tx_busy());
  }
	buttonPressed = 0; //Variable con la que se procedera a la inicializacion
	ret = uart0_send_string("Presiones KEY2 para proceder\n"); //Espera hasta que se pulse KEY2
	if (!ret) {
    // Wait until the string is transfered!
    while(uart0_get_tx_busy());
  }
	while (!buttonPressed);
	GUI_Text(0,0,(uint8_t *)"Valor del angulo: ",Blue,Black); //Pongo los valores pedidos en el display
  GUI_Text(0,30,(uint8_t *)"Valor de la distancia: ",Blue,Black);
  GUI_Text(0,60,(uint8_t *)"Valor de la bateria: ",Blue,Black);
	battery_sampling_init(1); //Configuro el muestreo de la bateria a 1 seg
	 while(index < length - 1 ) {
		 switch(rx_buffer[index]) {
			 case 'V': //Caso velocidad
			 {
				 delimitedChar(rx_buffer,strbuffer,1,2);
				 velocidad = char_to_int(strbuffer); //convierto el char a int y lo meto en la variable velocidad 
				 veloreal = (float)velocidad/100.0; 
				 break;
			 }
			 case 'A': //Caso ir hacia delante XX distancia en cm
			 {
				 u8_enconder_token=0;
				 delimitedChar(rx_buffer,strbuffer,index+1,index+2);
				 distancia = char_to_int(strbuffer); //Convierto el valor y guardo la distancia que tiene que recorrer
				 enc_set_dist_limit(distancia,ALL); //Establezco la distancia a la que tiene que parar
				 set_duty_pwm(veloreal,veloreal);				 //Activo el pwm a la velocidad que quiero que se ejecute
				 while(!u8_enconder_token);
				 para_pwm(ALL);
				 u8_enconder_token = 0;
				 break;
			 }
			 case 'D':
			 {
				 u8_enconder_token = 0;
				 delimitedChar(rx_buffer,strbuffer,index+1,index+2); //Covertir el dato
				 distang = char_to_int(strbuffer); //Guardo la distancia que tiene que recorrer despues de realizar el giro
				 enc_set_angle_limit(90,LEFT); //Establezco el angulo de giro
				 set_duty_pwm(veloreal,0); //Realizao el giro dch
				 while(!u8_enconder_token);
				 para_pwm(ALL);
				 u8_enconder_token = 0; //Limpio la variable de token de paro
				 enc_set_dist_limit(distang,ALL); //Establezco la distancia de recorrido 
				 set_duty_pwm(veloreal, veloreal); //Realizo la distancia 
				 while(!u8_enconder_token);
				 para_pwm(ALL);
				 u8_enconder_token = 0;
				 break;
			 }
			 case 'I':
			 {
				 u8_enconder_token = 0;
				 delimitedChar(rx_buffer,strbuffer,index+1,index+2); //Convierto el dato
				 distang = char_to_int(strbuffer); //Guardo la distancia que tiene que recorrer despues de realizar el giro
				 enc_set_angle_limit(90,RIGHT); //Establezco el angulo de giro
				 set_duty_pwm(0,veloreal); //Realizo giro izq
				 while(!u8_enconder_token);
				 para_pwm(ALL);
				 u8_enconder_token = 0; //Limpio la variable de token de paro
				 enc_set_dist_limit(distang,ALL); //Establezco la distancia de recorrido
				 set_duty_pwm(veloreal,veloreal); //Realizo la distancia
				 while(!u8_enconder_token);
				 para_pwm(ALL);
				 u8_enconder_token = 0;
				 break;
			 }
		 }
		 index +=3;
	 }
	 
	 ret = uart0_send_string("El proceso ha acabado\n"); //Pregunta por el comando que quiere utilizar
  if (!ret) {
    // Wait until the string is transfered!
    while(uart0_get_tx_busy());
  }
  while(1);
	return 0;
}
void EINT2_IRQHandler(void) {
    // Verifica si la interrupción fue generada por EINT2
    if (LPC_SC->EXTINT & (1 << 2)) {
        // Aquí puedes realizar la acción deseada si el botón fue pulsado
        buttonPressed = 1;

        // Limpia el flag de interrupción de EINT2
        LPC_SC->EXTINT |= (1 << 2);
    }
}
void TIMER3_IRQHandler(void) {
	u8_enconder_token++;
	LPC_TIM3->IR = (1<<0);
}
void TIMER2_IRQHandler(void) {
	u8_enconder_token++;
	LPC_TIM2->IR = (1<<0);
}
