#include <WProgram.h>
#include <stdarg.h>
#include "config.h"
#include "comm.h"
#include "messages.h"


static comm_callback callbacks[256] = { NULL };
static bool have_serial = false;


static void comm_send (CommMessage msg) {
  Serial.write(msg.cmd);
  Serial.write(msg.payload_len);
  for (int i = 0; i < msg.payload_len; i++) {
    Serial.write(msg.payload[i]);
  }
}


void comm_init() {
 
}

void comm_init_serial (int baudrate) {
  comm_init();
  Serial.begin(baudrate);
  have_serial = true;
}

CommMessage comm_new (byte cmd, byte payload_len, char *payload) {
  CommMessage msg;
  msg.cmd = cmd;
  msg.payload_len = payload_len;

  if (payload_len > 0) {
    msg.payload = (char *)malloc(payload_len);
    memcpy(msg.payload, payload, payload_len);
  } else {
    msg.payload = NULL;
  }
  return msg;
}

void comm_register(byte cmd, comm_callback func) {
  callbacks[cmd] = func;
}

void comm_dispatch (CommMessage msg) {
  if (callbacks[msg.cmd]) {
    CommMessage retval = (callbacks[msg.cmd])(msg);
    comm_send(retval);
    comm_free(retval);
  } else {
    comm_send( comm_new(MSG_UNKNOWN, 0, NULL) );
  }
}

void comm_serial_loop () {
  static int state = 0;
  static CommMessage msg;

  if (!have_serial) return;

  if ((Serial.available() > 0) || (state == 3)) {

    if (state == 0) { // Read cmd byte
      if (msg.payload_len > 0) {
        comm_free(msg);
        msg.payload_len = 0;
        msg.payload = NULL;
      }
      msg.cmd = Serial.read();
      state = 1;
    } else if (state == 1) { // Read payload_len byte
      msg.payload_len = Serial.read();
      if (msg.payload_len > 0) {
        msg.payload = (char *)malloc(msg.payload_len);
        state = 2;
      } else {
        state = 3; // Skip reading the payload
      }
    } else if (state == 2) { // Read payload 
      if (Serial.available() >= msg.payload_len) {
        for (int i = 0; i < msg.payload_len; i++) {
          msg.payload[i] = Serial.read();
        }
        state = 3;
      }
    } else if (state == 3) { // Dispatch
      comm_dispatch(msg);
      state = 0;
    }
  }
}

void comm_free (CommMessage msg) {
  if (msg.payload_len > 0) {
    free(msg.payload);
    msg.payload_len = 0;
  }
}

void comm_debug (char *fmt, ...) {
  char buf[512];
  int len;
  va_list ap;
  va_start(ap, fmt);

  len = vsnprintf(buf, 511, fmt, ap);
  CommMessage msg = comm_new(MSG_DEBUG, len, buf);
  comm_send(msg);
  comm_free(msg);
}







