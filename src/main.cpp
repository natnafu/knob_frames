#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

// Constants for the LED strip
#define NUM_STRIPS 5
#define LEDS_PER_STRIP 144
#define NUM_PIXELS (NUM_STRIPS * LEDS_PER_STRIP)

// LED strip connected to this pin
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
#define SPEED_CHANGE_THRESHOLD (0.02 * (SPEED_MAX - SPEED_MIN))
#define SPEED_SMOOTHING_FACTOR 0.1

// wavelength limits in units leds
#define WAVELN_MIN 0.0
#define WAVELN_MAX 400.0
#define WAVELN_THRESHOLD 3.0 // only change by whole number of leds
#define WAVELN_SMOOTHING_FACTOR 0.1

#define BRIGHTNESS_MIN 0.0
#define BRIGHTNESS_MAX 255.0
#define BRIGHT_CHANGE_THRESHOLD (0.01 * (BRIGHTNESS_MAX - BRIGHTNESS_MIN))
#define BRIGHT_SMOOTHING_FACTOR 0.1

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
double apply_hysteresis(double new_value, double last_value, double threshold) {
  if (fabs(new_value - last_value) < threshold) {
    return last_value; // no significant change
  }
  return new_value; // significant change
}

// function to apply smoothing to a value
double apply_smoothing(double target_value, double current_value, double smoothing_factor) {
  return current_value + (target_value - current_value) * smoothing_factor;
}

// Updates color params based on knobs
// TODO: for now, set current to target values
void update_params(color* rgb) {
  double new_target;

  // Update speed
  new_target = read_knob(rgb->speed_pin) * (SPEED_MAX - SPEED_MIN) + SPEED_MIN;
  rgb->speed_target = apply_hysteresis(new_target, rgb->speed_target, SPEED_CHANGE_THRESHOLD);
  rgb->speed_current = apply_smoothing(rgb->speed_target, rgb->speed_current, SPEED_SMOOTHING_FACTOR);

  // Update wavelength target TODO: should wavelength be an integer?
  new_target = read_knob(rgb->waveln_pin) * (WAVELN_MAX - WAVELN_MIN) + WAVELN_MIN;
  rgb->waveln_target = apply_hysteresis(new_target, rgb->waveln_target, WAVELN_THRESHOLD);
  rgb->waveln_current = apply_smoothing(rgb->waveln_target, rgb->waveln_current, WAVELN_SMOOTHING_FACTOR);

  // Update brightness target
  new_target = read_knob(rgb->brightness_pin) * (BRIGHTNESS_MAX - BRIGHTNESS_MIN) + BRIGHTNESS_MIN;
  rgb->brightness_target = apply_hysteresis(new_target, rgb->brightness_target, BRIGHT_CHANGE_THRESHOLD);
  rgb->brightness_current = apply_smoothing(rgb->brightness_target, rgb->brightness_current, BRIGHT_SMOOTHING_FACTOR);
  // Ensure brightness is an integer
  // rgb->brightness_current = (int)rgb->brightness_current;
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
  // Update all parameters
  update_params(&red);
  update_params(&grn);
  update_params(&blu);

  uint32_t t = micros();
  for (int i = 0; i < NUM_PIXELS; i++) {
    uint8_t r = calc_color(&red, i, t);
    uint8_t g = calc_color(&grn, i, t);
    uint8_t b = calc_color(&blu, i, t);
    pixels.setPixelColor(i, pixels.Color(r,g,b));
  }
  pixels.show();

  // DEBUG info
  // NOTE: serial monitor must be connected or ESP will skip some led updates
  static uint32_t debug_timer = millis();
  if (millis() - debug_timer > 500) {
    debug_print_params();
    debug_timer = millis();
  }
}
