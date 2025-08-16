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
long weight_v;
long weight_h;

// Create an HX711 for each load cell and a Servo object
HX711 lc_v;
HX711 lc_h;
Servo servo;

// --- IMPORTANT: CALIBRATION VALUE ---
// This value is crucial for accurate readings.
// You need to determine it during a calibration process.
// It represents "how many units (e.g., grams) per reading step."
// Example: If putting 1000g on the lc_v changes the raw reading by 200000,
// then the lc_v factor would be 1000.0 / 200000.0 = 0.005.
// A common value for a 1kg load cell might be around -22000.0 to -24000.0
// but it will vary based on your specific load cell and HX711 gain.
// Read the "Calibration Process" section below!

// calibration vertical: 362, 382
// calibration horizontal: 4, 6, 26.5

float CALIBRATION_FACTOR_V = 362; // !!! REPLACE WITH YOUR CALIBRATED VALUE !!!
float CALIBRATION_FACTOR_H = 26.5; // !!! REPLACE WITH YOUR CALIBRATED VALUE !!!

void setup() {
  Serial.begin(9600); // Start serial communication for debugging

  // lc_v setup
  Serial.println("Initializing the lc_v...");
  Serial.print("DOUT pin: ");
  Serial.println(LOADCELL_DOUT_PIN_V);
  Serial.print("SCK pin: ");
  Serial.println(LOADCELL_SCK_PIN_V);

  // lc_h setup
  Serial.println("Initializing the lc_h...");
  Serial.print("DOUT pin: ");
  Serial.println(LOADCELL_DOUT_PIN_H);
  Serial.print("SCK pin: ");
  Serial.println(LOADCELL_SCK_PIN_H);

  // Servo connection
  servo.attach(SERVO_PIN);
  servo.write(angle);

  Serial.println("Wind Tunnel Program V2");
  Serial.println("Enter '1' to start the program.");

  // LOOP UNTIL VALID INPUT IS RECIEVED
  //    '1' enters the program loop, sets calibration, and zeroes the scales.
  while (true) {
    if (Serial.available() > 0) {
      char ch = Serial.read();
      if (ch == '1') {
        return;
      }
    }
  }
  Serial.println("Program has started. Currently looking for input.");
}

void loop() {
  // Handles all the incoming input from the C++ program
  if (Serial.available() > 0) {
    String keyword = Serial.readStringUntil(':');

    // Debugging the Serial input
    Serial.print("Reading in keyword of [");
    Serial.print(keyword);
    Serial.println("]");

    if (keyword == "angle") {
      int t_angle = Serial.parseInt();
      Serial.print("Changing the angle of attack from ");
      Serial.print(angle);
      Serial.print(" to ");
      Serial.print(t_angle);
      Serial.println(" degrees.");
      angle = t_angle;
      servo.write(angle);
    } else {
      Serial.println("Unrecognized keyword... Please check all incoming input.");
    }
  }
}
