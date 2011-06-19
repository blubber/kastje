#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/io.h>

uint16_t alpha = 720;
uint16_t sync_bit = 5 * alpha;
uint16_t delta = 60;

uint8_t cmd, R, G, B, check;

volatile uint32_t data = 0;
volatile uint8_t nbits = 0;
volatile uint8_t reading = 0;
volatile uint16_t counter;


void send_byte (uint8_t b) {
  //  while(!(UCSR0A & (1<<UDRE0)));
  //UDR0 = (uint8_t)b;
}

void handle_data () {
  uint8_t parity = 0;
  uint16_t raw_data = 0;
  uint32_t shift = data;

  /* Sanity check on the manchester bits */
  for (int i = 2; i < nbits; i++) {
    if (((shift & 0b001) == (shift & 0b010)) && ((shift & 0b01) == (shift & 0b100))) {
      return;
    }
    shift >>= 1;
  }

  for (int i = 0; i < (nbits/2); i++) {
    if ((data >> (2 * i)) & 0x01) {
      raw_data |= (1 << i);
      if (i != 0) {
        parity ^= 0x01;
      }
    }
  }

  if (parity != (raw_data & 0x01)) {
    return;
  }
  
  raw_data >>= 1; // remove parity bit
  uint8_t cnt = (raw_data & 0x0F00) >> 8;
  uint8_t b = (raw_data & 0xFF);

  switch (cnt) {
  case 1:
    cmd = b; break;
  case 2:
    R = b; break;
  case 3:
    G = b; break;
  case 4: 
    B = b; break;
  case 5:
    check = b; break;
  }
}


ISR(INT0_vect) {
  if (reading == 1) {
    uint16_t length = TCNT1 - counter;
    if (length < 16) { return; }

    counter = TCNT1;


    if ((length > 1000) && (length < 1900)) {
      if (PIND & _BV(2)) {
        data = data << 2;
      } else {
        data = (data << 2) + 3;
      }
      nbits += 2;
    } else if ((length > 450) && (length < 1000)) {
      if (PIND & _BV(2)) {
        data = (data << 1);
      } else {
        data = (data << 1) + 1;
      }
      nbits++;
    }
  }
}


int main() {

    //  return 0;
  /* Init Serial */
  /*  
  UBRR0H = (103 >> 8);
  UBRR0L = 103;
  UCSR0A = (1 << U2X0);
  UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
  */

  // RX on PB0
  DDRD = 0;
  PORTD = 0;
  DDRB = 0;
  //PORTB |= _BV(5);
  DDRB |= _BV(5) | _BV(3);
  PORTB |= _BV(5) | _BV(3);

  // Setup timer
  TCCR1A = 0x00;
  TCCR1B = _BV(CS11);

  // Setup interrupt on INT0
  EICRA |= _BV(ISC00);
  EIMSK |= _BV(INT0);

  // Setup PWM
  DDRD |= _BV(5) | _BV(6) | _BV(3);

  TCCR0A |= _BV(WGM00) | _BV(WGM01);
  TCCR0B |= _BV(CS01);  
  PORTD &= ~_BV(5) | ~_BV(6);

  OCR0A = 0;
  OCR0B = 0;
  
  TCCR2A |=_BV(WGM21) | _BV(WGM20);
  TCCR2B |= _BV(CS21);
  PORTB |= _BV(3);

  OCR2A = 0;
  
  cli();

  uint8_t high = 0;
  uint8_t flank = 0;

  for (;;) {
    if (reading == 1) {
      if (nbits == 28) {
        reading = 0;
        cli();
      } else {
        continue;
      }
    }

    cli();

    if (nbits == 28) {
      handle_data();
      reading = data = nbits = 0;

      if (cmd) {
        if (check == ((uint8_t)(R + G + B))) {
          send_byte(0); send_byte(R);
          send_byte(0); send_byte(G);
          send_byte(0); send_byte(B);
          if (G == 0) {
            TCCR0A &= ~_BV(COM0A1);
            PORTD &= ~_BV(6);
          } else if (G == 255) {
            TCCR0A &= ~_BV(COM0A1);
            PORTD |= _BV(6);
          } else {
            TCCR0A |= _BV(COM0A1);
            OCR0A = G;
          }

          if (R == 0) {
            TCCR0A &= ~_BV(COM0B1);
            PORTD &= ~_BV(5);
          } else if (R == 255) {
            TCCR0A &= ~_BV(COM0B1);
            PORTD |= _BV(5);
          } else {
            TCCR0A |= _BV(COM0B1);
            OCR0B = R;
          }

          if (B == 0) {
            TCCR2A &= ~_BV(COM2A1);
            PORTB &= ~_BV(3);
          } else if (B == 255) {
            TCCR2A &= ~_BV(COM2A1);
            PORTB |= _BV(3);
          } else {
            TCCR2A |= _BV(COM2A1);
            OCR2A = B;
          }
        }

        R = G = B = cmd = check = 0;
      }
    }

    if ((PIND & _BV(2)) > 0) {
      if (high == 0) {
        high = 1;
        flank = 1;
      } else {
        flank = 0;
        high = 1;

      }
    } else {
      if (high == 1) {
        flank = 1;
        high = 0;
      } else {
        flank = 0;
        high = 0;
      }
    }

    if (flank == 1) {
      flank = 0;
      uint16_t length = (TCNT1 - counter);
      counter = TCNT1;


      if ((length < (sync_bit + delta)) && (length > (sync_bit - delta))) {
        if (high == 0) {
          //          PORTB ^= _BV(5);
          nbits = data = 0;
          reading = 1;
          sei();
        }
      }
    }
  }

  return 0;
}


