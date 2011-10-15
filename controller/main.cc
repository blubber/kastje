#include <WProgram.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "config.h"
#include "comm.h"
#include "messages.h"
#include "leds.h"
#include "kaku.h"

#ifdef MCP23017
#include "Wire.h"
#include "mcp23017.h"

volatile uint8_t mcp_int = 0;

void mcp_isr () {
  mcp_int++;
}

void writeRegister (uint8_t reg, uint8_t data) {
  Wire.beginTransmission(MCP23017);
  Wire.send(reg);
  Wire.send(data);
  Wire.endTransmission();
}

void writeRegisterBoth (uint8_t reg, uint8_t data1, uint8_t data2) {
  Wire.beginTransmission(MCP23017);
  Wire.send(reg);
  Wire.send(data1);
  Wire.send(data2);
  Wire.endTransmission();
}

uint8_t readRegister (uint8_t reg) {
  Wire.beginTransmission(MCP23017);
  Wire.send(reg);
  Wire.endTransmission();

  Wire.requestFrom(MCP23017, 1);
  return Wire.receive();
}

#endif


CommMessage ident_callback (CommMessage msg) {
  return comm_new(MSG_IDENT, strlen(IDENT_STRING), IDENT_STRING);
}

int main() {
  init(); // Init arduino stuff DO NOT REMOVE

  /* Initialize comm library. Listen on Serial with 9600 baud */
  comm_init_serial(9600);
  comm_register(MSG_IDENT, ident_callback);

  /* Initialize kaku stuff */
  kaku_init();

  /* Initialize led library */
  leds_init();

  /* Add led groups */
  LedGroup leds = { LEDS_PWM, 11, 10, 9 };
  LedGroup rf = { LEDS_RF, 0, 0, 0 };
  leds_add_group(0, leds);
  leds_add_group(1, rf);
  
  /* Flash leds to signal init done */
  leds_flash();

  /* Configure MCP23017 if available */
#ifdef MCP23017
  Wire.begin();

  writeRegister(0x05, 0x00);
  writeRegisterBoth(IODIRA, 0xFF, 0xFF);
  writeRegisterBoth(GPPUA, 0xFF, 0xFF);
  writeRegisterBoth(GPINTENA, 0xFF, 0xFF);
  writeRegisterBoth(DEFVALA, 0xFF, 0xFF);
  writeRegisterBoth(INTCONA, 0b11111011, 0b01011111);
  

  attachInterrupt(0, mcp_isr, CHANGE);
  attachInterrupt(1, mcp_isr, CHANGE);

  /* Clear capture registesrs, just to be sure */
  readRegister(INTCAPA);
  readRegister(INTCAPB);

  static uint32_t last_press = 0;
#endif

  for (;;) {
    comm_serial_loop();
    leds_loop();

#ifdef MCP23017
    if (mcp_int > 0) {
      uint16_t keyval = 0;
      uint32_t m = millis();
      uint8_t intfa = readRegister(INTFA);
      uint8_t intfb = readRegister(INTFB);
      uint16_t intf = (intfa << 8) + intfb;
      mcp_int--;
      
      if (intfa) {
        keyval |= (readRegister(INTCAPA) << 8);
      }

      if (intfb) {
        keyval |= (readRegister(INTCAPB));
      }

      for (int i = 0; i < 16; i++) {
        if (intf & (1 << i)) {
          if ((i == 4) || (i == 15)) {
            if ((m - last_press > 50)) {
              for (int u = 1; u < 5; u++) {
                kaku_set(3, u, i == 4);
              }
            }
          } else if ((i < 4) || ((i > 10) && (i < 15))) {
            if ((m - last_press > 50)) {
              kaku_set(3, i < 4 ? i+1 : i - 10, i < 4 ? 1 : 0);
            }
          } else if (i == 7) {            
            if ((m - last_press > 50)) {
              delay(1000);
              leds_sync_rf(0);
            }
          } else if (i == 1) {
            // sync rf leds
          } else if (i == 10 || i == 5) {
            if (keyval == 64479) {
              leds_inc_h(0);
            } else if (keyval == 65535) {
              leds_dec_h(0);
            }
          } else {
            Serial.print("Unknown i: ");
            Serial.println(i);
          }
        }
      }
      last_press = m;
    }
#endif
  }

  return 0;
}


