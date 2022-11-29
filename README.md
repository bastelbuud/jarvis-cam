# jarvis-cam
The projects consist of an esp32cam module mounted on a pan/tilt stand and a nextion display connected to a raspberry pi pico w.

The esp32cam contains the code to send a still image in different resolutions via an HTTP request. It also accepts mqtt messages to drive the pan and tilt motor, 
as well as to switch on and off the onboard led.

The nextion display provides 2 sliders for the control of the motors and a switch for the led. The values are send via a serial connection to the rasperry pi pico.

The pico publishs the values from the sliders and the switch via mqtt to the mqqt broker. The esp32 has subscribed to the topics, and so gets the values
to drive the motors and switch the  led on and off

This repo includes different files in different folders

jarvis-cam contains the arduino project to install on the esp32 cam module

nextion-pico contains the files to install on the nextion display and the picow to send the commands to drive the pan/tilt motors
