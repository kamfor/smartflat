#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "RF23.c"

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

void ioinit(void);    
void delay_ms(uint16_t x); 
void delay_us(uint8_t x);
//void sleep();

uint8_t data_array[4];

int main (void)
{
	ioinit();
	
	transmit_data(); //Send one packet when we turn on

	while(1)
	{

		if(PINB & (1<<PB2)){
		PORTA |= (1<<PA7);
 		data_array[0] = 0xFF;
		data_array[1] = 0XFF;
		data_array[2] = 0XFF;
		data_array[3] = 0xFF;
		transmit_data(); 
		delay_ms(200);
		}
		else PORTA &= ~(1<<PA7);
		
		//tx_send_command(0x20, 0x00); //Power down RF
		//cbi(PORTA, PA1); //Go into standby mode
		//sbi(PORTA, PA2); //Deselect chip
		
	}
	
    return(0);
}

void ioinit (void)
{
	//1 = Output, 0 = Input
	DDRA = 0xFF & ~(1<<PA5);
	DDRB = 0xFF & ~(1<<PB2);
	DDRA |=(1<<PA1); //CE
	DDRA |=(1<<PA2); //CSN

	PORTA |=(1<<PA2); 
	//Enable pull-up resistors 
	PORTB |=(1<<PB2);
	
   	 //Init Timer0 for delay_us
  	  TCCR0B = (1<<CS00); //Set Prescaler to No Prescaling (assume we are running at internal 1MHz). CS00=1 

	configure_transmitter();
}

//General short delays
void delay_ms(uint16_t x)
{
	for (; x > 0 ; x--)
	{
		delay_us(250);
		delay_us(250);
		delay_us(250);
		delay_us(250);
	}
}

//General short delays
void delay_us(uint8_t x)
{
	TIFR0 = 0x01; //Clear any interrupt flags on Timer2
   	TCNT0 = 256 - x; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
	while( (TIFR0 & (1<<TOV0)) == 0);
}

/*void Sleep()
{
	MCUCR = (1<<SM1)|(1<<SE); //Setup Power-down mode and enable sleep
	sei(); //Enable interrupts
	ACSR = (1<<ACD); //Turn off Analog Comparator - this removes about 1uA
	PRR = 0x0F; //Reduce all power right before sleep
	asm volatile ("sleep");

		
  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
	
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    MCUCR &= ~(_BV(ISC01) | _BV(ISC00));      //INT0 on low level
    GIMSK |= _BV(INT0);                       //enable INT0
    byte adcsra = ADCSRA;                     //save ADCSRA
    ADCSRA &= ~_BV(ADEN);                     //disable ADC
    cli();                                    //stop interrupts to ensure the BOD timed sequence executes as required
    byte mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);  //turn off the brown-out detector
    byte mcucr2 = mcucr1 & ~_BV(BODSE);
    MCUCR = mcucr1;
    MCUCR = mcucr2;
    sei();                                    //ensure interrupts enabled so we can wake up again
    sleep_cpu();                              //go to sleep
    sleep_disable();                          //wake up here
    ADCSRA = adcsra;                          //restore ADCSRA
}

//external interrupt 0 wakes the MCU
ISR(INT0_vect)
{
    GIMSK = 0;                     //disable external interrupts (only need one to wake up)
}*/

