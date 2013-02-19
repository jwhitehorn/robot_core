#define F_CPU 8000000UL  // 8 MHz
#include <stdbool.h>
#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "spi.h"
#include "helpers.h"

//SPI
#define CTRL_PORT   DDRB 
#define DATA_PORT   PORTB 
#define CLK_PIN     PB7 
#define DI_PIN      PB5 
#define DO_PIN      PB6 

//registers
#define REG_M1PW   0x01  //motor 1, pulse width
#define REG_M2PW   0x02  //motor 2, pulse width
#define REG_M1DR   0x03  //motor 1, direction
#define REG_M2DR   0x04  //motor 2, direction
#define ADC1       0x05  //ADC 1
#define GPIO1      0x06  //GPIO 1

//op codes
#define REGW       0x01
#define CLRH       0x02
#define RIFEQ      0x03
#define RIFLT      0x04
#define MODE       0x05


/*

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


#define MAX_TABLE_SIZE 10
char table[MAX_TABLE_SIZE][MAX_COMMAND_LEN];
int table_size = 0;


void clearBuffer(){
    command_length = 1; //since we're ignoring the first byte, just skip it and assume 0
    for(int i = 0; i != MAX_COMMAND_LEN; i++) command_buffer[i] = 0x00; //clear buffer
}

void storeCommandInTable(char *command, int length){
  if(table_size < MAX_TABLE_SIZE){
    int table_entry = table_size++;
    for(int i = 0; i != length || MAX_COMMAND_LEN; i++){
      table[table_entry][i] = command[i];
    }
  }
}

int compareRegister(int reg, int value){
  int valueRead = 0;
  if(reg == ADC1){
    uint16_t adc = ReadADC(0);
    valueRead = (adc >> 2);
  }else if(reg == GPIO1){
    valueRead = readPin(GPIO1_PIN);
  }else{
    return -2;
  } 
  if(valueRead < value){
    return -1;
  }else if(valueRead == value){
    return 0;
  }
  return 1;
}

bool processCommand(char *command, int length, bool store){
  if(length < 2) return false;    //no command is this short
  
  int op_code = command[1];  //first byte of op-code isn't implemented yet

  if(op_code == REGW){
    /*
    * REGW - Register Write - Writes byte 4 to the register specified in byte 3
    * Command length: 4 bytes
    */
    if(length >= 4){
      int reg = command[2];
      int byte = command[3];
      if(reg == 0x00){              //0x00 is undocumented, used for debugging. It echoes the received bit on the host pin.
        if(byte > 0){
          outputHigh(HOST_PIN);
        }else{
          outputLow(HOST_PIN);
        }
      }else if(reg == REG_M1PW){
        OCR0A = byte;
      }else if(reg == REG_M2PW){
        OCR1A = byte;
      }else if(reg == REG_M1DR){
        if(byte > 0){
          outputHigh(M1DR_PIN);
        }else{
          outputLow(M1DR_PIN);
        }
      }else if(reg == REG_M2DR){
        if(byte > 0){
          outputHigh(M2DR_PIN);
        }else{
          outputLow(M2DR_PIN);
        }
      }
      //clearBuffer();
      return true;
    }//else, wait for more bytes...
  }else if(op_code == CLRH){
    /*
    * CLRH - Clear Host Pin - Resets the host pin. E.g., it sets it low.
    * Command length: 2 bytes
    */
    outputLow(HOST_PIN);
    //clearBuffer();
    return true;
  }else if(op_code == RIFEQ){
    if(length >= 4){
      if(store){
        storeCommandInTable(command, length);
      }else{
//        outputHigh(HOST_PIN);
        int reg = command[2];
        int value = command[3];
        int result = compareRegister(reg, value);
        if(result == 0){
          outputHigh(HOST_PIN); 
//        }else{
//          sleep(1000);
//          outputHigh(DEBUG_PIN);
        }
      }
      return true;
    }
  }else if(op_code == RIFLT){
    if(length >= 4){
      if(store){
        storeCommandInTable(command, length);
      }else{
        int reg = command[2];
        int value = command[3];
        int result = compareRegister(reg, value);
        if(result == -1){
          outputHigh(HOST_PIN);
        }
      }
      return true;
    } 
  }else if(op_code == MODE){
    if(length >= 4){
      int reg = command[2];
      int mode = command[3];
      if(reg == GPIO1 && mode == 0x00){
        setInput(GPIO1_PIN); 
      }else if(reg == GPIO1 && mode == 0x01){
        setOutput(GPIO1_PIN);
      }
      return true;
    }
  }else{ //unknown op code
    //clearBuffer();
    return true; //??
  }
  return false;
}

void processCommandBuffer(){
  bool result = processCommand(command_buffer, command_length, true);
  if(result){
    clearBuffer();
  }
}

ISR(SPI_VECTOR){
  int byte = spiReceived() / 2;

  if(command_length < MAX_COMMAND_LEN){ //do not allow overflow, no command should be this long
    command_buffer[command_length++] = byte;
    processCommandBuffer();
  }else{
    clearBuffer();
  } 
}

int main(void){
  setOutput(M1PW_PIN);
  TCCR0B = 0;                     // stop timer 0
  TCCR0A = (1<<WGM00)|(1<<WGM01); // select fast pwm mode 3
  TCCR0A |= (1<<COM0A1);          //Clear OC0A on Compare Match when up-counting. Set OC0A on Compare Match when down-counting.
  OCR0A = 0x00;                   //duty cycle
  TCCR0B |= (1<<CS00);            // no prescaling

  setOutput(M2PW_PIN);
  TCCR1B = 0;
  TCCR1A = (1<<WGM00);
  TCCR1A |= (1<<COM1A1);
  OCR1A = 0x00;
  TCCR1B |= (1<<CS00);
  
  setOutput(HOST_PIN);
  outputLow(HOST_PIN);

  // Select Vref=AVcc
  ADMUX |= (1<<REFS0);
  //set prescaller to 128 and enable ADC 
  ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);  

  //Setup SPI/USI
  //CTRL_PORT |= _BV(DO_PIN);
  setupSpi();

  clearBuffer();
  sei();
  
  setOutput(M1DR_PIN);
  outputLow(M1DR_PIN);
  setOutput(M2DR_PIN);
  outputLow(M2DR_PIN);

  //by default, let's set GPIO pins to be inputs
  setInput(GPIO1_PIN);
  
  //blink to signal ready...
  setOutput(DEBUG_PIN);
  outputHigh(DEBUG_PIN);
  sleep(1000);
  outputLow(DEBUG_PIN);
/*
  table_size = 1;
  table[0][0] = 0x00;
  table[0][1] = 0x04;
  table[0][2] = 0x06;
  table[0][3] = 0x01; 
*/  
  //ready!
  while(1){
    for(int i = 0; i != table_size; i++){
      processCommand(table[i], MAX_COMMAND_LEN, false); 
    }
  }
}
