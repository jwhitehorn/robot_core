#define F_CPU 8000000UL  // 8 MHz
#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//SPI
#define CTRL_PORT   DDRB 
#define DATA_PORT   PORTB 
#define CLK_PIN     PB7 
#define DI_PIN      PB5 
#define DO_PIN      PB6 

#define HOST_PIN    PB3

//registers
#define REG_M1PW   0x01

char command_buffer[10];
int command_length = 0;

void clearBuffer(){
    command_length = 0;
    for(int i = 0; i != 10; i++) command_buffer[i] = 0x00; //clear buffer
}

void processCommandBuffer(){
  if(command_length < 3) return;    //no command is this short
  
  int op_code = command_length[1];  //first byte of op-code isn't implemented yet

  if(op_code == 0x01){
    if(command_length >= 4){
      int reg = command_buffer[2];
      int byte = command_buffer[3];
      if(reg == 0x00){              //0x00 is undocumented, used for debugging. It echoes the received bit on the host pin.
        byte = byte > 0 ? 1 : 0;    //ensure only 1-bit
        PORTB |= (byte<<HOST_PIN); 
      }else if(reg == REG_M1PW){
        OCR0A = byte;
      }
      clearBuffer();
    }
  }else{
    //unknown op code
    clearBuffer();
  }
}

//main run-loop of the firmware
void runLoop(){
  while ((USISR & (1 << USIOIF)) == 0) {}; // Do nothing until USI has data ready
  
  if(command_length < 10){ //do not allow overflow, no command should be this long
    command_buffer[command_length++] = USIDR;
    processCommandBuffer();
  }else{
    clearBuffer();
  } 
  USISR = _BV(USIOIF); //Clear the overflow flag 
}

int main(void){
  DDRB |= (1<<PB2);               // make OC0A an output
  TCCR0B = 0;                     // stop timer 0
  TCCR0A = (1<<WGM00)|(1<<WGM01); // select fast pwm mode 3
  TCCR0A |= (1<<COM0A1);          //Clear OC0A on Compare Match when up-counting. Set OC0A on Compare Match when down-counting.
  OCR0A = 0x00;                   //duty cycle
  TCCR0B |= (1<<CS00);            // no prescaling
  
  DDRB |= (1<<HOST_PIN);          // make host pin an output
  PORTB |= (0<<HOST_PIN);         // host pin starts low

  //Setup SPI/USI
  CTRL_PORT |= _BV(DO_PIN);
  USICR = _BV(USIWM0) | _BV(USICS0) | _BV(USICS1);
  USISR = _BV(USIOIF);

  while(1){ runLoop(); }
}
