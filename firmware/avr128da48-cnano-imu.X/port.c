#include "port.h"
#include <avr/io.h>
void PORT_init()
{
	
	
	// PC6 - Output - LED0
	PORTC.DIRSET = PIN6_bm;
	
    // PD6 interrupt
    PORTD.DIRCLR = PIN6_bm;
    PORTD.PIN6CTRL |=  PORT_ISC_RISING_gc;
    
	// PC7 - Input - PW0 - Button
	PORTC.DIRCLR = PIN7_bm;
	// enable pull-up and interrupt
	PORTC.PIN7CTRL |=  PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
}
