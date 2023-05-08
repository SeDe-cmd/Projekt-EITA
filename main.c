#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "graphics.h"
#include <avr/eeprom.h>
// RS sitter på PD4
// RW sitter på PD3
// EN sitter på PD5
// CS1 sitter på PD1
// CS2 sitter på PD0
// RST sitter på PD2

int PlayerPosition = 32;
int eggPosX = 0;
int eggPosY = 0;
int score[] = {0,0,0};
int hiScore[] = {0,0,0};
int diff = 1;
uint16_t hiScoreAdress = 64;

void cs1high()
{
	PORTD |= (0b00000010);
}

void cs1low()
{
	PORTD &= 0b11111101;
}

void cs2high()
{
	PORTD |= 0b00000001;
}

void cs2low()
{
	PORTD &= 0b11111110;
}
void rwHigh(){
	PORTD |= (0b00001000);
}
void rwLow(){
	PORTD &= 0b11110111;
}
void rsHigh(){
	PORTD |= (0b00010000);
}

void rsLow(){
	PORTD &= 0b11101111;
}
void rstHigh(){
	PORTD |= (0b00000100);
}
void rstLow(){
	PORTD &= 0b11111011;
}
void enHigh(){
	PORTD |= (0b00100000);
}
void enLow(){
	PORTD &= 0b11011111;
}
void highLowTransition(){
	enHigh();
	_delay_us(5);
	enLow();
	_delay_us(5);
}
void screen1(){
	cs1high();
	cs2low();
}
void screen2(){
	cs2high();
	cs1low();
}
uint8_t EEPROM_read(uint16_t uiAddress)
{		
	uint8_t value;
	value = eeprom_read_byte ((const uint8_t*)uiAddress);
	return value;
}
void EEPROM_write(uint16_t uiAddress, uint8_t ucData)
{
	eeprom_update_byte (( uint8_t *) uiAddress, ucData);
}
void GLCD_ClearAll()			/* GLCD all display clear function */
{
	cs2high();
	cs1high();
	rstHigh();
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
	rsLow();
	rwLow();
	highLowTransition();
}

void GLCD_Data(char Data){		/* GLCD data function */
	PORTB = Data;		/* Copy data on data pin */
	rsHigh();
	rwLow();
	highLowTransition();
}

void GLCD_draw(){
	rsHigh();
	rwLow();
	highLowTransition();
}


void GLCD_Init(){
	/* GLCD initialize function */
	DDRB = 0xFF;
	DDRD = 0xFF;
	/* Select both left & right half of display & Keep reset pin high */
	//PORTD |= (1 << CS1) | (1 << CS2) | (1 << RST);
	cs1high();
	cs2high();
	rstHigh();
	_delay_ms(20);
	GLCD_Command(0x3E);		/* Display OFF */
	GLCD_Command(0b01000000);		/* Set Y address (column=0) */
	GLCD_Command(0b10111000);		/* Set x address (page=0) */
	GLCD_Command(0xC0);		/* Set z address (start line=0) */
	GLCD_Command(0x3F);		/* Display ON */
	_delay_us(10);
	
}


void GLCD_setxpos(char x){
	rsLow();
	rwLow();
	GLCD_Command(0b10111000 + x);
	enHigh();
	_delay_us(2);
}

void GLCD_setypos(char y){
	rsLow();
	rwLow();
	GLCD_Command(0b01000000 + y);
	enHigh();
	_delay_us(2);
}



void InitADC(void){
	ADMUX|=(1<<REFS0);
	ADCSRA|=(1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2); //ENABLE ADC, PRESCALER 128
}

uint16_t readadc(int ch){
	ADMUX = (ADMUX & 0xf8)|ch;  //Clear last 3 bits of ADMUX, OR with ch
	ADCSRA|=(1<<ADSC);        //START CONVERSION
	while((ADCSRA)&(1<<ADSC));    //WAIT UNTIL CONVERSION IS COMPLETE
	return(ADC);        //RETURN ADC VALUE
}

void checkJoystick(){
	uint16_t x =readadc(0);      //READ ADC VALUE FROM PA.0
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
void drawNum(int num, int xpos, int ypos){
	GLCD_setxpos(xpos);
	GLCD_setypos(ypos);
	switch(num)
	{
		case 0:
		drawGraphics(zero);
		break;
		case 1:
		drawGraphics(one);
		break;
		case 2:
		drawGraphics(two);
		break;
		case 3:
		drawGraphics(three);
		break;
		case 4:
		drawGraphics(four);
		break;
		case 5:
		drawGraphics(five);
		break;
		case 6:
		drawGraphics(six);
		break;
		case 7:
		drawGraphics(seven);
		break;
		case 8:
		drawGraphics(eight);
		break;
		case 9:
		drawGraphics(nine);
		break;
		default:
		drawGraphics(zero);
	}
}
void drawScoreboard(int board[],int ypos){
	for(int i = 0; i < 3; i++){
		drawNum(board[i], ypos, 32 + i*6);
	}
}
void updateScore(){
	screen1();
	score[2] = score[2]+1;
	if(score[2] == 10){
		score[1] = score[1] +1;
		score[2] = 0;
	}
	if(score[1] == 10){
		score[0] = score[0] +1;
		score[1] = 0;
	}
	drawScoreboard(score,0);
	screen2();
}
void checkCol(){
	if(eggPosY + 5 > PlayerPosition && PlayerPosition + 9 > eggPosY){
		//despawnEgg();
		eggPosX = 0;
		eggPosY = rand() % 59;
		updateScore();
		} else {
		spawnEgg();
		screen1();
		if(score[0]*100 + score[1] * 10 + score[2] > hiScore[0]*100 + hiScore[1] * 10 + hiScore[2]){
			for(int i = 0; i < 3; i++){
				hiScore[i] = score[i];
				score[i] = 0;
			}
			EEPROM_write(hiScoreAdress, hiScore[0]*100 + hiScore[1] * 10 + hiScore[2]);
			drawScoreboard(hiScore,1);
		}
		for(int i = 0; i< 3; i++){
			score[i] = 0;
		}
		drawScoreboard(score,0);
		screen2();
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
void despawnEgg(int hEgg){
	for(int i = eggPosY; i < eggPosY + 5; i++){
		GLCD_setxpos(eggPosX);
		GLCD_setypos(i);
		PORTB = 0;
		GLCD_draw();
	}
	if(hEgg == 1){
		for(int i = eggPosY; i < eggPosY + 5; i++){
			GLCD_setxpos(eggPosX+1);
			GLCD_setypos(i);
			PORTB = 0;
			GLCD_draw();
		}
	}
}
int nextEgg = 1;
void moveEgg(){
	if(eggPosX != 7){
		despawnEgg(0);
	}
	if(eggPosX != 6){
		despawnEgg(1);
	}
	if(nextEgg == 1 || eggPosX ==6){
		eggPosX++;
		nextEgg =0;
	}
	if(eggPosX >= 7){
		eggPosX = 7;
		checkCol();
		} else if(nextEgg == 0 && eggPosX < 6) {
		GLCD_setxpos(eggPosX);
		GLCD_setypos(eggPosY);
		drawGraphics(topEgg);
		GLCD_setxpos(eggPosX + 1);
		GLCD_setypos(eggPosY);
		drawGraphics(bottomEgg);
		nextEgg = 1;
		} else {
		spawnEgg();
	}
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

void getHiScoreFromEeporm(){
	uint8_t prevHiScore = EEPROM_read(hiScoreAdress);
	for(int i = 2 ; i >= 0 ; i--){
		hiScore[i] = prevHiScore %10;
		prevHiScore = prevHiScore /10;
	}
}

void drawMenu(){
	for(int i = 0; i < 8; i++){
		GLCD_setxpos(i);
		GLCD_setypos(0);
		PORTB = 0xff;
		GLCD_draw();
	}
	
	int word[] = {s,c,o,r,e};
	for(int i = 0; i < 5; i++){
		drawletter(word[i],0,i*6 + 1);
	}
	for(int i = 0; i < 3; i++){
		drawNum(score[i], 0, 32 + i*6);
	}
	for(int i = 0; i < 3; i++){
		drawNum(hiScore[i], 1, 32 + i*6);
	}
}

void titleScreen(){
	int titel[] = {t,h,e};
	int egg[] = {e,g,g};
	int level[] = {l,e,v,e,l};

	drawNum(1,3,32);
	drawNum(2,3,38);
	drawNum(3,3,44);	

	while difficultyPick(1){
		uint16_t x = readadc(0);

		if(x>611){
			switch(diff){
				case 1:
				diff++;
				GLCD_setxpos(4);
				GLCD_setypos(38);
				drawGraphics(select);
				break;

				case 2:
				diff++
				GLCD_setxpos(4);
				GLCD_setypos(44);
				drawGraphics(select);
				break;

				case 3:
				diff = 1;
				GLCD_setxpos(4);
				GLCD_setypos(32);
				drawGraphics(select);
				break;
				
			}
		}
		if(x<590){
			switch(diff){
				case 1:
				diff = 3;
				GLCD_setxpos(4);
				GLCD_setypos(44);
				drawGraphics(select);
				break;

				case 2:
				diff--;
				GLCD_setxpos(4);
				GLCD_setypos(32);
				drawGraphics(select);
				break;

				case 3:
				diff--;
				GLCD_setxpos(4);
				GLCD_setypos(38);
				drawGraphics(select);
				break;
			}
		}
	}
}





int main(void){
	eggPosY = rand() % 59;
	init();
	//EEPROM_write(hiScoreAdress,0);
	getHiScoreFromEeporm();
	screen1();
	drawMenu();
	screen2();
	spawnEgg();
	spawnPlayer();
	
	while (1){
		slowEgg += 1;
		checkJoystick();
		if(slowEgg == 10){
			moveEgg();
			slowEgg = 0;
		}
		_delay_ms(10);
	}
}
