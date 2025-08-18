#include "HX711.h"
#include "Servo.h"

// Define the pins for HX711 module
const int LOADCELL_DOUT_PIN_V = 2; // Vertical HX711 DOUT pin connected to Arduino pin 2
const int LOADCELL_SCK_PIN_V = 3;  // Vertical HX711 SCK pin connected to Arduino pin 3
const int LOADCELL_DOUT_PIN_H = 4; // Horizontal HX711 DOUT pin connected to Arduino pin 4
const int LOADCELL_SCK_PIN_H = 5;  // Horizontal HX711 SCK pin connected to Arduino pin 5
const int SERVO_PIN = 6;         // Servo pin is controlled to pin 6

// MISC VARIABLES FOR SERVO ADJUSTMENTS -> starts at 90
const int BASE_ANGLE = 90;
int angle = BASE_ANGLE;

// MISC VARIABLES FOR LOAD CELLS
static float weight_v = 10.439f;
static float weight_h = 10.432f;
static String buffer;

// Create an HX711 for each load cell and a Servo object
HX711 lc_v;
HX711 lc_h;
Servo servo;

// calibration vertical: 362, 382
// calibration horizontal: 4, 6, 26.5

static float calibration_v = 362; // REPLACE WITH YOUR CALIBRATED VALUE 
static float calibration_h = 26.5; // REPLACE WITH YOUR CALIBRATED VALUE 

void setup() {
  Serial.begin(115200); // Start serial communication for debugging
  Serial.setTimeout(5); // Set a shorter timeout to avoid random 1s timeout 

  // Servo connection
  servo.attach(SERVO_PIN);
  servo.write(angle);

  // set up load cell
  lc_v.begin(LOADCELL_DOUT_PIN_V, LOADCELL_SCK_PIN_V);
  lc_h.begin(LOADCELL_DOUT_PIN_H, LOADCELL_SCK_PIN_H);

  lc_v.set_scale(calibration_v);
  lc_h.set_scale(calibration_h);

  lc_v.tare();
  lc_h.tare();
}

void loop() {
  // Sends results
  if (Serial.available() > 0) {
    String keyword = Serial.readStringUntil(':');

    if (keyword == "angle") {
      int t_angle = Serial.parseInt();
      angle = t_angle;
      servo.write(angle);
    } else if (keyword == "calibration_v") { 
      float t_val = Serial.parseFloat();
      calibration_v = t_val;
      lc_v.set_scale(t_val);
    } else if (keyword == "calibration_h") { 
      float t_val = Serial.parseFloat();
      calibration_h = t_val;
      lc_h.set_scale(t_val);
    } else if (keyword == "tare") {
      Serial.parseInt();
      lc_v.tare();
      lc_h.tare();
    } else if (keyword == "tare_v") {
      Serial.parseInt();
      lc_v.tare();
    } else if (keyword == "tare_h") {
      Serial.parseInt();
      lc_h.tare();
    } else if (keyword == "read") {
      Serial.parseInt();
      weight_v = lc_v.get_units(1); 
      weight_h = lc_h.get_units(1); 
      buffer = String(weight_v, 2) + ":" + String(weight_h, 2);
      Serial.println(buffer);
    } else {
      Serial.readString();
    }
  }
}
