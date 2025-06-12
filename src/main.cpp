#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

#define NUM_STRIPS 5
#define LEDS_PER_STRIP 144

#define NUM_PIXELS (NUM_STRIPS * LEDS_PER_STRIP)
#define LED_PIN D6

// Knob to Pin mapping
#define R1 A8  // red speed
#define R2 A5  // green speed
#define R3 A2  // blue speed
#define R4 A9  // red wavelength
#define R5 A4  // green wavelength
#define R6 A1  // blue wavelength
#define R7 A10 // red brightness
#define R8 A3  // green brightness
#define R9 A0  // blue brightness

// knob min/max bit values
#define KNOB_MIN_VAL 0
#define KNOB_MAX_VAL 4095
#define KNOB_MID_VAL ((KNOB_MAX_VAL - KNOB_MIN_VAL) / 2)

// speed limits in units led/s
#define SPEED_MIN 0.0
#define SPEED_MAX 0.5

// wavelength limits in units leds
#define WAVELN_MIN 0.0
#define WAVELN_MAX 400.0
#define WAVELN_THRESHOLD 3.0 // only change by whole number of leds

#define BRIGHTNESS_MIN 0.0
#define BRIGHTNESS_MAX 255.0
#define BRIGHT_CHANGE_THRESHOLD 0 // only update param if changes by this fraction of max

struct color {
  // pot knobs
  int speed_pin;
  int waveln_pin;
  int brightness_pin;
  // wave parameters
  double speed_current;
  double speed_target;
  double waveln_current;
  double waveln_target;
  double brightness_current;
  double brightness_target;
  // phase offset in microseconds
  uint32_t phase_us;
};

struct color red = {R1, R4, R7, 0, 0, 0, 0, 0, 0};
struct color grn = {R2, R5, R8, 0, 0, 0, 0, 0, 0};
struct color blu = {R3, R6, R9, 0, 0, 0, 0, 0, 0};

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Read potentiometer value from a knob, returns a value between 0.0 and 1.0
// invert ADC value so value increases when turned clockwise
double read_knob(int knob_pin) {
  return (KNOB_MAX_VAL - analogRead(knob_pin)) / (double)(KNOB_MAX_VAL);
}

// function to apply hysteresis to a value
double apply_hysteresis(double value, double last_value, double threshold) {
  if (fabs(value - last_value) < threshold) {
    return last_value; // no significant change
  }
  return value; // significant change
}

// Updates color params based on knobs
void update_params() {
  // Update speed
  red.speed_target = read_knob(red.speed_pin) * (SPEED_MAX - SPEED_MIN) + SPEED_MIN;
  grn.speed_target = read_knob(grn.speed_pin) * (SPEED_MAX - SPEED_MIN) + SPEED_MIN;
  blu.speed_target = read_knob(blu.speed_pin) * (SPEED_MAX - SPEED_MIN) + SPEED_MIN;
  // for now, set current speed to target speed
  red.speed_current = red.speed_target;
  grn.speed_current = grn.speed_target;
  blu.speed_current = blu.speed_target;

  // Update wavelength target
  // TODO: should wavelength be an integer?
  red.waveln_target = read_knob(red.waveln_pin) * (WAVELN_MAX - WAVELN_MIN) + WAVELN_MIN;
  grn.waveln_target = read_knob(grn.waveln_pin) * (WAVELN_MAX - WAVELN_MIN) + WAVELN_MIN;
  blu.waveln_target = read_knob(blu.waveln_pin) * (WAVELN_MAX - WAVELN_MIN) + WAVELN_MIN;
  // for now, set current wavelength to target wavelength
  red.waveln_current = red.waveln_target;
  grn.waveln_current = grn.waveln_target;
  blu.waveln_current = blu.waveln_target;

  // Update brightness target
  red.brightness_target = (int)(read_knob(red.brightness_pin) * (BRIGHTNESS_MAX - BRIGHTNESS_MIN) + BRIGHTNESS_MIN);
  grn.brightness_target = (int)(read_knob(grn.brightness_pin) * (BRIGHTNESS_MAX - BRIGHTNESS_MIN) + BRIGHTNESS_MIN);
  blu.brightness_target = (int)(read_knob(blu.brightness_pin) * (BRIGHTNESS_MAX - BRIGHTNESS_MIN) + BRIGHTNESS_MIN);
  // for now, set current brightness to target brightness
  red.brightness_current = red.brightness_target;
  grn.brightness_current = grn.brightness_target;
  blu.brightness_current = blu.brightness_target;
}

void debug_print_params() {
  Serial.println("DEBUG:");
  Serial.printf("red speed(%.6f,%.6f) wave(%.6f,%.6f) bright(%.6f,%.6f)\n", red.speed_current, red.speed_target, red.waveln_current, red.waveln_target, red.brightness_current, red.brightness_target);
  Serial.printf("grn speed(%.6f,%.6f) wave(%.6f,%.6f) bright(%.6f,%.6f)\n", grn.speed_current, grn.speed_target, grn.waveln_current, grn.waveln_target, grn.brightness_current, grn.brightness_target);
  Serial.printf("blu speed(%.6f,%.6f) wave(%.6f,%.6f) bright(%.6f,%.6f)\n", blu.speed_current, blu.speed_target, blu.waveln_current, blu.waveln_target, blu.brightness_current, blu.brightness_target);
}

// calculate color value for a pixel based on its index, current time, and color parameters
uint8_t calc_color(color* rgb, int i, uint32_t t) {
  double pos;
  if (rgb->waveln_current == 0) {
    // If wavelength is 0, just use speed to calculate position
    pos = rgb->speed_current * (t - rgb->phase_us) / 1000000.0;
  } else {
    pos = ((double) i / rgb->waveln_current) + (rgb->speed_current * (t - rgb->phase_us) / 1000000.0);
  }

  // Apply sine wave transformation
  pos = 0.5 * (1.0 + sin(2 * PI * pos));
  // Apply brightness
  pos = pos * rgb->brightness_current;

  return pos;
}

void setup() {
  Serial.begin(115200);

  pinMode(R1, INPUT);
  pinMode(R2, INPUT);
  pinMode(R3, INPUT);
  pinMode(R4, INPUT);
  pinMode(R5, INPUT);
  pinMode(R6, INPUT);
  pinMode(R7, INPUT);
  pinMode(R8, INPUT);
  pinMode(R9, INPUT);

  pinMode(LED_PIN, OUTPUT);
  pixels.begin();
}

void loop() {
  static uint32_t timer = millis();
  if (millis() - timer > 1) {
    // update every 1ms
    update_params();
    timer = millis();
  }

  uint32_t t = micros();
  for (int i = 0; i < NUM_PIXELS; i++) {
    uint8_t r = calc_color(&red, i, t);
    uint8_t g = calc_color(&grn, i, t);
    uint8_t b = calc_color(&blu, i, t);
    pixels.setPixelColor(i, pixels.Color(r,g,b));
  }
  pixels.show();

  // DEBUG
  static uint32_t debug_timer = millis();
  if (millis() - debug_timer > 500) {
    debug_print_params();
    debug_timer = millis();
  }
}
