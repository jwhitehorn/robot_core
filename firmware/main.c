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

#define HOST_PIN    PB3
#define DEBUG_PIN   PD0

//registers
#define REG_M1PW   0x01

//op codes
#define REGW       0x01
#define CLRH       0x02


#define MAX_COMMAND_LEN 10
char command_buffer[MAX_COMMAND_LEN];
int command_length = 0;

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
          output_high(PORTB, HOST_PIN);
          }else{
            output_low(PORTB, HOST_PIN);
          }
      }else if(reg == REG_M1PW){
        OCR0A = byte;
      }
      clearBuffer();
    }//else, wait for more bytes...
  }else if(op_code == CLRH){
    /*
    * CLRH - Clear Host Pin - Resets the host pin. E.g., it sets it low.
    * Command length: 2 bytes
    */
    output_low(PORTB, HOST_PIN);
    clearBuffer();
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
  
  DDRB |= (1<<HOST_PIN);          // make host pin an output
  PORTB |= (0<<HOST_PIN);         // host pin starts low

  set_output(DDRD, DEBUG_PIN);
  output_high(PORTD, DEBUG_PIN);
  _delay_ms(1000);
  output_low(PORTD, DEBUG_PIN);

  //Setup SPI/USI
  CTRL_PORT |= _BV(DO_PIN);
  USICR = _BV(USIOIE) | _BV(USIWM0) | _BV(USICS0) | _BV(USICS1);
  USISR = (1<<USIOIF);

  clearBuffer();
  sei();
  while(1);
}
