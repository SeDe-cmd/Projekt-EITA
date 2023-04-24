/*
 * projekt.c
 *
 * Created: 2023-04-24 10:30:39
 * Author : se8301ed-s
#define RS		 PD4		
#define RW		 PD3
#define EN		 PD5
#define CS1		 PD1
#define CS2		 PD0
#define RST		 PD2
 */
#include <avr/io.h>
#include <util/delay.h>
#define F_CPU 8000000UL
#define RS PD4
#define RW PD3
#define EN PD5
#define CS1 PD1
#define CS2 PD0
#define RST PD2
void GLCD_Command(char Command){		/* GLCD command function */
	PORTB = Command;		/* Copy command on data pin */
	PORTD &= ~(1 << RS);	/* Make RS LOW for command register*/
	PORTD &= ~(1 << RW);	/* Make RW LOW for write operation */
	PORTD |=  (1 << EN);	/* HIGH-LOW transition on Enable */
	_delay_us(5);
	PORTD &= ~(1 << EN);
	_delay_us(5);
	
}
void GLCD_draw(){
	PORTD |=  (1 << RS);	/* Make RS HIGH for data register */
	PORTD &= ~(1 << RW);	/* Make RW LOW for write operation */
	PORTD |=  (1 << EN);	/* HIGH-LOW transition on Enable */
	_delay_us(5);
	PORTD &= ~(1 << EN);
	_delay_us(5);
}

void GLCDinit(){
	DDRB |= 0xFF;
	DDRD |= 0xFF;
	PORTD |= (1 << CS1) | (1 << CS2) | (1 << RST);
	_delay_ms(20);
	_delay_ms(20);
	GLCD_Command(0x3E);		/* Display OFF */
	GLCD_Command(0b01000000);		/* Set Y address (column=0) */
	GLCD_Command(0b10111000);		/* Set x address (page=0) */
	GLCD_Command(0xC0);		/* Set z address (start line=0) */
	GLCD_Command(0x3F);		/* Display ON */
	PORTD |= (0 << CS1) | (0 << CS2) | (1 << RST);
	_delay_us(10);
	
	
}
void testRight(uint8_t* num){
	*num++;
}

void testLeft(uint8_t* num){
	*num++;
}


void right(){
	char a = 0;
}

void left(){
	char b = 0;
}
void InitADC(void){
	ADMUX |= (1<<REFS0) | (0 << ADLAR);
	//ADCSRA|=(1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2); //ENABLE ADC, PRESCALER 128
	ADCSRA |= (1<<ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0) | (1 << ADSC);
}

uint16_t readadc(uint8_t ch){
	        //ANDing to limit input to 7
	ADMUX = (ADMUX & 0b11111100);  //Clear last 3 bits of ADMUX, OR with ch
	ADCSRA |= (1 << ADSC);        //START CONVERSION
	while((ADCSRA & (1 << ADSC)));    //WAIT UNTIL CONVERSION IS COMPLETE
	return(ADC);        //RETURN ADC VALUE
}
void GLCD_setxpos(char x){
	PORTD |=  (1 << RS);	/* Make RS HIGH for data register */
	PORTD &= ~(1 << RW);	/* Make RW LOW for write operation */
	GLCD_Command(0b10111000 + x);
	PORTD |=  (1 << EN);	/* Make HIGH-LOW transition on Enable */
	_delay_us(2);
	PORTD |=  (1 << EN);	/* Make HIGH-LOW transition on Enable */
}

void GLCD_setypos(char y){
	PORTD |=  (1 << RS);	/* Make RS HIGH for data register */
	PORTD &= ~(1 << RW);	/* Make RW LOW for write operation */
	GLCD_Command(0b01000000 + y);
	PORTD |=  (1 << EN);	/* Make HIGH-LOW transition on Enable */
	_delay_us(2);
	PORTD |=  (1 << EN);	/* Make HIGH-LOW transition on Enable */
}

void drawHalf(int xpos, int ypos){
	GLCD_Command(0x3E);		/* Display OFF */
	GLCD_setypos(ypos);		/* Set Y address (column=0) */
	GLCD_setxpos(xpos);
	GLCD_Command(0x3F);		/* Display ON */
	for(int x = 0; x < 5; x++){
		PORTB=(0xFF);
		GLCD_draw();
		_delay_us(10);
	}
	
}
// direction = PINA0 (L/R) eller PINA1 (U/D)
void checkJoystick(){
			uint16_t x,y;
;			x = readadc(0);      //READ ADC VALUE FROM PA.0
			y = readadc(1);      //READ ADC VALUE FROM PA.1
			/* Höger */
			//testLeft(x);
			if(x>611){
				right();
			}
			/* Vänster */
			if(x<590){
				left();
			}
}


int main(void)
{
	
	InitADC();
	GLCDinit();
    /* Replace with your application code */
    while (1) 
    {
		//checkJoystick();
		for(int i = 0 ; i< 255; i++){
			for(int j = 0; j < 255; j++){
				GLCD_setxpos(i);
				GLCD_setypos(j);
				for(int x = 0; x < 8; x++){
					PORTB =(0b11111111);
					GLCD_draw();
					_delay_us(10);
				}
			}
		}
		
		drawHalf(0,0);
		GLCD_Command(0x3F);		/* Display ON */
    }
}

