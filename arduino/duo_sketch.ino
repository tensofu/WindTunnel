// This program utilizes Olav Kallhovd's HX711 ADC Library https://docs.arduino.cc/libraries/hx711_adc/

#include "HX711_ADC.h"
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
static float weight_v = 0.0f;
static float weight_h = 0.0f;
static String buffer;

// Create an HX711 for each load cell and a Servo object
HX711_ADC lc_v(LOADCELL_DOUT_PIN_V, LOADCELL_SCK_PIN_V);
HX711_ADC lc_h(LOADCELL_DOUT_PIN_H, LOADCELL_SCK_PIN_H);
Servo servo;

// calibration vertical: 362, 382
// calibration horizontal: 4, 6, 26.5

static float calibration_v = 712; // REPLACE WITH YOUR CALIBRATED VALUE 
static float calibration_h = 26.5; // REPLACE WITH YOUR CALIBRATED VALUE 

unsigned long t = 0;

void setup() {
  Serial.begin(115200); delay(10); // Start serial communication for debugging
  Serial.setTimeout(5); // Set a shorter timeout to avoid random 1s timeout 

  // Servo connection
  servo.attach(SERVO_PIN);
  servo.write(angle);

  // set up load cell
  lc_v.begin();
  lc_h.begin();

  unsigned long stabilizingtime = 2000;
  byte lc_v_rdy = 0;
  byte lc_h_rdy = 0;

  while ((lc_v_rdy + lc_h_rdy) < 2) { //run startup, stabilization and tare, both modules simultaniously
    if (!lc_v_rdy) lc_v_rdy = lc_v.startMultiple(stabilizingtime, true);
    if (!lc_h_rdy) lc_h_rdy = lc_h.startMultiple(stabilizingtime, true);
  }

  lc_v.setCalFactor(calibration_v); // user set calibration value (float)
  lc_h.setCalFactor(calibration_h); // user set calibration value (float)
}

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 5; //increase value to slow down serial print activity
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
      lc_v.setCalFactor(calibration_v);
    } else if (keyword == "calibration_h") { 
      float t_val = Serial.parseFloat();
      calibration_h = t_val;
      lc_h.setCalFactor(calibration_h);
    } else if (keyword == "tare") {
      lc_v.tareNoDelay();
      lc_h.tareNoDelay();
    } else if (keyword == "tare_v") {
      lc_v.tareNoDelay();
    } else if (keyword == "tare_h") {
      lc_h.tareNoDelay();
    } else if (keyword == "read") {
      lc_v.update();
      lc_h.update();

      weight_v = lc_v.getData(); 
      weight_h = lc_h.getData(); 
      buffer = String(weight_v, 2) + ":" + String(weight_h, 2);
      Serial.println(buffer);

      // serialFlush();
    } else {
      serialFlush();
    }
  }
}
