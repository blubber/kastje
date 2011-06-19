#ifndef __LEDS_H__
#define __LEDS_H__

#define COLOR_RGB(r, g, b) (ColorRGB){r, g, b}
#define COLOR_HSL(h, s, l) (ColorHSL){h, s, l}


enum LedGroupType {
  LEDS_EMPTY,
  LEDS_PWM,
  LEDS_RF,
  LEDS_TLC
};

typedef struct __color_rgb {
  byte r;
  byte g;
  byte b;
} ColorRGB;

typedef struct __color_hsl {
  float hue;
  float saturation;
  float lightness; 
} ColorHSL;

typedef struct __led_group {
  LedGroupType type;
  byte pin_r;
  byte pin_g;
  byte pin_b;
  boolean cycling;
  byte cycle_speed;
  unsigned long last_update;
  __color_rgb color;
  __color_hsl color_hsl;
} LedGroup;


/* Initialize led library */
void leds_init();

/* Add a group of leds */
void leds_add_group(byte id, __led_group group);

/* Flash leds after init */
void leds_flash ();

/* Led loop, add to main loop */
void leds_loop ();

/* Set color on a particular group */
void leds_set_color(int group, ColorRGB color);

/* Increase HSL Hue by 0.01 */
void leds_inc_h (int group);

/* Decrease HSL Hue by 0.01 */
void leds_dec_h (int group);

void leds_sync_rf(int group);


#endif 
