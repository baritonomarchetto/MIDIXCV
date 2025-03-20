# MIDIXCV
A multi-outs (up to 32!) oct/V MIDI-to-CV converter for modular synthesizers.

The hardware is made of three boards: main board, analog board and digital board. 
Main board hosts MIDI in circuit, arduino nano (the brain of the converter), PSU input connector and an amplified output section. It handles 4 amplified analog outs with 12-bit resolution (4095) with two funciont: DAC out protection and octaves range extension (8 octaves). 
Analog board adds 4 amplified analog outputs to the equation. You can stack more than one analog board in order to increase the number of analog outs. 
Digital board hosts 4 (buffered) digital outputs. It is possible to increment outputs gain and increase the signal level up to 10V.

A calibration keypad has also been drawn in order to make outputs calibration easier.

This repository hosts PCB's Gerber files, BOMs, CPLs and arduino's sketches.

More info on the [dedicated Instructable](https://www.instructables.com/MIDIXCV-MIDI-to-Multiple-Control-Voltages-Converte/).

ACKNOWLEDGMENTS
Manufacturing of FR-4 PCBs, aluminum face-plate and SMD assembly where sponsored by [JLCPCB](https://jlcpcb.com/IAT).

JLCPCB is a high-tech manufacturer specialized in the production of high-reliable and cost-effective PCBs.They offer a flexible PCB assembly service with a huge library of more than 350.000 components in stock (included the MCP4728 needed for this project).

