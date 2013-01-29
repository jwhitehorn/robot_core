#define F_CPU 8000000UL  // 8 MHz
#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define CTRL_PORT   DDRB 
#define DATA_PORT   PORTB 
#define CLK_PIN     PB7 
#define DI_PIN      PB5 
#define DO_PIN      PB6 

void run_loop(){
  while ((USISR & (1 << USIOIF)) == 0) {}; // Do nothing until USI has data ready
  OCR0A = USIDR;
  USISR = _BV(USIOIF); //Clear the overflow flag 
}

int main(void){
  DDRB |= (1<<PB2);               // make OC0A an output
  TCCR0B = 0;                     // stop timer 0
  TCCR0A = (1<<WGM00)|(1<<WGM01); // select fast pwm mode 3
  TCCR0A |= (1<<COM0A1);          //Clear OC0A on Compare Match when up-counting. Set OC0A on Compare Match when down-counting.
  OCR0A = 0x00;                   //duty cycle
  TCCR0B |= (1<<CS00);            // no prescaling

  //Setup SPI/USI
  CTRL_PORT |= _BV(DO_PIN);
  USICR = _BV(USIWM0) | _BV(USICS0) | _BV(USICS1);
  USISR = _BV(USIOIF);

  while(1){ run_loop(); }
}
