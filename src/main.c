#include <stdio.h>
#include "Board_LED.h"
#include "GPIO_LPC17xx.h"
#include "PIN_LPC17xx.h"                // Keil::Device:PIN
#include "LPC17xx.h"
#include "core_cm3.h"
#include "math.h"

#define tabSize 15000

int ADC_counter = 0;

void SPI_IRQHandler(void);
void EINT0_IRQHandler(void);
void TIMER0_IRQHandler(void);

void sendString(char *str)
{
	while(*str != '\0')
	{
		while ((LPC_UART0->LSR & (1 << 5)) == 0);
		LPC_UART0->THR = *str;
		str++;
	}
}

void FLASH_read(void);
void FLASH_write(void);

int DAC_counter = 0;
int value = 0;
short tab[tabSize];

void initUart()
{
	PIN_Configure(0, 2, 1, 0, 0); 
	PIN_Configure(0, 3, 1, 0, 0); 
	
	LPC_UART0->LCR = 3 | (1 << 7);
	LPC_UART0->DLL = 14;
	LPC_UART0->DLM = 0;
	LPC_UART0->LCR = 3;
}

void initADC()
{
	LPC_SC->PCONP = LPC_SC->PCONP | 1 << 12; // ADC power on
	LPC_ADC->ADCR = 2 | 1 << 21 | 1 << 8; // set adc clock divider (by 2) | set conversion start after pressing button no 2
	
	LPC_PINCON->PINSEL1 = LPC_PINCON->PINSEL1  | 1 << 16; // configure pin function to AD0.1
	LPC_PINCON->PINSEL4 = LPC_PINCON->PINSEL4 | 1 << 20; // connect button 1 to correct pin
	LPC_PINCON->PINSEL4 = LPC_PINCON->PINSEL4 | 1 << 22; // connect button 2 to correct pin

	LPC_ADC->ADINTEN = 2; // enable interrupts for AD0.1
	
	NVIC_EnableIRQ(ADC_IRQn); // setup NVIC
	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT1_IRQn);
}

void initDAC()
{
	LPC_PINCON->PINSEL1 = LPC_PINCON->PINSEL1  | 2 << 20; // connect out to correct pin (pinsel function)
}

void initTimer0()
{
	LPC_TIM0->PR = 0;
	LPC_TIM0->MR0 = 2500;
	LPC_TIM0->MCR = 3;
	LPC_TIM0->TCR = 1;
}

void initTimer1()
{
	LPC_TIM1->PR = 0;
	LPC_TIM1->MR0 = 2500;
	LPC_TIM1->MCR = 3;
	LPC_TIM1->TCR = 1;
}

int main()
{		
	LED_Initialize();
	initTimer0();
	initTimer1();
	initUart();
	initADC();
	initDAC();
	sendString("Press KEY2 to start recording\n\n");
	
	while(1);
}

void ADC_IRQHandler(void)
{
	int res;
	int i;
	int wynik;

	wynik = (LPC_ADC->ADDR1 & 0xFFF0)  >> 6;
	value = wynik;
		
	tab[ADC_counter] = (short) wynik;
	ADC_counter++;
	if (ADC_counter == tabSize)
	{	
		LED_Off(3);
		ADC_counter = 0;
		NVIC_DisableIRQ(TIMER0_IRQn);
		sendString("New record available\n");
		sendString("Press KEY2 to record again or KEY1 to play recording\n\n");
	}
}

void EINT0_IRQHandler(void)
{
	NVIC_EnableIRQ(TIMER0_IRQn);
	LPC_SC->EXTINT = 1;
	LED_On(3);
}

void EINT1_IRQHandler(void)
{
	NVIC_EnableIRQ(TIMER1_IRQn);
	LPC_SC->EXTINT = 2;
}

void TIMER0_IRQHandler(void)
{
	LPC_ADC->ADCR |= 1 << 24;
	LPC_TIM0->IR = 1;
}

void TIMER1_IRQHandler(void)
{
	LPC_DAC->DACR = tab[DAC_counter] << 6;
	DAC_counter++;
	if (DAC_counter == tabSize)
	{
		DAC_counter = 0;
		NVIC_DisableIRQ(TIMER1_IRQn);
		sendString("Recording played\n");
		sendString("Press KEY2 to record again or KEY1 to play recording\n\n");
	}
	LPC_TIM1->IR = 1;
}
