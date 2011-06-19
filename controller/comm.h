#ifndef __COMM_H__
#define __COMM_H__

#include <WProgram.h>
#include <stdarg.h>

/* This struct is used for message passing */
typedef struct __comm_message {
  byte cmd;
  byte payload_len;
  char *payload;
} CommMessage;

/* Callback function type */
typedef __comm_message (*comm_callback)(CommMessage);


/* Call either one of these functions to init the comm library */
void comm_init();
void comm_init_serial (int baudrate);

/* Creates a new CommMessage, this function will allocate
 * space and copy the payload to it, if payload_len > 0 */
CommMessage comm_new (byte cmd, byte payload_len, char *payload);

/* Register a new callback.
 * Callbacks must be of type comm_callback.
 * At most 1 callback can be registered for each message
 */
void comm_register(byte cmd, comm_callback func);

/* Serial read loop, call from mainloop. */
void comm_serial_loop ();

/* Dispatch a comm message */
void comm_dispatch(CommMessage msg);

/* Free the message payload, must be called on each comm message */
void comm_free(CommMessage msg);

/* Send a debug message */
void comm_debug(char *fmt, ...);

#endif 
