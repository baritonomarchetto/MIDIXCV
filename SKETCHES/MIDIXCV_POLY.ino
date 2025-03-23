/* 
 *    MIDXCV - arduino MIDI-to-CV converter
 *
 *    POLYPHONIC CONFIGURATION, with dynamic voice reallocation
 *    
 *    Translates MIDI messages into multiple voltages (pitch control, gates, aftertoutch, keyboard tracking etc).
 *    Main board hardware built around an arduino nano and a quad,I2C 12-bit DAC (MCP4728).   
 *    Analog board hardware use a quad,I2C 12-bit DAC (MCP4728).
 *    
 *    Up to 32 analog outputs (control voltages) are possible.
 *    
 *    Every DAC (exept main board) has to be reprogrammed to a different address.
 *    
 *    Libraries needed for normal operation:
 *    Francois Best's MIDI library https://github.com/FortySevenEffects/arduino_midi_library
 *    Benoit Shillings's MCP4728 library https://github.com/BenoitSchillings/mcp4728
 *    
 *    Softwares needed for MPUs adress change:
 *    TrippyLighting's SoftI2cMaster library https://github.com/TrippyLighting/SoftI2cMaster
 *    Jan Knipper's MCP4728 program address sketch https://github.com/jknipper/mcp4728_program_address
 *    
 *    This sketch handles:
 *    - 4x pitches (main board, DAC0)
 *    - 4x auxiliary CVs (analog board, DAC1): unused (was used as "gate" in parafonic config.), MIDI velocity, MIDI note height (for keyboard tracking), aftertouch
 *    - 4x gates (polyphonic configuration)
 *
 *    by Barito, 2023 (last update mar '25)
 */
 
#include <MIDI.h>
#include <Wire.h>
#include "mcp4728.h"

mcp4728 dac0 = mcp4728(0); // initiate mcp4728 object, Device ID = 0. Used for VCO PITCHES
mcp4728 dac1 = mcp4728(1); // initiate mcp4728 object, Device ID = 1. Used for CONTROL VOLTAGES

#define MAX_VOICES  4
#define MAX_INT_V  97
#define MIDI_CHANNEL 1
#define MIDIOFFSET 24
#define DIPS 3

#define VEL 1 //VELOCITY
#define KBTRACK 2 //KEYBOARD TRACKING
#define ATOUCH 3 //AFTERTOUCH
#define OPEN 2500 //digital outs software-limited to 5V circa
//#define LIMIT_CV //analog AUX outputs voltage limited to 5V max

//SDA pin: A4
//SCL pin: A5
const byte LDAC0 = 2;
//const byte LDAC1 = 3;
//const byte LDAC2 = 4;
//const byte LDAC3 = 5;
const byte gate_pin[MAX_VOICES] = {6, 7, 8, 9}; //DIGITAL BOARD OUT W, X, Y, Z
//DIP0: MIDI on/off (for new sketch upload)
const byte DIP_pin[DIPS] = {10, 11, 12}; //UNUSED
bool DIP_state[DIPS];
bool gate_state[MAX_VOICES];

MIDI_CREATE_DEFAULT_INSTANCE();

int noteCount;
byte lowestNote;
byte highestNote;
int activeSlot;
int pitchbend = 0;
int noteOverflow;
int velVal;
int kbtVal;
int ATVoltage;
bool sustain;
bool firstCount;

byte noteMem[MAX_VOICES];
bool busy[MAX_VOICES];

// DAC's V/semitone tabulated values (mV), output #1
const int cv0IntRef[MAX_INT_V] = {
0, 44, 88, 130, 172, 215, 257, 299, 342, 384, 426, 468, 
508, 550, 592, 633, 675, 718, 759, 801, 843, 884, 926, 968, 
1007, 1049, 1091, 1133, 1174, 1218, 1259, 1301, 1344, 1385, 1426, 1468, 
1510, 1551, 1593, 1635, 1677, 1720, 1762, 1803, 1846, 1887, 1929, 1971, 
2012, 2053, 2095, 2137, 2179, 2221, 2263, 2304, 2347, 2389, 2431, 2473, 
2515, 2554, 2596, 2638, 2680, 2722, 2763, 2805, 2847, 2889, 2931, 2973, 
3015, 3056, 3097, 3140, 3181, 3223, 3265, 3307, 3349, 3392, 3434, 3477, 
3519, 3560, 3601, 3643, 3685, 3727, 3770, 3812, 3854, 3896, 3938, 3981, 
4022
}; //4095 MAX

// DAC's V/semitone tabulated values (mV), output #2
const int cv1IntRef[MAX_INT_V] = {
0, 40, 82, 125, 167, 210, 258, 295, 338, 381, 423, 465, 
505, 548, 590, 633, 675, 718, 760, 801, 843, 885, 928, 970, 
1011, 1053, 1097, 1139, 1181, 1225, 1267, 1310, 1353, 1395, 1438, 1480, 
1522, 1564, 1608, 1650, 1692, 1735, 1777, 1819, 1862, 1905, 1947, 1990, 
2031, 2073, 2115, 2158, 2201, 2243, 2286, 2328, 2371, 2413, 2455, 2498, 
2539, 2581, 2624, 2666, 2709, 2752, 2795, 2837, 2879, 2921, 2963, 3006, 
3047, 3089, 3131, 3173, 3215, 3258, 3301, 3343, 3385, 3427, 3469, 3512, 
3553, 3595, 3637, 3680, 3722, 3765, 3807, 3849, 3892, 3935, 3977, 4019, 
4061
}; //4095 MAX

// DAC's V/semitone tabulated values (mV), output #3
const int cv2IntRef[MAX_INT_V] = {
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

// DAC's V/semitone tabulated values (mV), output #4
const int cv3IntRef[MAX_INT_V] = {
0, 35, 73, 115, 157, 200, 241, 283, 325, 366, 408, 450, 
491, 533, 576, 618, 660, 703, 744, 786, 829, 871, 913, 956, 
996, 1037, 1079, 1121, 1163, 1206, 1248, 1289, 1332, 1374, 1416, 1459, 
1501, 1542, 1585, 1626, 1668, 1711, 1753, 1794, 1837, 1879, 1921, 1964, 
2006, 2046, 2089, 2131, 2173, 2215, 2257, 2298, 2340, 2381, 2423, 2465, 
2507, 2547, 2589, 2630, 2672, 2713, 2755, 2797, 2839, 2882, 2923, 2965, 
3006, 3047, 3088, 3130, 3172, 3215, 3257, 3299, 3342, 3385, 3426, 3468, 
3510, 3551, 3593, 3635, 3676, 3719, 3761, 3803, 3845, 3887, 3929, 3972, 
4014
}; //4095 MAX

void setup(){
 //DIP switches
    for (int a = 0; a < DIPS; a++){
      pinMode(DIP_pin, INPUT_PULLUP);
      DIP_state[a] = digitalRead(DIP_pin[a]);
    }
 //DIGITAL OUTS (4X GATES)
    for (int a = 0; a < MAX_VOICES; a++){
      pinMode(gate_pin[a], OUTPUT);
      digitalWrite(gate_pin[a], LOW);
      gate_state[a] = LOW;
    }
 //MIDI initialization
    MIDI.setHandleNoteOn(HandleNoteOn);
    MIDI.setHandleNoteOff(HandleNoteOff);
    MIDI.setHandlePitchBend(HandlePitchBend);
    MIDI.setHandleAfterTouchChannel(HandleAfterTouch);
    MIDI.setHandleControlChange(HandleControlChange);
    MIDI.begin(MIDI_CHANNEL);
 //DAC initialization
    pinMode(LDAC0, OUTPUT); //VOICES
    //pinMode(LDAC1, OUTPUT); //AUX CVs
    digitalWrite(LDAC0, LOW);
    //digitalWrite(LDAC1, LOW);
    dac0.begin();  // initialize i2c interface
    dac1.begin();  // initialize i2c interface
    //dac.vdd(5000); // set VDD(mV) of MCP4728 for correct conversion between LSB and Vout
    dac0.setPowerDown(0, 0, 0, 0); // set Power-Down ( 0 = Normal , 1-3 = shut down most channel circuit, no voltage out) (1 = 1K ohms to GND, 2 = 100K ohms to GND, 3 = 500K ohms to GND)
    dac1.setPowerDown(0, 0, 0, 0);
    dac0.setVref(1,1,1,1); // set to use internal voltage reference (2.048V) ( 0 = external, 1 = internal )
    dac1.setVref(1,1,1,1);
    dac0.setGain(1, 1, 1, 1); // set the gain of internal voltage reference ( 0 = gain x1, 1 = gain x2 )
    dac1.setGain(1, 1, 1, 1);
    dac0.analogWrite(cv0IntRef[40], cv1IntRef[40], cv2IntRef[40], cv3IntRef[40]); //set pitches to half note circa
    dac1.analogWrite(0, 0, 0, 0); //set all outs to zero
 //OTHER initializations
    noteCount = 0; //initialize note counting
}

void HandleNoteOn(byte channel, byte note, byte velocity) {
  noteCount++;
  note = note-MIDIOFFSET;
  if(note < MAX_INT_V && note>=0){
    switch(noteCount){
      case 0:
        //do nothing (not going to happen on "note on" signal)
      break;
      case 1: //first key press - note allocated to 2x voices (not more voices or the volume drop during reallocation will be sensible)
      firstCount=!firstCount; //this is to allow a decent release to two adiacent notes when playing a single note at a time
      if(firstCount == 0){
        AllocVoice(note);
        AllocVoice(note);
      }
      else{
        busy[0] = true;
        busy[1] = true;
        AllocVoice(note);
        AllocVoice(note);
        busy[0] = false;
        busy[1] = false;
      }
      break;
      case 2: //new note allocated to 2x voices
        AllocVoice(note);
        AllocVoice(note);
      break;
      case 3:  //third voice allocated to 2x voice
        FreeVoice(); //go, find a doubled voice and free it
        AllocVoice(note);
        FreeVoice(); //go, find another doubled voice and free it
        AllocVoice(note);
      break;
      case 4: //fourth voice allocated to 1x voice
        FreeVoice(); //go, find a doubled voice and free it
        AllocVoice(note);
        NoteHighest();
      break;
      default: //POLYPHONY EXCEEDED (MAX_VOICES+ notes)
        for(int i = 0; i < MAX_VOICES; i++){ //search for the HIGHEST pitch and steal the note for the 4+ note
          if (noteMem[i] == highestNote){ 
            noteMem[i] = note;
            busy[i] = true;
            digitalWrite(gate_pin[i], HIGH);
            NoteHighest();
          }
        }
      break;
    }
    dac0.analogWrite(cv0IntRef[noteMem[0]] + pitchbend, cv1IntRef[noteMem[1]] + pitchbend, cv2IntRef[noteMem[2]] + pitchbend, cv3IntRef[noteMem[3]] + pitchbend);
    Set_vel_kbt(velocity, note);
  }
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
  if(sustain == 0){
    noteCount--;
    if(noteCount<0){
      noteCount = 0;
    }
    note = note-MIDIOFFSET;
    if(note < MAX_INT_V && note>=0){
      for (int a = 0; a < MAX_VOICES; a++){ //define which voice is going and needs reallocation
        if (noteMem[a] == note){
          busy[a] = false;
          digitalWrite(gate_pin[a], LOW); //close voice gate
        }
      }
    }
  }
}

void AllocVoice(byte n){
  for (int a = 0; a< MAX_VOICES; a++){
    if(busy[a] == false){
      noteMem[a] = n;
      busy[a] = true;
      digitalWrite(gate_pin[a], HIGH);
      return;
    }
  }
}

void FreeVoice(){
  for (int a = 0; a< MAX_VOICES; a++){
    for (int b = 0; b< MAX_VOICES-1; b++){
      if(a != b && noteMem[a] == noteMem[b]){
        busy[a] = false;
        return;
      }
    }
  }
}

void Set_vel_kbt(byte v, byte n){
  #ifdef LIMIT_CV
    velVal = v << 3;//note velocity (255->2040)
    kbtVal = n << 3;//keyboard tracking (255->2040)
  #else
    velVal = v << 4;//note velocity (255->4080)
    kbtVal = n << 4;//keyboard tracking (255->4080)
  #endif
  dac1.analogWrite(0, velVal, kbtVal, ATVoltage);//(unused), velocity, keyboard tracking, aftertouch
}

void HandlePitchBend(byte channel, int bend){
  pitchbend = bend>>6;
  dac0.analogWrite(cv0IntRef[noteMem[0]] + pitchbend, cv1IntRef[noteMem[1]] + pitchbend, cv2IntRef[noteMem[2]] + pitchbend, cv3IntRef[noteMem[3]] + pitchbend);
}

void HandleAfterTouch(byte channel, byte pressure){
  #ifdef LIMIT_CV
    ATVoltage = pressure << 3;// 255->2040
  #else
    ATVoltage = pressure << 4;// 255->4080
  #endif
  dac1.analogWrite(ATOUCH, ATVoltage);
}

void HandleControlChange(byte channel, byte number, byte value){
  switch(number){
    case 64: //SUSTAIN
      if(value > 0){//this could be "64" depending on the hardware
        sustain = 1;
      }
      else{
        sustain = 0;
        for (int a = 0; a < MAX_VOICES; a++){
          busy[a] = false;
          digitalWrite(gate_pin[a], LOW); //close voices gates
        }
        noteCount = 0;
      }
    break;
  }
}

void NoteLowest(){
  lowestNote = MAX_INT_V;//reset
  for (int a = 0; a < MAX_VOICES; a++){
    if(noteMem[a] < lowestNote){
       lowestNote = noteMem[a];
    }
  }
}

void NoteHighest(){
  highestNote = 0;//reset
  for (int a = 0; a < MAX_VOICES; a++){
    if(noteMem[a] > highestNote){
       highestNote = noteMem[a];
    }
  }
}

void loop(){
 MIDI.read();
}
