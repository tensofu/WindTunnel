# Wind Tunnel Program

This project was written to supplement the calculation and calibration of a wind tunnel mechanism via serial communication between a C++ and Arduino program. 
- The program aims to controls the angle of attack for a wing flap within the wind tunnel, calibrate the vertical/horizontal load cell components in real time, and to display readings from the load cells into a graph.

### Build Instructions
TODO: detail the requirements (cmake)

The build instructions are similar to building any other simple CMake project.
1. Clone the repository
2. Make a `build` directory and generate the Makefile. 
```bash
mkdir build
cd build
cmake ..
make
```
3. Run the exectuable.

TODO: detail dynamic linking of OpenGL
TODO: any changes within source code (setting the port and etc.)

### Overview

There are two distinct programs in this repository: 

1. Sketch that works as a sole Arduino program (located in `arduino/solo_sketch.ino`).
2. Sketch that works by communicating with a C++ program via the serial port and UART (rest of the repository + `arduino/duo_sketch.ino`).

The C++ program will provide a GUI that allows the variables within the Arduino program to easily be changed in real time. The idea is to have a slider or input box that could change the value, and send that change for the Arduino to process.

The messsage will be sent in strings of `{variable_name}:{int_value}` or `{variable_name}:{float_value}` (these are the two cases I am expecting to use). When attempting to change multiple variables at once, these strings are concatenated into a single buffer that is sent to the serial port.
- C++ makes use of the <cstdint> library to represent 16-bit integers via `int16_t`, as our board (Arduino UNO) reads in 16-bit integers (this ensures that we do not send integers that cannot be parsed correctly on the Arduino side).
- Floating-point values should be handled safely by default, assuming that platforms and architectures adhere to the [IEEE-754] standard.

### Requirements
1. CMake (version 3.10+)
2. OpenGL

### Static Dependencies
1. ImGui
2. SDL
3. Serial
4. ImPlot



