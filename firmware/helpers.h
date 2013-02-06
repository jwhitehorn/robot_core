#ifdef PORTA
#define ATTINY
#else
#define ATMEGA
#endif

void output_low(int port, int pin){ port &= ~(1<<pin); }
void output_high(int port, int pin){ port |= (1<<pin); }
void set_input(int portdir, int pin){ portdir &= ~(1<<pin); }
void set_output(int portdir, int pin){ portdir |= (1<<pin); }