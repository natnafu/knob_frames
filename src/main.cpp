#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

#define SERIAL_STREAM_ENABLE 0
#define ENABLE_SERIAL_DEBUG 0

// Constants for the LED strip
#define NUM_STRIPS 7
#define LEDS_PER_STRIP 144
#define NUM_PIXELS (NUM_STRIPS * LEDS_PER_STRIP)

// Pin mapping for ESP32-S3 and ESP32
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  #define LED_PIN D6
  #define R1 A8  // red speed
  #define R2 A5  // green speed
  #define R3 A2  // blue speed
  #define R4 A9  // red wavelength
  #define R5 A4  // green wavelength
  #define R6 A1  // blue wavelength
  #define R7 A10 // red brightness
  #define R8 A3  // green brightness
  #define R9 A0  // blue brightness
  #define PIN_RED_SPEED       R1
  #define PIN_RED_WAVELN      R4
  #define PIN_RED_BRIGHTNESS  R7
  #define PIN_GRN_SPEED       R2
  #define PIN_GRN_WAVELN      R5
  #define PIN_GRN_BRIGHTNESS  R8
  #define PIN_BLU_SPEED       R3
  #define PIN_BLU_WAVELN      R6
  #define PIN_BLU_BRIGHTNESS  R9
#elif defined(CONFIG_IDF_TARGET_ESP32)
  #define LED_PIN 2
  #define R1 35  // red speed
  #define R2 34  // red wavelength
  #define R3 32  // red brightness
  #define R4 33  // green speed
  #define R5 27  // green wavelength
  #define R6 26  // green brightness
  #define R7 25  // blue speed
  #define R8 14  // blue wavelength
  #define R9 12  //
  #define PIN_RED_SPEED       R1
  #define PIN_RED_WAVELN      R2
  #define PIN_RED_BRIGHTNESS  R3
  #define PIN_GRN_SPEED       R4
  #define PIN_GRN_WAVELN      R5
  #define PIN_GRN_BRIGHTNESS  R6
  #define PIN_BLU_SPEED       R7
  #define PIN_BLU_WAVELN      R8
  #define PIN_BLU_BRIGHTNESS  R9
#endif

// knob min/max bit values
#define KNOB_MIN_VAL 0
#define KNOB_MAX_VAL 4095

// speed limits in units led/s
#define SPEED_MIN 0.0
#define SPEED_MAX 1.0
#define SPEED_CHANGE_THRESHOLD (0.02 * (SPEED_MAX - SPEED_MIN))

// wavelength limits in units leds
#define WAVELN_MIN 1
#define WAVELN_MAX 400.0
#define WAVELN_THRESHOLD (0.01 * (WAVELN_MAX - WAVELN_MIN))
#define WAVELN_SMOOTHING 1

#define BRIGHTNESS_MIN 0.0
#define BRIGHTNESS_MAX 255.0
#define BRIGHT_CHANGE_THRESHOLD (0.01 * (BRIGHTNESS_MAX - BRIGHTNESS_MIN))

struct color {
  // pot knobs
  int speed_pin;
  int waveln_pin;
  int brightness_pin;
  // raw wave parameters before filtering
  double waveln_raw;
  double speed_raw;
  double brightness_raw;
  // wave parameters after filtering
  double speed;
  double waveln;
  double brightness;
  // Current phase position
  double phase;
};

struct color red = {PIN_RED_SPEED, PIN_RED_WAVELN, PIN_RED_BRIGHTNESS};
struct color grn = {PIN_GRN_SPEED, PIN_GRN_WAVELN, PIN_GRN_BRIGHTNESS};
struct color blu = {PIN_BLU_SPEED, PIN_BLU_WAVELN, PIN_BLU_BRIGHTNESS};

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
void update_params(color* rgb) {
  // Update speed
  rgb->speed_raw = read_knob(rgb->speed_pin) * (SPEED_MAX - SPEED_MIN) + SPEED_MIN;
  rgb->speed = apply_hysteresis(rgb->speed_raw, rgb->speed, SPEED_CHANGE_THRESHOLD);

  // Update wavelength target
  // apply smoothing to wavelength changes since they can be very abrupt
  double new_waveln_raw = read_knob(rgb->waveln_pin) * (WAVELN_MAX - WAVELN_MIN) + WAVELN_MIN;
  if (new_waveln_raw < WAVELN_THRESHOLD) new_waveln_raw = WAVELN_MIN;
  rgb->waveln_raw = rgb->waveln_raw + (new_waveln_raw - rgb->waveln_raw) * WAVELN_SMOOTHING;
  rgb->waveln = apply_hysteresis(rgb->waveln_raw, rgb->waveln, WAVELN_THRESHOLD);

  // Update brightness target
  rgb->brightness_raw = read_knob(rgb->brightness_pin) * (BRIGHTNESS_MAX - BRIGHTNESS_MIN) + BRIGHTNESS_MIN;
  rgb->brightness = apply_hysteresis(rgb->brightness_raw, rgb->brightness, BRIGHT_CHANGE_THRESHOLD);
}

void debug_print_params() {
  Serial.println("DEBUG:");
  Serial.printf("red speed(%.6f) wave(%.6f) bright(%.6f)\n", red.speed ,red.waveln, red.brightness);
  Serial.printf("grn speed(%.6f) wave(%.6f) bright(%.6f)\n", grn.speed ,grn.waveln, grn.brightness);
  Serial.printf("blu speed(%.6f) wave(%.6f) bright(%.6f)\n", blu.speed ,blu.waveln, blu.brightness);
}

// calculate color value for a pixel based on its index and color parameters
uint8_t calc_color(color* rgb, int i) {
  double pos;
  if (rgb->waveln == 0) {
    // If wavelength is 0, just use the phase
    pos = rgb->phase;
  } else {
    pos = ((double) i / rgb->waveln) + rgb->phase;
  }

  // Apply sine wave transformation
  pos = 0.5 * (1.0 + sin(2 * PI * pos));
  // Apply brightness
  pos = pos * rgb->brightness;

  return pos;
}

void setup() {
  Serial.begin(921600);

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

  // Update phases based on speed
  red.phase += red.speed / 60.0; // Adjust divisor to control animation speed
  grn.phase += grn.speed / 60.0;
  blu.phase += blu.speed / 60.0;

#if SERIAL_STREAM_ENABLE
  // Start of LED data frame
  Serial.println("LED_DATA_START");
  Serial.printf("NUM_STRIPS:%d,LEDS_PER_STRIP:%d\n", NUM_STRIPS, LEDS_PER_STRIP);
#endif

  for (int i = 0; i < NUM_PIXELS; i++) {
    uint8_t r = calc_color(&red, i);
    uint8_t g = calc_color(&grn, i);
    uint8_t b = calc_color(&blu, i);
    pixels.setPixelColor(i, pixels.Color(r,g,b));

#if SERIAL_STREAM_ENABLE
    // Send LED color data over serial
    Serial.printf("LED:%d,%d,%d,%d\n", i, r, g, b);
#endif
  }

#if SERIAL_STREAM_ENABLE
  // End of LED data frame
  Serial.println("LED_DATA_END");
#endif

  // Update the LEDs
  pixels.show();

#if ENABLE_SERIAL_DEBUG
  // Also send the current parameters
  static uint32_t debug_timer = 0;
  if (millis() - debug_timer > 100) {
    debug_print_params();
    debug_timer = millis();
  }
#endif
}
