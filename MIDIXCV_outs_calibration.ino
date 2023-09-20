/*  MIDIXCV - DAC table calibration sketch
 *  
 *  This sketch can be used to calibrate MIDIXCV's main board voltage outs. 
 *  It allows to identify "correct" voltage indexes in the supported MIDI IN range.
 *  
 *  Values are shown in the serial monitor.
 *  
 *  An oscilloscope or a well tuned VCO are needed to monitor the actual voltage output.
 *  
 *  two buttons are used to increase and decrase MIDI note number.
 *  two buttons are used to increas and decrease voltage indexes.
 *  two buttons are used to increase and decrease voltage indexes faster.
 *  press note- while keeping pressed note+ for a full values log.
 *  
 *  Instructions
 *  - define the correct values in the whole octaves range
 *  - press log button (note-) while keeping pressed LOG button (note-) to print on the IDE's serial monitor a complete values log
 *  - copy and paste on MIDIXCV firmware ("cvIntRef[MAX_INT_V]" array).
 *  
 *  by Barito 2023 (last update sept 2023)
 */ 

#include <Wire.h>
#include "mcp4728.h"

mcp4728 dac0 = mcp4728(0); // initiate mcp4728 object, Device ID = 0

#define MAX_INT_V  97
#define MIDI_OFFSET 24

//SDA pin: A4
//SCL pin: A5
const byte LDAC0 = 2;//LDAC0
//const byte LDAC1 = 3;//LDAC1
const byte b1 = 4;//LDAC2 - NOTE+
const byte b2 = 5;//LDAC3 - VOLT+
const byte b3 = 6;//LDAC4 - VOLT++
const byte b4 = 7;//LDAC5 - NOTE-
const byte b5 = 8;//LDAC6 - VOLT-
const byte b6 = 9;//LDAC7 - VOLT--

bool b1_state;
bool b2_state;
bool b3_state;
bool b4_state;
bool b5_state;
bool b6_state;

int noteCount = 0;
int dTime = 200;

// DAC's V/semitone tabulated values (mV)
int cvIntRef[MAX_INT_V] = {
0, 45, 88, 129, 172, 214, 255, 299, 342, 384, 426, 469, 
509, 551, 594, 636, 678, 722, 763, 806, 849, 890, 932, 975, 
1016, 1058, 1101, 1143, 1186, 1229, 1271, 1313, 1356, 1397, 1440, 1483, 
1524, 1566, 1609, 1650, 1692, 1735, 1777, 1818, 1862, 1903, 1946, 1989, 
2030, 2071, 2114, 2155, 2197, 2240, 2283, 2326, 2371, 2412, 2455, 2498, 
2539, 2580, 2623, 2665, 2707, 2750, 2792, 2834, 2877, 2919, 2960, 3004, 
3045, 3086, 3130, 3172, 3214, 3258, 3300, 3342, 3386, 3428, 3470, 3513, 
3554, 3594, 3637, 3680, 3722, 3765, 3806, 3848, 3892, 3935, 3976, 4020, 
4061
}; //4095 MAX

//voltages you want to read on your multimeter (2X gain)
int out_mVolt[MAX_INT_V] = {
 0,  83, 167,  250,  333,  417,  500,  583,  667,  750,  833,  917,
 1000, 1083,  1167,  1250, 1333, 1417, 1500, 1583, 1667, 1750, 1833, 1917,
 2000, 2083,  2167,  2250, 2333, 2417, 2500, 2583, 2667, 2750, 2833, 2917,
 3000, 3083,  3167,  3250, 3333, 3417, 3500, 3583, 3667, 3750, 3833, 3917,
 4000, 4083,  4167,  4250, 4333, 4417, 4500, 4583, 4667, 4750, 4833, 4917,
 5000, 5083,  5167,  5250, 5333, 5417, 5500, 5583, 5667, 5750, 5833, 5917,
 6000, 6083,  6167,  6250, 6333, 6417, 6500, 6583, 6667, 6750, 6833, 6917,
 7000, 7083,  7167,  7250, 7333, 7417, 7500, 7583, 7667, 7750, 7833, 7917,
 8000
};

void setup() {
  //buttons initialization
  pinMode(b1, INPUT_PULLUP);
  pinMode(b2, INPUT_PULLUP);
  pinMode(b3, INPUT_PULLUP);
  pinMode(b4, INPUT_PULLUP);
  pinMode(b5, INPUT_PULLUP);
  pinMode(b6, INPUT_PULLUP);
  b1_state = digitalRead(b1);
  b2_state = digitalRead(b2);
  b3_state = digitalRead(b3);
  b4_state = digitalRead(b4);
  b5_state = digitalRead(b5);
  b6_state = digitalRead(b6);
  //DAC initialization
  pinMode(LDAC0, OUTPUT);
  digitalWrite(LDAC0, LOW);//activate latch pin
  dac0.begin();  // initialize i2c interface
  dac0.setPowerDown(0, 0, 0, 0); // set Power-Down ( 0 = Normal , 1-3 = shut down most channel circuit, no voltage out) (1 = 1K ohms to GND, 2 = 100K ohms to GND, 3 = 500K ohms to GND)
  dac0.setVref(1,1,1,1); // set to use internal voltage reference (2.048V) ( 0 = external, 1 = internal )
  dac0.setGain(1, 1, 1, 1); // set the gain of internal voltage reference ( 0 = gain x1, 1 = gain x2 )
  dac0.analogWrite(0, 0, 0, 0); //set all outs to zero
  Serial.begin(9600);//initialize serial transfer
  SerialLOG();
  DACsWrite();
}

void Main(){
  //NOTE+
  if(digitalRead(b1)!= b1_state){
    b1_state = !b1_state;
    if(b1_state == LOW){
      noteCount = noteCount+1;
      if(noteCount >= MAX_INT_V){//cycle back to the first note
        noteCount = 0;
      }
      SerialLOG();
      DACsWrite();
    }
    delay(dTime); //cheap debounce
  }
  //VOLT+
  else if(digitalRead(b2)!= b2_state){
    b2_state = !b2_state;
    if(b2_state == LOW){
      cvIntRef[noteCount] = cvIntRef[noteCount]+1;
      if(cvIntRef[noteCount]>=4095){
        cvIntRef[noteCount] = 4095;
      }
      SerialLOG();
      DACsWrite();
    }
    delay(dTime); //cheap debounce
  }
  //VOLT++
  else if(digitalRead(b3)!= b3_state){
    b3_state = !b3_state;
    if(b3_state == LOW){
      cvIntRef[noteCount] = cvIntRef[noteCount]+5;
      if(cvIntRef[noteCount]>=4095){
        cvIntRef[noteCount] = 4095;
      }
      SerialLOG();
      DACsWrite();
    }
    delay(dTime); //cheap debounce
  }
  //NOTE-
  else if(digitalRead(b4)!= b4_state){
    b4_state = !b4_state;
    if(b4_state == LOW){
      if(digitalRead(b1)==HIGH){//BUTTON NOTE+ NOT PRESSED
        noteCount = noteCount-1;
        if(noteCount <= 0){//cycle to the highest note
          noteCount = MAX_INT_V;
        }
        SerialLOG();
        DACsWrite();
      }
      else{//NOTE+ AND NOTE- PRESSED, WRITE FULL LOG
        FULL_LOG();
      }
    }
    delay(dTime); //cheap debounce
  }
  //VOLT-
  else if(digitalRead(b5)!= b5_state){
    b5_state = !b5_state;
    if(b5_state == LOW){
      cvIntRef[noteCount] = cvIntRef[noteCount]-1;
      if (cvIntRef[noteCount] <= 0){
        cvIntRef[noteCount] = 0;
      }
      SerialLOG();
      DACsWrite();
    }
    delay(dTime); //cheap debounce
  }
  //VOLT--
  else if(digitalRead(b6)!= b6_state){
    b6_state = !b6_state;
    if(b6_state == LOW){
      cvIntRef[noteCount] = cvIntRef[noteCount]-5;
      if (cvIntRef[noteCount] <= 0){
        cvIntRef[noteCount] = 0;//roll back to higher values
      }
      SerialLOG();
      DACsWrite();
    }
    delay(dTime); //cheap debounce
  }
}

void SerialLOG(){
  Serial.print("Note number: ");
  Serial.println(noteCount+MIDI_OFFSET);
  Serial.print("Voltage to read: ");
  Serial.println(out_mVolt[noteCount]);
  //Serial.println(" (-/+ 1mV)");
  //Serial.print("Voltage number: ");
  //Serial.println(cvIntRef[noteCount]);
}

void FULL_LOG(){
  Serial.println("");
  Serial.println("");
  for(int j=0; j<8; j++){
    for(int i=0; i< 12; i++){
      Serial.print(cvIntRef[i+j*12]);
      Serial.print(", ");
    }
    Serial.println("");
  }
  Serial.print(cvIntRef[MAX_INT_V-1]);
}

void DACsWrite(){
  dac0.analogWrite(cvIntRef[noteCount],cvIntRef[noteCount],cvIntRef[noteCount],cvIntRef[noteCount]); //set all outs to the new value
}  

void loop() {
   Main();
}
