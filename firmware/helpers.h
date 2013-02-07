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
  return SPDR;  //not sure if this is right yet...
  #endif
}

void setupSpi(){
  #ifdef ATTINY
  USICR = _BV(USIOIE) | _BV(USIWM0) | _BV(USICS0) | _BV(USICS1);
  USISR = (1<<USIOIF);
  #else
  //not sure if the atmega code is right...
  /*
  EICRB = (1<<ISC01) | (0<<ISC00);
  EIMSK |= (1<<INT0);*/
  #endif
}


//General helpers:
void outputLow(int port, int pin){ port &= ~(1<<pin); }
void outputHigh(int port, int pin){ port |= (1<<pin); }
void setInput(int portdir, int pin){ portdir &= ~(1<<pin); }
void setOutput(int portdir, int pin){ portdir |= (1<<pin); }
#define sleep _delay_ms

//Pins:
#define HOST_PIN    PORTD, PD1
#define DEBUG_PIN   PORTD, PD0
#ifdef ATTINY
#define M1DR_PIN    PORTA, PA1
#define M2DR_PIN    PORTA, PA0
#else
#define M1DR_PIN    PORTD, PD2
#define M2DR_PIN    PORTD, PD3
#endif