Teensy 3.x/4 PC Fan Controller
==============================
This project contains a PCB board created using Eagle PCB, an Arduino Sketch for the Teensy 3/4 boards and a simple Python script to communicate to the Teensy.   This project will allow you to read temperature probes and program 4-pin PC PWM Fans.   

Documentation
=============
This documenation is work-in-progress...   Much more to come.


Features and Compatibility
--------------------------
* Supports controlling 6 x 4-pin PWM PC Fans.
* Supports reading 6 x 10k Temp Probes.
* Communications to Teensy is done via USB (Serial)
* A basic Python script is included with an example yaml configuration file.  (More details to come...) 


The PCB Board
-------------
* The PCB board is created using Eagle PCB software.
* I increased the default gaps and trace sizes so that it would be possible to etch the board easily at home. (If you have experience doing so)
* It's compatible with the 'small' Teensy 3.x & 4.x boards.

The Arduino sketch
-----------------
* The sketch is compatible with Teensy 3.x & 4.x boards.
* Currently the Teensy runs in "manual" mode (Fans are not auto regulating).
* Future versions may support Fans auto regulating based on probe temperatures.

The Python FanController script
-------------------------------
* The script constantly reads the temperature probes and adjusts fans speeds based on ranges set in the yml config file.
* Currently it's linear, but future versions can use other calculations.
* Currently it gets temperatures from the FanController device only.  Future version may include hooks to get external temperatures (directly from the CPU for example).


