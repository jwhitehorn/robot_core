#!/bin/bash

avr-gcc -std=c99 -O2 -Os -mmcu=attiny2313 -o $1.elf $1.c && avr-objcopy -R .eeprom -O ihex $1.elf $1.hex && avrdude -p attiny2313 -c gpio -v -U flash:w:$1.hex 
