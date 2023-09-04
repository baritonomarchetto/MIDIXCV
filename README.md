# MIDIXCV
A multi-outs (up to 32!) MIDI-to-CV converter for modular synthesizers.

The hardware is made of three boards: main board, analog board and digital board. 
Main board hosts MIDI in circuit, arduino nano (the brain of the converter), and PSU input connector. It handles 4 amplified analog outs with 12-bit resolution (4095). 
Analog board adds 4 amplified analog outputs to the equation. You can stack more than one analog boards in order to increase the number of analog outs. 
Digital board hosts 4 (buffered) digital outputs. It is possible to increment outputs gain and increase the signal level up to 10V.
