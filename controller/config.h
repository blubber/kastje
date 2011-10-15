#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Ident string for this arduino */
#define IDENT_STRING "Arduino 1"

/* The number of led groups */
#define N_LED_GROUPS 2

/* Flash ledgroup when init is done, -1 for no flash */
#define FLASH_LED_GROUP -1

/* Kaku output pin */
#define RF_TX_PIN 12

/* Set the address of a MCP23017 (if any) */
#define MCP23017 0x20

#endif
