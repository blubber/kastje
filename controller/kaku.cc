#include <WProgram.h>
#include <wiring.h>
#include "kaku.h"
#include "config.h"
#include "comm.h"
#include "messages.h"


static int dataPin = RF_TX_PIN;
static int alpha = 103;
static void inline raw_conrad_switch(int, int, boolean);
void kaku_on (int, int);
void kaku_off (int, int);


CommMessage kaku_callback (CommMessage msg) {
  if (msg.payload_len == 2) {
    int group = (int)(msg.payload[0]);
    int module = (int)(msg.payload[1]);
    
    if (msg.cmd == MSG_ON) {
      kaku_on(group, module);
    } else if (msg.cmd == MSG_OFF) {
      kaku_off(group, module);
    }
  }
  
  return comm_new(MSG_ACK, 0, NULL);
}


void kaku_init () {
  pinMode(dataPin, OUTPUT);
  comm_register(MSG_ON, kaku_callback);
  comm_register(MSG_OFF, kaku_callback);
}


void kaku_on (int group, int module) {
  cli();
  raw_conrad_switch(group, module, true);
  raw_conrad_switch(group, module, true);
  raw_conrad_switch(group, module, true);
  raw_conrad_switch(group, module, true);
  sei();
}


void kaku_off (int group, int module) {
  cli();
  raw_conrad_switch(group, module, false);
  raw_conrad_switch(group, module, false);
  raw_conrad_switch(group, module, false);
  raw_conrad_switch(group, module, false);
  sei();
}


void kaku_set (int group, int module, int state) {
  if (state == 0) {
    kaku_off(group, module);
  } else {
    kaku_on(group, module);
  }
}

void inline H(int time){
  digitalWrite(dataPin, HIGH);
  delayMicroseconds(alpha * time);
}

void inline L(int time){
  digitalWrite(dataPin, LOW);
  delayMicroseconds(alpha * time);
}

void inline bitL(){
  H(4);
  L(12);
  H(4);
  L(12);
}

void inline bitH(){
  H(12);
  L(4);
  H(12);
  L(4);
}

void inline bitF(){
  H(4);
  L(12);
  H(12);
  L(4);
}

void inline bitSync(){
  H(4);
  L(128 - 4 - 3);  // sync bit korter correctie ivm timing tussen de pulsjes.
}



// group in 1-4 as on the receivers, module as in 1-4 as on the receivers. on is a boolean to define the new state.
void inline raw_conrad_switch(int group, int module, boolean on){
  int i = 0;
  for (i=0;i<4;i++){
    if (i == group - 1){
      bitL();
    } else {
      bitF();
    } 
  }
  for (i=0;i<4;i++){
    if (i == module - 1){
      bitL();
    } else {
      bitF();
    } 
  }
  bitF();
  bitF();
  bitF();
  if (on){
    bitF();
  } else {
    bitL(); 
  }
  bitSync();
}




