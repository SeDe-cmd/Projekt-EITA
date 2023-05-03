#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "graphics.h"
#define RS PD4		/* Define control pins */
#define RW PD3
#define EN PD5
#define CS1 PD1
#define CS2 PD0
#define RST PD2

int PlayerPosition = 32;
int eggPosX = 0;
int eggPosY = 0;

void GLCD_ClearAll()			/* GLCD all display clear function */
{
	PORTD |= (1 << CS1) | (1 << CS2) | (1 << RST);
	for(int i = 0; i < 8; i++){
		GLCD_Command((0xB8) + i);/* Increment page */
		for(int j = 0; j < 64; j++){
			GLCD_Data(0);	/* Write zeros to all 64 column */
		}
	}
	GLCD_Command(0b01111111);		/* Set Y address (column=0) */
	GLCD_Command(0b10111111);		/* Set x address (page=0) */
}

void GLCD_Command(char Command){		/* GLCD command function */
	PORTB = Command;		/* Copy command on data pin */
	PORTD &= ~(1 << RS);	/* Make RS LOW for command register*/
	PORTD &= ~(1 << RW);	/* Make RW LOW for write operation */
	PORTD |=  (1 << EN);	/* HIGH-LOW transition on Enable */
	_delay_us(5);
	PORTD &= ~(1 << EN);
	_delay_us(5);
}

void GLCD_Data(char Data){		/* GLCD data function */
	PORTB = Data;		/* Copy data on data pin */
	PORTD |=  (1 << RS);	/* Make RS HIGH for data register */
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


void GLCD_Init(){
	/* GLCD initialize function */
	DDRB = 0xFF;
	DDRD = 0xFF;
	/* Select both left & right half of display & Keep reset pin high */
	PORTD |= (1 << CS1) | (1 << CS2) | (1 << RST);
	_delay_ms(20);
	GLCD_Command(0x3E);		/* Display OFF */
	GLCD_Command(0b01000000);		/* Set Y address (column=0) */
	GLCD_Command(0b10111000);		/* Set x address (page=0) */
	GLCD_Command(0xC0);		/* Set z address (start line=0) */
	GLCD_Command(0x3F);		/* Display ON */
	PORTD |= (0 << CS1) | (0 << CS2) | (1 << RST);
	_delay_us(10);
	
}


void GLCD_setxpos(char x){
	PORTD &= ~(1 << RS);	/* Make RS HIGH for data register */
	PORTD &= ~(1 << RW);	/* Make RW LOW for write operation */
	GLCD_Command(0b10111000 + x);
	PORTD |=  (1 << EN);	/* Make HIGH-LOW transition on Enable */
	_delay_us(2);
	PORTD |=  (1 << EN);	/* Make HIGH-LOW transition on Enable */
}

void GLCD_setypos(char y){
	PORTD &= ~(1 << RS);	/* Make RS HIGH for data register */
	PORTD &= ~(1 << RW);	/* Make RW LOW for write operation */
	GLCD_Command(0b01000000 + y);
	PORTD |=  (1 << EN);	/* Make HIGH-LOW transition on Enable */
	_delay_us(2);
	PORTD |=  (1 << EN);	/* Make HIGH-LOW transition on Enable */
}



void cs2high()
{
	PORTD |= (0b00000010);
}

void cs2low()
{
	PORTD &= 0b11111101;
}

void cs1high()
{
	PORTD |= 0b00000001;
}

void cs1low()
{
	PORTD &= 0b11111110;
}

void InitADC(void){
	ADMUX|=(1<<REFS0);
	ADCSRA|=(1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2); //ENABLE ADC, PRESCALER 128
}

uint16_t readadc(){
	ADMUX = (ADMUX & 0xf8)|0;  //Clear last 3 bits of ADMUX, OR with ch
	ADCSRA|=(1<<ADSC);        //START CONVERSION
	while((ADCSRA)&(1<<ADSC));    //WAIT UNTIL CONVERSION IS COMPLETE
	return(ADC);        //RETURN ADC VALUE
}

void checkJoystick(){
	uint16_t x =readadc();      //READ ADC VALUE FROM PA.0
	if(x>611){
		right();
	}
	if(x< 590){
		left();
	}
}


void right(){
	movePlayer();
	PlayerPosition += 1;
	spawnPlayer();
}

void left(){
	movePlayer();
	PlayerPosition -= 1;
	spawnPlayer();
}
void init(){
	GLCD_Init();
	GLCD_ClearAll();
	InitADC();
}
void drawGraphics(int graphic[8][5]){
	int sum =0;
	for(int j = 0; j < 5; j ++){
		for(int i = 0 ; i < 8; i++){
			if(graphic[i][j] == 1){
				//sum += pow(2,i)+1;
				int sumation = 1;
				for(int x = 0; x< i; x++){
					sumation = sumation * 2;
				}
				sum += sumation;
			}
		}
		
		PORTB = sum;
		GLCD_draw();
		sum = 0;
	}
}
void updateScore(){
	cs2high();
	cs1low();
// 	for(int i = 0; i < 8; i++){
// 		GLCD_setxpos(i);
// 		GLCD_setypos(32);
// 		PORTB = 0xff;
// 		GLCD_draw();
// 	}
	cs2low();
	cs1high();
}
void checkCol(){
	if(eggPosY + 5 > PlayerPosition && PlayerPosition + 9 > eggPosY){
		//despawnEgg();
		eggPosX = 0;
		eggPosY = rand() % 59;
		updateScore();
	} else {
		spawnEgg();
	}
}

void spawnPlayer(){
	
	if(PlayerPosition > 63-9){
		PlayerPosition = 63-9;
	} else if(PlayerPosition < 0){
		PlayerPosition = 0;
	}
	
		for(int i = PlayerPosition; i < PlayerPosition + 9; i++){
			GLCD_setxpos(7);
			GLCD_setypos(i);
			PORTB = 0xff;
			GLCD_draw();
		}
}
void spawnEgg(){
	for(int i = eggPosY; i < eggPosY + 5; i++){
		GLCD_setxpos(eggPosX);
		GLCD_setypos(i);
		if(i - eggPosY == 0 || i == eggPosY + 4){
			PORTB = 0b01111110;
		} else {
			PORTB  =0b11111111;
		}
		GLCD_draw();
	}
}
void despawnEgg(){
	for(int i = eggPosY; i < eggPosY + 5; i++){
		GLCD_setxpos(eggPosX);
		GLCD_setypos(i);
		PORTB = 0;
		GLCD_draw();
	}
}
int nextEgg = 0;
void moveEgg(){
	if(eggPosX != 7){
		despawnEgg();
	}
	if(nextEgg == 1){
		eggPosX++;
	} else {
		
	}
	if(eggPosX >= 7){
		eggPosX = 7;
		checkCol();
	}else {
		
		spawnEgg();
	}
	nextEgg = 1;
}
void movePlayer(){
	for(int i = PlayerPosition; i < PlayerPosition + 9; i++){
		GLCD_setxpos(7);
		GLCD_setypos(i);
		PORTB = 0;
		GLCD_draw();
	}
}
int slowEgg =0;
void drawMenu(){
	for(int i = 0; i < 8; i++){
		GLCD_setxpos(i);
		GLCD_setypos(0);
		PORTB = 0xff;
		GLCD_draw();
	}
	GLCD_setxpos(4);
	GLCD_setypos(32);
	drawGraphics(topEgg);
	GLCD_setypos(32);
	GLCD_setxpos(5);
	drawGraphics(bottomEgg);
		 
	
}
int main(void){
	eggPosY = rand() % 59;
	
	init();
	cs2high();
	cs1low();
	drawMenu();
	cs1high();
	cs2low();
	spawnEgg();
	spawnPlayer();
	
	while (1){
		slowEgg += 1;
		checkJoystick();
		if(slowEgg == 10){
			moveEgg();
			slowEgg = 0;
		}
		_delay_ms(20);
	}
}
