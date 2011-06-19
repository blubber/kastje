#include <WProgram.h>
#include "messages.h"
#include "comm.h"
#include "config.h"
#include "leds.h"

static int n_led_groups = N_LED_GROUPS;
static LedGroup groups[N_LED_GROUPS];


// Function defs
ColorRGB HSL_to_RGB (ColorHSL);
ColorHSL RGB_to_HSL (ColorRGB);
void send_rgb(uint8_t, uint8_t, uint8_t);

void leds_set_color (int id, ColorRGB color) {
  if (id >= n_led_groups) {
    return;
  }

  groups[id].color = color;
  analogWrite(groups[id].pin_r, color.r);
  analogWrite(groups[id].pin_g, color.g);
  analogWrite(groups[id].pin_b, color.b);
  groups[id].cycling = false;
}


CommMessage callback_rgb (CommMessage msg) {
  if (msg.payload_len == 4) {
    ColorRGB color;
    byte id = msg.payload[0];
    color.r = msg.payload[1];
    color.g = msg.payload[2];
    color.b = msg.payload[3];
    leds_set_color(id, color);
  }

  return comm_new(MSG_ACK, 0, NULL);
}


CommMessage callback_cycle (CommMessage msg) {
  if (msg.payload_len == 4) {
    byte group = (byte)msg.payload[0];
    byte speed = (byte)msg.payload[1];
    float sat = ((unsigned char)msg.payload[2])/255.0;
    float light = ((unsigned char)msg.payload[3])/255.0;
    
    if (group == 255) {
      for (int i = 0; i < n_led_groups; i++) {
        LedGroup g = groups[i];
        if (g.type != LEDS_EMPTY) {
          g.cycle_speed = speed;
          g.color_hsl.saturation = sat;
          g.color_hsl.lightness = light;
          g.cycling = true;
          groups[group] = g;
        }
      }
    } else if (group < n_led_groups) {
      LedGroup g = groups[group];
      if (g.type != LEDS_EMPTY) {
        g.cycle_speed = speed;
        g.color_hsl.saturation = sat;
        g.color_hsl.lightness = light;
        g.cycling = true;
        groups[group] = g;
      }
    }
  }

  return comm_new(MSG_ACK, 0, NULL);
}

void leds_init() {
  for (int i = 0; i < n_led_groups; i++) {
    groups[0].type = LEDS_EMPTY;
  }
  comm_register(MSG_RGB, callback_rgb);
  comm_register(MSG_CYCLE, callback_cycle);
}


void leds_add_group (byte id, LedGroup group) {
  groups[id].type = group.type;
  groups[id].pin_r = group.pin_r;
  groups[id].pin_g = group.pin_g;
  groups[id].pin_b = group.pin_b;

  groups[id].cycling = false;
  groups[id].cycle_speed = 20;
  groups[id].last_update = 0;
  groups[id].color = COLOR_RGB(0, 0, 0);  
  groups[id].color_hsl = COLOR_HSL(0, 0, 0);
}


void leds_flash () {
  if (FLASH_LED_GROUP > -1) {
    if (groups[FLASH_LED_GROUP].type == LEDS_PWM) {
      LedGroup g = groups[FLASH_LED_GROUP];
      analogWrite(g.pin_r, 255); analogWrite(g.pin_g, 255); analogWrite(g.pin_b, 255);
      delay(1000);
      analogWrite(g.pin_r, 0); analogWrite(g.pin_g, 0); analogWrite(g.pin_b, 0);
    }
  }
}


void leds_loop () {
  for (int i = 0; i < n_led_groups; i++) {
    if (groups[i].type == LEDS_EMPTY) {
      continue;
    }

    if (groups[i].cycling == true) {
      unsigned long m = millis();
      if (m < groups[i].last_update) { // overflow
        groups[i].last_update = m;
        continue;
      } else if ((m - groups[i].last_update) >= groups[i].cycle_speed) {
        groups[i].color_hsl.hue += 0.001;
        if (groups[i].color_hsl.hue > 1) {
          groups[i].color_hsl.hue = 0;
        }

        ColorRGB color = HSL_to_RGB(groups[i].color_hsl);
        groups[i].color = color;
        analogWrite(groups[i].pin_r, color.r);
        analogWrite(groups[i].pin_g, color.g);
        analogWrite(groups[i].pin_b, color.b);
        groups[i].last_update = m;
      }
    }
  }
}


void leds_inc_h (int group) {
  ColorHSL h = RGB_to_HSL(groups[0].color);
  
  h.hue += 0.005;
  h.hue = h.hue > 1 ? h.hue - 1 : h.hue;

  ColorRGB r = HSL_to_RGB(h);

  leds_set_color(group, r);
}


void leds_dec_h (int group) {
  ColorHSL h = RGB_to_HSL(groups[0].color);
  
  h.hue -= 0.005;
  h.hue = h.hue < 0 ? h.hue + 1 : h.hue;

  ColorRGB r = HSL_to_RGB(h);

  leds_set_color(group, r);
}


ColorHSL RGB_to_HSL (ColorRGB rgb) {
  ColorHSL hsl;
  float R = rgb.r / 255.0;
  float G = rgb.g / 255.0;
  float B = rgb.b / 255.0;
  float H = 0, S = 0, L = 0;
  
  float rgb_min = min(R, min(G, B));
  float rgb_max = max(R, max(G, B));
  float delta = rgb_max - rgb_min;

  L = (rgb_min + rgb_max) / 2;

  if (delta == 0) {
    H = 0;
    S = 0;
  } else {
    S = (L < 0.5) ? delta / (rgb_min + rgb_max) : delta / (2 - delta);

    float del_R = (((rgb_max - R) / 6) + (delta / 2)) / delta;
    float del_G = (((rgb_max - G) / 6) + (delta / 2)) / delta;
    float del_B = (((rgb_max - B) / 6) + (delta / 2)) / delta;

    if (R == rgb_max) H = del_B - del_G;
    else if (G == rgb_max) H = (1.0 / 3.0) + del_R - del_B;
    else if (B == rgb_max) H = (2.0 / 3.0) + del_G - del_R;
  }
  
  hsl.hue = H < 0 ? H + 1 : (H > 1 ? H - 1 : H);
  hsl.saturation = S;
  hsl.lightness = L;

  return hsl;
}



double Hue_to_RGB (double v1, double v2, double H) {
  H = H < 0 ? H + 1 : (H > 1 ? H - 1 : H);
  if ((6.0 * H) < 1.0) {
    return v1 + ((v2 - v1) * 6.0 * H);
  }
  if ((2.0 * H) < 1.0) {
    return v2;
  }
  if (( 3.0 * H) < 2.0) {
    return v1 + ((v2 - v1) * (( 2.0 / 3.0) - H) * 6.0);
  }
  return v1;
}


ColorRGB HSL_to_RGB(ColorHSL hsl){
  ColorRGB rgb;
  double H = hsl.hue, S = hsl.saturation, L = hsl.lightness;
  double R, G, B;

  if (S == 0) {
    R = 255 * L;
    G = 255 * L;
    B = 255 * L;
  } else {
    double v2 = L < 0.5 ? L * (1.0 + S) : (L + S) - (S * L);
    double v1 = (2.0 * L) - v2;
    R = 255.0 * Hue_to_RGB(v1, v2, H + (1.0/3.0));
    G = 255.0 * Hue_to_RGB(v1, v2, H);
    B = 255.0 * Hue_to_RGB(v1, v2, H - (1.0/3.0));
   
  }

  rgb.r = (byte)R;
  rgb.g = (byte)G;
  rgb.b = (byte)B;

  return rgb;
}


void leds_sync_rf (int group) {
  ColorRGB c = groups[group].color;

  for (int i = 0; i < n_led_groups; i++) {
    if (groups[i].type == LEDS_RF) {
      send_rgb(c.r, c.g, c.b);
    }
  }
}



int alpha = 720;
int offset = 0; // 16 of 160
int sync_length = 5;

inline void H () {
  digitalWrite(RF_TX_PIN, LOW);
  delayMicroseconds(alpha - 56); // - offset);
  digitalWrite(RF_TX_PIN, HIGH);
  delayMicroseconds(alpha + 24); // - offset);
}

inline void L () {
  digitalWrite(RF_TX_PIN, HIGH);
  delayMicroseconds(alpha); // - offset);
  digitalWrite(RF_TX_PIN, LOW);
  delayMicroseconds(alpha); // - offset);
}


inline void send_byte (byte counter, byte data) {
  uint16_t out = data + (counter << 8);
  uint8_t parity = 1;
  
  /* Send some bits to set the receiver gain */
  for (int i = 0; i < 20; i++) {
    digitalWrite(RF_TX_PIN, HIGH);
    delayMicroseconds(sync_length * 2 * alpha);
    digitalWrite(RF_TX_PIN, LOW);
    delayMicroseconds(alpha);
  }

  /* Send sync bit */
  digitalWrite(RF_TX_PIN, HIGH);
  delayMicroseconds((sync_length * (alpha)));
  H();

  for (int i = 0; i < 12; i++) {
    if (out & (1<<(11-i))) {
      parity ^= 0x01;
      H();
    } else {
      L();
    }
  }

  parity ? H() : L();
  digitalWrite(RF_TX_PIN, LOW);
  delayMicroseconds(100);
}


void send_rgb (uint8_t R, uint8_t G, uint8_t B) {
  uint8_t check = R + G + B;
  
  send_byte(2, R);
  send_byte(3, G);
  send_byte(4, B);
  send_byte(5, check);
  send_byte(1, 'R');
}










