/*
 * mk_lab_8.c
 *
 * Created: 10.03.2021 20:35:28
 * Author : Leon
 */ 
#define F_CPU 16000000UL // System clock

#include <util/delay.h>
#include <avr/interrupt.h>

volatile uint8_t stage = 0;
volatile uint16_t ms;
volatile uint16_t varible_delay;

static inline void initTimer1(void) {
	TCCR1A = (0<<WGM11) | (0<<WGM10) | (0<<COM1A1) | (0<<COM1A0);
	TCCR1B = (0<<WGM13) | (1<<WGM12) | (0<<CS12) | (0<<CS11) | (1<<CS10) | (1<<ICES1);
	// WGM:		Mode 15, CTC, TOP = OCR1A;
	// COM:		Normal port operation
	// -> OCR1A determins TOP.
	// CS: f = 16Mhz -> T = 1ms -> N = T*f = 16k
	// ICS1: Input capture edge, true -> positive
	OCR1A = 16000; // TOP verdi,
}



static inline void initICI(void){
	TIMSK1 = (1<<ICIE1);
	TIMSK1 &= ~(1<<TOIE1);
	PORTB |= (1<<PORTB4) | (1<<PORTB0); // pullup
}

void print_1000(volatile uint16_t t){
	transmitByte('0' + ((t/ 1000) % 10));               // Thousands
	transmitByte('0' + ((t/ 100) % 10));                 // Hundreds
	transmitByte('0' + ((t/ 10) % 10));                      // Tens
	transmitByte('0' + (t % 10));
}

int main(void)
{
	//Set AVcc as ref and set input as AD1
		// ADMUX
			//	[7-6]:	REFS[1-0], Voltage referance selection
				//	01 AVcc with external capacitor at AREF pin... (Lacking capacitor)
			//	[6]:	ADLAR, ADC left adjust result
			//	[3-0]:	MUX, determines sigle ended input pin
	ADMUX = (1<<REFS0)| (0x00) ;    // ADMUX = 0b 0110 0001
	
	//Enable ADC, prescale, single conversion and start conversion
		// ADCSRA "=" ADEN, ADSC, ADIF, ADIE, ADPS2, ADPS1, ADPS0
			// ADEN: ADC enable. If set then ADC is on
			// ADSC: In single conversion mode, set this bit to start conversion. ADSC will read one as long as a conversion is in progress
			// ADPS[2-0]: ADC prescaler select bits. Divsion factor between system clock and input clock to the ADC
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);    //0b 1000 0111 (16MHz/128 = 125kHz)
	
	
	
	TCCR0A = (1<<WGM01) | (1<<WGM00); // | (1<<COM0A1);
	TCCR0B = (1<<CS02);
		// (1<<WGM01) | (1<<WGM00): Mode 3, fast PWM, update at bottom, TOP = 0xFF  
		// (1<<COM0A1) | (0<<COM0A0): Clear on compare match, set at Bottom -> higher OCR0A -> higher HIGH time.
		// f_OC0A = 16*10^6 / (N * 256), We want f_OC0A > 100 => N < 16*10^6/(100*256), N < 625, f_OC0A(256) = 244,14
		// -> (1<<CS02) -> N = 256
	DDRD = (1<<PIND6) | (1<<PIND5); // Sets OC0A/PORTD6 as output
    
	// initUSART();
	initICI();
	initTimer1();
	initUSART();	
	stage = 0;
	sei();
	TIFR1 = (1<<ICF1) | (1<<OCF1A);
	DDRB |= (1 << PINB2) | (1 << PINB3) | (1 << PINB4);								/* set pin for output */
	
	
	while (1) 
    {
		ADCSRA |= (1<<ADSC);							// Setting ADSC => Starting conversion
		loop_until_bit_is_clear(ADCSRA, ADSC);			//Wait til ADSC is clear
		OCR0A = ADC/4;
    }
}

ISR(TIMER1_CAPT_vect){
	TIFR1 = (1<<OCF1A);
	switch(stage){
		// Start spill
		case 0:
			loop_until_bit_is_set(PINB, PINB0);
			
			// Gå videre til neste stadie
			stage = 1;
		
			// Config Interrupt
			TIMSK1 &= ~(1<<ICIE1);
			TIMSK1  =  (1<<OCIE1A);
		
			TCCR0A &= ~(1<<COM0A1);
			PORTB &= ~(1<<PORTB2);
			PORTB &= ~(1<<PORTB3);
			PORTB &= ~(1<<PORTB4);
		
			varible_delay = (TCNT1>>2);
			printString("START \r");
			ms = 0;
		break;
		
		// CHEATER!!!
		case 2:
			loop_until_bit_is_set(PINB, PINB0);
			
			stage = 0;
			TCCR0A &= ~(1<<COM0A1);
			PORTB = (1<<PORTB0) | (1<<PORTB2);
			printString("\t CHEATER \r");
			ms = 0;
		break;
		
		// Button pressed, winner
		case 3:
			stage = 0;
			TIMSK1 &= ~(1<<OCIE1A);
			PORTB |=  (1<<PORTB3);
			
			// Knappetrykk kraft
			volatile uint16_t pot_value = ADC;
			ADMUX = (1<<REFS0)| (0x01);
			ADCSRA |= (1<<ADSC);
			loop_until_bit_is_clear(ADCSRA, ADSC);
			volatile uint16_t pressure_value = ADC;
			ADMUX = (1<<REFS0)| (0x00);
			
			// print results
			printString("\t Winner: \r");
				printString("\t \t reactiontime in ms: "); print_1000(ms); printString("\r");
				printString("\t \t pressure on button: "); print_1000(pressure_value); printString("\r");
				printString("\t \t light level: "); print_1000(pot_value); printString("\r \r" );
			
			loop_until_bit_is_set(PINB, PINB0);
			
			ms = 0;
		break;
	}
}

ISR(TIMER1_COMPA_vect){	
	switch(stage){
		case 1:
			if(ms >= 1000){
				ms = 0;
				stage = 2;
				TCCR0A |= (1<<COM0A1);
				TIMSK1 |= (1<<ICIE1);
			}
			else{ms++;}
		break;
		
		case 2:
			if((ms >= varible_delay) & (ms >=500)){
				ms = 0;
				stage = 3;
				TCCR0A &= ~(1<<COM0A1);
			}
			else{ms++;}
		break;
		
		case 3: // Too slow
			if(ms >= 1000){
				ms = 0;
				stage = 0;
				TIMSK1 &= ~(1<<OCIE1A);
				PORTB |= (1<<PORTB2);
				printString("\t Too slow \r");
			}
			else{ms++;}
		break;
	}
}
