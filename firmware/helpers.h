//Helper file to abstract away platform specifics.

//Suport for both attiny and atmega devices
#ifdef PORTA
#define ATTINY
#define SPI_VECTOR USI_OVERFLOW_vect
#else
#define ATMEGA
#define SPI_VECTOR SPI_STC_vect
#define USIDR SPDR
#define USISR SPDR
#endif

//ATtiny doesn't support SPI per se, but it can use USI as SPI... let's abstract that...
int spiReceived(){
  #ifdef ATTINY
  int byte = USIDR / 2;
  USISR = (1<<USIOIF);
  USIDR = byte;
  return byte;
  #else
  SPDR = 0x00;  
  return SPDR; 
  #endif
}

void setupSpi(){
  #ifdef ATTINY
  USICR = _BV(USIOIE) | _BV(USIWM0) | _BV(USICS0) | _BV(USICS1);
  USISR = (1<<USIOIF);
  #else
  setup_spi(SPI_MODE_1, SPI_MSB, SPI_INTERRUPT, SPI_SLAVE);
  #endif
}


//General helpers:
#define __output_low(port,pin, portdir) port &= ~(1<<pin)    
#define __output_high(port,pin, portdir) port |= (1<<pin)
#define __set_input(port, pin, portdir) portdir &= ~(1<<pin)
#define __set_output(port, pin, portdir) portdir |= (1<<pin)

#define outputLow(pin) __output_low(pin)
#define outputHigh(pin) __output_high(pin)
#define setInput(pin) __set_input(pin)
#define setOutput(pin) __set_output(pin)

#define sleep _delay_ms

//Pins:
#define HOST_PIN    PORTD, PD1, DDRD
#define DEBUG_PIN   PORTD, PD0, DDRD
#ifdef ATTINY
#define M1DR_PIN    PORTA, PA1, DDRA
#define M2DR_PIN    PORTA, PA0, DDRA
#define M1PW_PIN    PORTB, PB2, DDRB
#define M2PW_PIN    PORTB, PB6, DDRB
#else
#define M1DR_PIN    PORTD, PD2, DDRD
#define M2DR_PIN    PORTD, PD3, DDRD
#define M1PW_PIN    PORTD, PD6, DDRD
#define M2PW_PIN    PORTB, PB1, DDRB
#endif
