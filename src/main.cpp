#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

#define NUM_PIXELS 432
#define LED_PIN D6

// Knob to Pin mapping
#define R1 A8
#define R2 A5
#define R3 A2
#define R4 A9
#define R5 A4
#define R6 A1
#define R7 A10
#define R8 A3
#define R9 A0

// knob min/max bit values
#define KNOB_MIN_VAL 0
#define KNOB_MAX_VAL 4095
#define KNOB_MID_VAL ((KNOB_MAX_VAL - KNOB_MIN_VAL) / 2)

// Smoothing factors (0.0 to 1.0)
// Lower values = smoother transitions but slower response
// Higher values = faster response but less smooth
#define SPEED_SMOOTHING_FACTOR 0.01     // Faster response for speed
#define WAVELN_SMOOTHING_FACTOR 0.5    // Slower transitions for wavelength
#define BRIGHTNESS_SMOOTHING_FACTOR 1 // Medium speed for brightness

#define SPEED_ZERO_RANGE 100 // range around mid knob value that is considered 0

// speed limits in units led/s
#define SPEED_MIN 0
#define SPEED_MAX 1
#define SPEED_CHANGE_THRESHOLD 0.05 // only update param if changes by this fraction of max

// wavelength limits in units leds
#define WAVELN_MIN 0
#define WAVELN_MAX 400
#define WAVELN_THRESHOLD 1 // only change by whole number of leds

#define BRIGHTNESS_MAX 1
#define BRIGHT_CHANGE_THRESHOLD 0.05 // only update param if changes by this fraction of max


struct color {
  double speed;
  double target_speed;
  double waveln;
  double target_waveln;
  double brightness;
  double target_brightness;
  double phase_us;
  uint32_t target_speed_changed_us; // saves micros() when speed is changed, 0 if no new change
  double last_pos_offset; // Used for smooth phase transitions
} red, grn, blu;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Map the linear value to an exponential scale
float mapToExponential(float x, float in_min, float in_max, float out_min, float out_max) {
  // Map the linear value to a normalized range [0, 1]
  float normalizedValue = (x - in_min) / (in_max - in_min);
  // Map the normalized value to the exponential scale
  float exponentialValue = out_min + (out_max - out_min) * pow(normalizedValue, 2);
  return exponentialValue;
}

// invert knob value so value increases when turned clockwise
double read_knob(int knob_pin) {
  return KNOB_MAX_VAL - analogRead(knob_pin);
}

// calculates speed based on a knob position.
// the sign of the speed indicates direction.
// returns a value that is between -1.0 and 1.0
double calc_speed(int knob_pin, double last_target_speed) {
  // shift knob value so range is +/- the middle value
  double val = read_knob(knob_pin) - KNOB_MID_VAL;

  // if val is within SPEED_ZERO_RANGE of 0, consider the value 0
  if (abs(val) < SPEED_ZERO_RANGE) val = 0;

  // convert to speed
  val = (SPEED_MAX *(val / KNOB_MID_VAL));

  // if value hasn't changed by enough, keep speed the same
  if (abs(val - last_target_speed) < (SPEED_MAX * SPEED_CHANGE_THRESHOLD)) return last_target_speed;

  return val;
}

// calculates wave length based on a knob position.
// returns a value between 0 and WAVELEN_MAX
double calc_waveln(int knob_pin, double last_target_waveln) {
  double val = WAVELN_MAX * (read_knob(knob_pin) / KNOB_MAX_VAL);
  if (val == 0) return val; // return 0, no further math needed

  // Map exponentially to get smaller wavelenghts easier
  val = mapToExponential(val, 0, WAVELN_MAX, 0, WAVELN_MAX);

  // make it easier to have a wavelength of 1
  if (val < 2) val = 1;

  // if value hasn't changed by enough, keep wavelength the same
  if (abs(val - last_target_waveln) < WAVELN_THRESHOLD) return last_target_waveln;

  return val;
}

// calculates the brightness based on a knob position.
// returns a value between 0.0 and 1.0
double calc_brightness(int knob_pin, double last_target_brightness) {
  double val = BRIGHTNESS_MAX * read_knob(knob_pin) / KNOB_MAX_VAL;
  if (val == 0) return val; // return 0, no further math needed

  // Map exponentially to match how our eyes perceive brightness
  val = mapToExponential(val, 0, BRIGHTNESS_MAX, 0, BRIGHTNESS_MAX);

  // if value hasn't changed by enough, keep brightness the same
  if (abs(val - last_target_brightness) < (BRIGHTNESS_MAX * BRIGHT_CHANGE_THRESHOLD)) return last_target_brightness;

  return val;
}

// Smoothly transition from current value to target value
double smooth_value(double current, double target, double smoothing_factor) {
  return current + (target - current) * smoothing_factor;
}

// Updates color params based on knobs
void update_params() {
  // Update target values based on knob positions

  // Update speed target and note if target changed to adjust phase for continuity
  double old_target = red.target_speed;
  red.target_speed = calc_speed(R1, red.target_speed);
  if (old_target != red.target_speed) {
    red.target_speed_changed_us = micros();
  }
  old_target = grn.target_speed;
  grn.target_speed = calc_speed(R2, grn.target_speed);
  if (old_target != grn.target_speed) {
    grn.target_speed_changed_us = micros();
  }
  old_target = blu.target_speed;
  blu.target_speed = calc_speed(R3, blu.target_speed);
  if (old_target != blu.target_speed) {
    blu.target_speed_changed_us = micros();
  }

  // Update wavelength target
  red.target_waveln = calc_waveln(R4, red.target_waveln);
  grn.target_waveln = calc_waveln(R5, grn.target_waveln);
  blu.target_waveln = calc_waveln(R6, blu.target_waveln);

  // Update brightness target
  red.target_brightness = calc_brightness(R7, red.target_brightness);
  grn.target_brightness = calc_brightness(R8, grn.target_brightness);
  blu.target_brightness = calc_brightness(R9, blu.target_brightness);

  // Smoothly transition current values toward target values
  red.speed = smooth_value(red.speed, red.target_speed, SPEED_SMOOTHING_FACTOR);
  grn.speed = smooth_value(grn.speed, grn.target_speed, SPEED_SMOOTHING_FACTOR);
  blu.speed = smooth_value(blu.speed, blu.target_speed, SPEED_SMOOTHING_FACTOR);
  red.waveln = smooth_value(red.waveln, red.target_waveln, WAVELN_SMOOTHING_FACTOR);
  grn.waveln = smooth_value(grn.waveln, grn.target_waveln, WAVELN_SMOOTHING_FACTOR);
  blu.waveln = smooth_value(blu.waveln, blu.target_waveln, WAVELN_SMOOTHING_FACTOR);
  red.brightness = smooth_value(red.brightness, red.target_brightness, BRIGHTNESS_SMOOTHING_FACTOR);
  grn.brightness = smooth_value(grn.brightness, grn.target_brightness, BRIGHTNESS_SMOOTHING_FACTOR);
  blu.brightness = smooth_value(blu.brightness, blu.target_brightness, BRIGHTNESS_SMOOTHING_FACTOR);
}

void debug_print_params() {
  Serial.println("color speed wave bright phase_us");
  Serial.printf("red %.6f %.6f %.6f %.6f\n", red.speed, red.waveln, red.brightness, red.phase_us);
  Serial.printf("grn %.6f %.6f %.6f %.6f\n", grn.speed, grn.waveln, grn.brightness, grn.phase_us);
  Serial.printf("blu %.6f %.6f %.6f %.6f\n", blu.speed, blu.waveln, blu.brightness, blu.phase_us);
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

  // Initialize all values to 0
  red.speed = red.target_speed = 0;
  grn.speed = grn.target_speed = 0;
  blu.speed = blu.target_speed = 0;

  red.waveln = red.target_waveln = 0;
  grn.waveln = grn.target_waveln = 0;
  blu.waveln = blu.target_waveln = 0;

  red.brightness = red.target_brightness = 0;
  grn.brightness = grn.target_brightness = 0;
  blu.brightness = blu.target_brightness = 0;

  red.last_pos_offset = grn.last_pos_offset = blu.last_pos_offset = 0;
}

uint8_t calc_color(color* rgb, int i, uint32_t t) {
  // Handle phase adjustment when speed changes
  if (rgb->target_speed_changed_us) {
    // Adjust phase to maintain visual continuity
    rgb->phase_us += t - rgb->target_speed_changed_us;
    rgb->target_speed_changed_us = 0;
  }

  // Calculate position based on wavelength and speed
  double pos;
  if (rgb->waveln == 0) {
    pos = rgb->speed * (t - rgb->phase_us) / 1000000.0;
  } else {
    pos = ((double) i / rgb->waveln) + (rgb->speed * (t - rgb->phase_us) / 1000000.0);
  }

  // Apply sine wave transformation
  pos = 0.5 * (1.0 + sin(2 * PI * pos));
  // Apply brightness
  pos = pos * 255 * rgb->brightness;

  return pos;
}

void loop() {
  update_params();

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
