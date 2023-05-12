#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include "graphics.h"			// Includes all graphics from the graphics file
#include <avr/eeprom.h>			// includes the header file for all eeprom operations
#include <avr/interrupt.h>		// Includes interupt header file
// RS sitter på PD4				// describes physical wire postions, PD4 is a pin on Atmega1284 while R5 is a pin on the GDM display
// RW sitter på PD3
// EN sitter på PD5
// CS1 sitter på PD1
// CS2 sitter på PD0
// RST sitter på PD2
int PlayerPosition = 32; 		// The players position, this is y coordinates and is set to be in the middle of the screen as default
int eggPosX = 0;
int eggPosY = 0;
int score[] = {0,0,0};			// Score is an array because of the way numbers are written on the screen.
int hiScore[] = {0,0,0};
int nextEgg = 1;
uint16_t hiScoreAdress = 64;	// The addres in the eeprom for the highscore of the 2nd difficulty.
char paus = 0;					// If the game is pasued the number is set to one, otherwise 0
char gamestate = 0;				// There are 2 gamestates, if gamestate is one we are not running the main game just running difficulty selection.
char speed = 10;				
void game_over();
// this first section declares all of the bit operations made by the D regestry, all of the pins which we need to use display ram operations in other words.				
void cs1high()
{
	PORTD |= (0b00000010);		// Sets the bit CS1 to 1, this is pin nr 12 on the GDM lcd screen. This bit is set to make the screen aware which half of the screen we are programming
}
void cs1low()
{
	PORTD &= 0b11111101;		// Sets CS1 bit to 0. 
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
void screen1(){				// Highlights screen 1 or the first half of the screen as the primary objective of all following display RAM operations
	cs1high();
	cs2low();
}
void screen2(){
	cs2high();
	cs1low();
}
uint8_t EEPROM_read(uint16_t uiAddress)						//	Using the headerfile, this function reads the data on the address which corrolates with the address given as UI Address
{
	uint8_t value;
	value = eeprom_read_byte ((const uint8_t*)uiAddress);	// Read bytes from the pointer e(adress) of the uiAddress
	return value;											// Send back the value which was stored on uiAddress
}
void EEPROM_write(uint16_t uiAddress, uint8_t ucData)		// Change the data on a certain uiAddress
{
	eeprom_update_byte (( uint8_t *) uiAddress, ucData);	// The update_byte is a fucntion in the eeprom.h file
}
void interrupt_init(){										// Initialise function for interupts
	PORTA |= (1<<PINA2);									// enables the pin to be influenced by the button. 1 is to listen for input
	PCMSK0 |= (1<< PCINT2);									// Gives the exact pin which the interupt will be expected on
	PCICR |= (1<<PCIE0);									// enables interrupts on pcint 0:7 
	sei();
}
void GLCD_ClearAll()										// Clear all pixels on diplay
{
	cs2high();
	cs1high();
	rstHigh();
	for(int i = 0; i < 8; i++){					
		GLCD_Command((0xB8) + i);							// go through each page
		for(int j = 0; j < 64; j++){						// go thorugh each y value
			GLCD_Data(0);									// set the data to 0
		}
	}
	GLCD_Command(0b01111111);								// Resets y address to 0
	GLCD_Command(0b10111111);								// Reset page (x address) to 0
}

void GLCD_Command(char Command){							// Command function, for setting the x, y adress and other operations which do not write to ram
	PORTB = Command;										// Data pins is set to the command
	rsLow();				
	rwLow();
	highLowTransition();
}

void GLCD_Data(char Data){									// GDM write to RAM function											
	PORTB = Data;		
	rsHigh();
	rwLow();
	highLowTransition();
}

void GLCD_draw(){											// Sequence to let the glcd know that the data in data registry is data for pixels, the y and x coord is already set in RAM
	rsHigh();
	rwLow();
	highLowTransition();
}


void GLCD_Init(){
	DDRB = 0xFF;											// Set the data directions for B and D (command and data registry) to 1s to enable them.
	DDRD = 0xFF;
	cs1high();
	cs2high();												// Select both screens
	rstHigh();
	_delay_ms(20);											// Pause to wait for screen to be completly done
	GLCD_Command(0x3E);										// Display off command
	GLCD_Command(0b01000000);								// Set Y address to 0
	GLCD_Command(0b10111000);								// Set x address to 0
	GLCD_Command(0xC0);										// Set z adress to 0
	GLCD_Command(0x3F);										// Display on command
	_delay_us(10);
	
}


void GLCD_setxpos(char x){
	rsLow();
	rwLow();
	GLCD_Command(0b10111000 + x);							// "10111" tells the GLCD that we are setting X coordinate, then the last 000 is for the actual x adress, so this is how x is set. this means x can be 111 at max (7) 
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
	ADMUX|=(1<<REFS0);										// Reference is set to 01 which is: AVCC with external capacitor at AREF pin						
	ADCSRA|=(1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);		// ENABLE ADC, PRESCALER 128
}

uint16_t readadc(int ch){
	ADMUX = (ADMUX & 0xf8)|ch;								// Select chanel by or:ing it with the current admux
	ADCSRA|=(1<<ADSC);										// Start the conversion and set conversion flag to 1
	while((ADCSRA)&(1<<ADSC));								// Until adsc is back to 0 (conversion complete), wait.
	return(ADC);											// Return the resulting value
}

void checkJoystick(){
	uint16_t x =readadc(0);									// READ ADC VALUE FROM PA.0
	if(x>611){												// threshold for standing still is between 590 - 611 so if x goes above or below those there has been a joystick opertaion
		right();											// go right
	}
	if(x< 590){
		left();												// Go left
	}
}


void right(){
	removePlayer();					
	PlayerPosition += 1;
	spawnPlayer();
}

void left(){
	removePlayer();
	PlayerPosition -= 1;
	spawnPlayer();
}
void init(){												// All initialization functions
	GLCD_Init();
	GLCD_ClearAll();
	InitADC();
	interrupt_init();
}
void drawGraphics(int graphic[8][5]){						// Using graphic arrays draw the graphic which is sent in as the array
	int sum =0;
	for(int j = 0; j < 5; j ++){							// for each row
		for(int i = 0 ; i < 8; i++){						// for each column
			if(graphic[i][j] == 1){							// if there is a one on the current position we would want a 1 on the portb data registry,
				//sum += pow(2,i)+1;						// Cs own power function dosnt work.
				int sumation = 1;
				for(int x = 0; x< i; x++){					// Own power function
					sumation = sumation * 2;				// 2^i
				}
				sum += sumation;							// total sum (which is sent to portb for this y coordinate) is the 2^i + 2^i+1 and so on
			}
		}
		
		PORTB = sum;
		GLCD_draw();										// Draw the PORTB for column i
		sum = 0;											// Reset sum
	}
}
void drawNum(int num, int xpos, int ypos){					// Function which uses the graphics for numbers to draw them using a switch statement
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
		drawNum(board[i], ypos, 32 + i*6);						// Draw the numbers which are sent in using the array
	}
}
void updateScore(){
	screen1();
	score[2] = score[2]+1;										// start with the last number (singular)
	if(score[2] == 10){											// check to see if we are at 10 on singular numbers, if that is the case we cant update this coloumn more...
		score[1] = score[1] +1;									// ... so instead we update tens with one ...	
		score[2] = 0;											// ... and reset singular back to 1
	}
	if(score[1] == 10){
		score[0] = score[0] +1;
		score[1] = 0;
	}
	drawScoreboard(score,0);
	screen2();
}

void checkCol(){												// Checks if players position is under the egg at the final tick,
	if(eggPosY + 5 > PlayerPosition && PlayerPosition + 9 > eggPosY){	// if it is..
		//despawnEgg();
		eggPosX = 0;
		eggPosY = rand() % 59;
		updateScore();													//..Get one more point
		} else {														// If its not:
		screen1();
		if(score[0]*100 + score[1] * 10 + score[2] > hiScore[0]*100 + hiScore[1] * 10 + hiScore[2]){//check if the current score we're on is grater than the old highscore
			for(int i = 0; i < 3; i++){
				hiScore[i] = score[i];							// Update highscore 
				score[i] = 0;
			}
			EEPROM_write(hiScoreAdress, hiScore[0]*100 + hiScore[1] * 10 + hiScore[2]); // Update highscore in the eeprom
			drawScoreboard(hiScore,1);							// Redraw the high score
		}
		for(int i = 0; i< 3; i++){
			score[i] = 0;										// reset score
		}
		drawScoreboard(score,0);
		screen2();
		game_over();											// run the game over function as we have lost
	}
}

void spawnPlayer(){
	
	if(PlayerPosition > 63-9){
		PlayerPosition = 63-9;									// Dont move the player if its at the edge just redraw it at its current position
		} else if(PlayerPosition < 0){
		PlayerPosition = 0;										// same for 0
	}
	
	for(int i = PlayerPosition; i < PlayerPosition + 9; i++){	// Draw the player
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
			PORTB = 0b01111110;									// Make the egg alittle bit ovalshaped rather than a square or rectangle
			} else {
			PORTB  =0b11111111;
		}
		GLCD_draw();
	}
}
void despawnEgg(int hEgg){										// remove the egg on the current eggposition, if hegg is one then we have drawn two egg halves and need to remove both of those
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
void moveEgg(){
	if(eggPosX != 7){											// Dont despawn the egg if we are at colision positions
		despawnEgg(0);
	}
	if(eggPosX != 6){
		despawnEgg(1);
	}
	if(nextEgg == 1 || eggPosX ==6){							// if we should spawn the whole egg, then we update the eggposition, or if we are at x = 6	
		eggPosX++;
		nextEgg =0;												// Next time spawn egg halves
	}
	if(eggPosX >= 7){											// If we are at colision position (almost all the way down)
		eggPosX = 7;											// no longer update the positions
		checkCol();												// check if player and egg colides
		} else if(nextEgg == 0 && eggPosX < 6) {				// if we are not at x == 6 and we are supposed to spawn egg halves then we do that at x pos and x pos+1
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
void removePlayer(){
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

void drawletter(int letter[8][5], int xpos, int ypos){			//Similar to draw num but less restricted since there are many more letters than numbers
	GLCD_setxpos(xpos);
	GLCD_setypos(ypos);
	drawGraphics(letter);
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
	int diff = 2;
	int level[] = {l,e,v,e,l};
	for(int i = 0; i < 3; i++){
		drawletter(titel[i],2,1+i*6);
	}
	for(int i = 0; i < 3; i++){
		
		drawletter(egg[i],2,28+i*6);
	}for(int i = 0; i < 5; i++){
	
		drawletter(level[i],3,1+i*6);
	}
	drawNum(1,3,32);
	drawNum(2,3,38);
	drawNum(3,3,44);
	
	GLCD_setxpos(4);
	GLCD_setypos(38);
	drawGraphics(select);
	while (gamestate == 0){											// While gamestate is 0 (there hasnt been a button press) we wait for one
		uint16_t x = readadc(0);
		if(x>611){
			switch(diff){
				case 1:
				diff++;
				GLCD_setxpos(4);
				GLCD_setypos(38);
				drawGraphics(select);
				GLCD_setxpos(4);
				GLCD_setypos(32);
				drawGraphics(blank);
				break;

				case 2:
				diff++;
				GLCD_setxpos(4);
				GLCD_setypos(44);
				drawGraphics(select);
				GLCD_setxpos(4);
				GLCD_setypos(38);
				drawGraphics(blank);
				break;

				case 3:
				diff = 3;
				GLCD_setxpos(4);
				GLCD_setypos(44);
				drawGraphics(select);
				drawGraphics(blank);
				break;
					
			}
		}
		if(x<590){
			switch(diff){
				case 1:
				diff = 1;
				GLCD_setxpos(4);
				GLCD_setypos(32);
				drawGraphics(select);
				break;

				case 2:
				diff--;
				GLCD_setxpos(4);
				GLCD_setypos(32);
				drawGraphics(select);
				GLCD_setxpos(4);
				GLCD_setypos(38);
				drawGraphics(blank);
				break;

				case 3:
				diff--;
				GLCD_setxpos(4);
				GLCD_setypos(38);
				drawGraphics(select);
				GLCD_setxpos(4);
				GLCD_setypos(44);
				drawGraphics(blank);
				break;
			}
		}
	_delay_ms(250);
	}
	if(diff == 1){
		speed = 15;
		hiScoreAdress = 32;						//address for diff 1s highscore
	} else if(diff == 2){
		speed = 10;
		hiScoreAdress = 64;						//adress for diff 2s highscore
	} else {
		speed = 7;
		hiScoreAdress = 128;
	}
}
void press(){
	gamestate = 1;
}
void pause(){
	paus++;
	while(paus == 1){
		sei();
	}
	paus =0;
}
int curr = 0;
int prev = 0;
ISR(PCINT0_vect){	//When there si a change on pcint0_vect (we have only enabled pcint2) 
	curr++;
	if(curr == 8){	// check if we are at 8 then we reset to 2, just to not run out of pauses
		curr = 2;
	}
	if(curr %2 != 0){	// If we are at an uneven amount of changes (only when the button is pressed and not when its released)
		if(gamestate == 0){
			press();
		} else {
			pause();
		}
	}
}

void game_over(){
	GLCD_ClearAll();		// Restart and reset all operations
	gamestate = 0;
	PlayerPosition = 32;
	despawnEgg(0);
	despawnEgg(1);
	eggPosX = 0;
	eggPosY = rand() % 59;
	screen1();
	getHiScoreFromEeporm();
	drawMenu();
	titleScreen();
	getHiScoreFromEeporm();
	drawMenu();
	screen2();
	spawnPlayer();
	slowEgg = 0;
}

int main(void){
	eggPosY = rand() % 59;
	init();
// 	EEPROM_write(32,0); //Functions to reset highscores
// 	
// 	EEPROM_write(64,0);
// 	
// 	EEPROM_write(128,0);
	screen1();
	titleScreen();
	getHiScoreFromEeporm();
	drawMenu();
	screen2();
	spawnEgg();
	spawnPlayer();
	
	while (1){
		slowEgg += 1;
		checkJoystick();
		if(slowEgg == speed){
			moveEgg();
			slowEgg = 0;
		}
		_delay_ms(10);
	}
}
