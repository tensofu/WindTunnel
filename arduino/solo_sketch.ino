#include "HX711.h"
#include "Servo.h"

// Define the pins for HX711 module
const int LOADCELL_DOUT_PIN_V = 2; // Vertical HX711 DOUT pin connected to Arduino pin 2
const int LOADCELL_SCK_PIN_V = 3;  // Vertical HX711 SCK pin connected to Arduino pin 3
const int LOADCELL_DOUT_PIN_H = 4; // Horizontal HX711 DOUT pin connected to Arduino pin 4
const int LOADCELL_SCK_PIN_H = 5;  // Horizontal HX711 SCK pin connected to Arduino pin 5
const int SERVO_PIN = 6;         // Servo pin is controlled to pin 6

// MISC VARIABLES FOR SERVO ADJUSTMENTS -> starts at 90, changes angle by MARGIN
const int MARGIN = 1;
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
const float TARGET_GRAMS = 2.5;

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

  // // Initialize the lc_v loadcell with the DOUT and SCK pins
  // // You can also specify the gain factor (128 is default and usually best for load cells)
  lc_v.begin(LOADCELL_DOUT_PIN_V, LOADCELL_SCK_PIN_V);
  lc_h.begin(LOADCELL_DOUT_PIN_H, LOADCELL_SCK_PIN_H);

  // Check if the lc_v is ready
  if (lc_v.is_ready()) {
    Serial.println("lc_v is ready!");
  } else {
    Serial.println("lc_v not found. Please check wiring.");
    Serial.println("Execution has been halted.");
    while(true); // Halt execution if lc_v is not found
  }

  // Check if the lc_H is ready
  if (lc_h.is_ready()) {
    Serial.println("lc_h is ready!");
  } else {
    Serial.println("lc_h not found. Please check wiring.");
    Serial.println("Execution has been halted.");
    while(true); // Halt execution if lc_h is not found
  }

  Serial.println("Wind Tunnel Program");
  Serial.println("Enter '1' to start the program and zero out the mechanism.");
  Serial.println("Enter '2' to set the servo's angle of attack.");

  // LOOP UNTIL VALID INPUT IS RECIEVED
  //    '1' enters the program loop, sets calibration, and zeroes the scales.
  while (true) {
    if (Serial.available() > 0) {
      char ch = Serial.read();

      if (ch == '1') {
        Serial.print("Zeroing out the lc_v using a calibration factor of ");
        Serial.println(CALIBRATION_FACTOR_V);
        lc_v.set_scale(CALIBRATION_FACTOR_V);
        lc_v.tare();

        Serial.print("Zeroing out the lc_h using a calibration factor of ");
        Serial.println(CALIBRATION_FACTOR_H);
        lc_h.set_scale(CALIBRATION_FACTOR_H);
        lc_h.tare();
        return;
      }

      

      switch (ch) {   
        case '2':
          Serial.println("Setting the servo's angle. Use the keys 'a' and 'd' to change the angle. Press 'q' to quit.");
          while (true) {
            if (Serial.available() > 0) {
              char input = Serial.read();
              if (input == 'q') {
                Serial.println("Exiting the loop...");
                break;
              }
              switch (input) {
                case 'a':
                  angle -= MARGIN;
                  servo.write(angle);
                  Serial.print("The angle of the servo is now: ");
                  Serial.println(angle);
                  break;
                case 'd': 
                  angle += MARGIN;
                  servo.write(angle);
                  Serial.print("The angle of the servo is now: ");
                  Serial.println(angle);
                  break;
                default:
                  break;
              }
            }
          }
          break;

        default:
          Serial.println("Invalid input. Please enter any of the valid input as instructed.");
      }
    }
  }
}

void loop() {
  // Read the average of several readings to get a more stable value
  // lc_v.get_units(num_readings) returns the weight in your defined units
  // after applying the calibration factor and tare offset.
  float weight_v = lc_v.get_units(10); // Take 10 readings and average them
  float weight_h = lc_h.get_units(10); // Take 10 readings and average them
  // Serial.println(lc_v.read_average(10));

  // Prints the vertical reading
  Serial.print("VERTICAL: ");
  Serial.print(weight_v); // Print weight with 2 decimal places
  Serial.println(" grams"); // Or "kg", "lbs", etc., depending on your calibration

  // Prints the horizontal reading
  Serial.print("HORIZONTAL: ");
  Serial.print(weight_h); // Print weight with 2 decimal places
  Serial.println(" grams"); // Or "kg", "lbs", etc., depending on your calibration

  // Optional: Re-tare if a button is pressed or at certain times
  if (Serial.available() > 0) {
    char ch = Serial.read();
    if (ch == 't') {
      Serial.println("Zeroing out both the scales...");
      lc_v.tare();
      lc_h.tare();
      Serial.println("Re-tared.");
    } else if (ch == 's') {
      CALIBRATION_FACTOR_V -= 1;
      Serial.print("Adjusting the vertical scale using a calibration factor of ");
      Serial.println(CALIBRATION_FACTOR_V);
      lc_v.set_scale(CALIBRATION_FACTOR_V);
    } else if (ch == 'w') {
      CALIBRATION_FACTOR_V += 1;
      Serial.print("Adjusting the vertical scale using a calibration factor of ");
      Serial.println(CALIBRATION_FACTOR_V);
      lc_v.set_scale(CALIBRATION_FACTOR_V);
    } else if (ch == 'a') {
      CALIBRATION_FACTOR_V -= 1;
      Serial.print("Adjusting the horizontal scale using a calibration factor of ");
      Serial.println(CALIBRATION_FACTOR_H);
      lc_h.set_scale(CALIBRATION_FACTOR_H);
    } else if (ch == 'd') {
      CALIBRATION_FACTOR_V += 1;
      Serial.print("Adjusting the horizontal scale using a calibration factor of ");
      Serial.println(CALIBRATION_FACTOR_H);
      lc_h.set_scale(CALIBRATION_FACTOR_H);
    } else if (ch == 'j') {
      angle -= MARGIN;
      Serial.print("Adjusting the angle of attack to: ");
      Serial.println(angle);
      servo.write(angle);
    } else if (ch == 'l') {
      angle += MARGIN;
      Serial.print("Adjusting the angle of attack to: ");
      Serial.println(angle);
      servo.write(angle);
    }
  }
}
