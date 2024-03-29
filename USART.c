/*
 * USART.c
 *
 * Created: 05.02.2021 12:33:41
 *  Author: Leon
 */

#define F_CPU 16000000UL // System clock
#define USART_BAUDRATE 9600 // desired baud rate
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1) // UBRR value

#include <avr/io.h>
#include "USART.h"
#include <util/setbaud.h>

void initUSART(void) { /* requires BAUD */
	UBRR0H = UBRRH_VALUE; /* defined in setbaud.h */
	UBRR0L = UBRRL_VALUE;
	#if USE_2X
	UCSR0A |= (1 << U2X0);
	#else
	UCSR0A &= ~(1 << U2X0);
	#endif
	/* Enable USART transmitter/receiver */
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); /* 8 data bits, 1 stop bit */
}
void transmitByte(uint8_t data) {
	/* Wait for empty transmit buffer */
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = data; /* send data */
}
uint8_t receiveByte(void) {
	loop_until_bit_is_set(UCSR0A, RXC0); /* Wait for incoming data */
	return UDR0; /* return register value */
}
// Example of a useful printing command
void printString(const char myString[]) {
	uint8_t i = 0;
	while (myString[i]) {
		transmitByte(myString[i]);
		i++;
	}
}