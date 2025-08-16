# Wind Tunnel Program

An arduino program that lets the user set the angle of attack and calibrate each load cell (vertical and horizontal) of the tunnel during calibration.

This will be in two sketches: 

1. Sketch that works as a sole Arduino program
2. Sketch that works by communicating with a C++ program via Serial port and UART.

The C++ program will create a GUI that allows the variables within the Arduino program to easily be changed. The idea is to have a slider or input box that could change the value, and send that change for the Arduino to process.

The messsage will be sent in this way: `{variable_name}:{int_value}` or `{variable_name}:{float_value}` (these are the two cases I am expecting to use)
- Note that C++ should make use of the <cstdint> library to represent integers as `int16_t`, as Arduino Uno reads in 16-bit integers.
