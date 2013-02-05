#define F_CPU 8000000UL  // 8 MHz
#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define output_low(port,pin) port &= ~(1<<pin)
#define output_high(port,pin) port |= (1<<pin)
#define set_input(portdir,pin) portdir &= ~(1<<pin)
#define set_output(portdir,pin) portdir |= (1<<pin)

//SPI
#define CTRL_PORT   DDRB 
#define DATA_PORT   PORTB 
#define CLK_PIN     PB7 
#define DI_PIN      PB5 
#define DO_PIN      PB6 

#define HOST_PIN    PD1
#define DEBUG_PIN   PD0

//registers
#define REG_M1PW   0x01  //motor 1, pulse width
#define REG_M2PW   0x02  //motor 2, pulse width
#define REG_M1DR   0x03  //motor 1, direction
#define REG_M2DR   0x04  //motor 2, direction

//op codes
#define REGW       0x01
#define CLRH       0x02
#define RIFEQ      0x03


/*

Since the tiny (or any AVR) is limited on RAM, instead of a full blown general purpose bytecode, do the following:

Add the following commands:

RIFL x - raise the host pin if pin x is low
RIFH x - raise the host pin if pin x is high
RILT x, y - raise the host pin if analog pin x is less than y
RIGT x, y - raise the host pin if analog pin x is greater than y
REGWIFL x, y, z - writes y to register x if pin z is low
REGWIFH x, y, z - writes y to register x if pin z is high
REGWIFLT x, y, z, a - writes y to register x if pin z is less than a
REGWIFGT x, y, z, a - writes y to register x if pin z is greater than a

In fact, refactor the above to treat pins as registers to make it more general purpose.

Have the main run-loop of the app just poll various pins and check various registers based on
a table that the "processCommand" function esthablishes via direction of the host.

The refactored commands might look like:

RIFEQ x, y - raise the host pin if register x is equal to y (binary pins will have their registers either be 0x00 or 0x01)
RIFLT x, y - raise the host pin if register x is less than y
RIFGT x, y - raise the host pin if register x is greater than y
REGWIFEQ w, x, y, z - writes x to register w if register y is equal to z
REGWIFLT w, x, y, z - writes x to register w if register y is less than z
REGWIFGT w, x, y, z - writes x to register w if register y is greater than z

This will mean we'll need "registers" for each GPIO & analog pin.

Later we can add timers as registers too.
  
*/


#define MAX_COMMAND_LEN 10
char command_buffer[MAX_COMMAND_LEN] = {0};
int command_length = 0;


#define TABLE_SIZE 10
char table[TABLE_SIZE][MAX_COMMAND_LEN];
int table_size = 0;


void clearBuffer(){
    command_length = 1; //since we're ignoring the first byte, just skip it and assume 0
    for(int i = 0; i != MAX_COMMAND_LEN; i++) command_buffer[i] = 0x00; //clear buffer
}

void processCommandBuffer(){
  if(command_length < 2) return;    //no command is this short
  
  int op_code = command_buffer[1];  //first byte of op-code isn't implemented yet

  if(op_code == REGW){
    /*
    * REGW - Register Write - Writes byte 4 to the register specified in byte 3
    * Command length: 4 bytes
    */
    if(command_length >= 4){
      int reg = command_buffer[2];
      int byte = command_buffer[3];
      if(reg == 0x00){              //0x00 is undocumented, used for debugging. It echoes the received bit on the host pin.
        if(byte > 0){
          output_high(PORTD, HOST_PIN);
        }else{
          output_low(PORTD, HOST_PIN);
        }
      }else if(reg == REG_M1PW){
        OCR0A = byte;
      }else if(reg == REG_M2PW){
        OCR1A = byte;
      }else if(reg == REG_M1DR){
        if(byte > 0){
          output_high(PORTA, PA1);
        }else{
          output_low(PORTA, PA1);
        }
      }else if(reg == REG_M2DR){
        if(byte > 0){
          output_high(PORTA, PA0);
        }else{
          output_low(PORTA, PA0);
        }
      }
      clearBuffer();
    }//else, wait for more bytes...
  }else if(op_code == CLRH){
    /*
    * CLRH - Clear Host Pin - Resets the host pin. E.g., it sets it low.
    * Command length: 2 bytes
    */
    output_low(PORTD, HOST_PIN);
    clearBuffer();
  }else if(op_code == RIFEQ){
    if(command_length >= 4){
      int tableEntry = table_size++;
      //TODO: check for overflow
      for(int i = 0; i != command_length; i++){
        table[tableEntry][i] = command_buffer[i]; 
      }
      clearBuffer();
    }
  }else{ //unknown op code
    clearBuffer();
  }
}

ISR(USI_OVERFLOW_vect){
  int byte = USIDR / 2;
  USISR = (1<<USIOIF);
  USIDR = byte;
/*
  for(int i = 0; i != byte; i++){
    output_high(PORTD, DEBUG_PIN);
    _delay_ms(1000);
    output_low(PORTD, DEBUG_PIN);
    _delay_ms(1000);
  }
  */
  if(command_length < MAX_COMMAND_LEN){ //do not allow overflow, no command should be this long
    command_buffer[command_length++] = byte;
    processCommandBuffer();
  }else{
    clearBuffer();
  } 
}

int main(void){
  DDRB |= (1<<PB2);               // make OC0A an output
  TCCR0B = 0;                     // stop timer 0
  TCCR0A = (1<<WGM00)|(1<<WGM01); // select fast pwm mode 3
  TCCR0A |= (1<<COM0A1);          //Clear OC0A on Compare Match when up-counting. Set OC0A on Compare Match when down-counting.
  OCR0A = 0x00;                   //duty cycle
  TCCR0B |= (1<<CS00);            // no prescaling

  DDRB |= (1<<PB3);
  TCCR1B = 0;
  TCCR1A = (1<<WGM00);
  TCCR1A |= (1<<COM1A1);
  OCR1A = 0x00;
  TCCR1B |= (1<<CS00);
  
  set_output(DDRD, HOST_PIN);
  output_low(PORTD, HOST_PIN);

  //Setup SPI/USI
  CTRL_PORT |= _BV(DO_PIN);
  USICR = _BV(USIOIE) | _BV(USIWM0) | _BV(USICS0) | _BV(USICS1);
  USISR = (1<<USIOIF);

  clearBuffer();
  sei();
  
  set_output(DDRA, PA1);
  output_low(PORTA, PA1);
  set_output(DDRA, PA0);
  output_low(PORTA, PA0);
  
  set_output(DDRD, DEBUG_PIN);
  output_high(PORTD, DEBUG_PIN);
  _delay_ms(1000);
  output_low(PORTD, DEBUG_PIN);  
  while(1);
}
