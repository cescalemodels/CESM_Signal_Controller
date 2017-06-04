/**********************************************************************

Searchlight.h
COPYRIGHT (C) 2017 David J. Cutting, Alex Shepard

**********************************************************************/

#include "Config.h"

#include <NmraDcc.h>
#include <SoftPWM.h>                                                  //  Include SoftPWM library from here: https://github.com/Palatis/arduino-softpwm/ 

NmraDcc Dcc;                                                          //  Create an NmraDcc object named Dcc
DCC_MSG Packet;                                                       //  Create a DCC_MSG object named Packet

#define DCC_READ_PIN        PIN_B1                                    //  Pin number for the pin that reads the DCC signal
#define PROG_JUMPER_PIN     PIN_B2                                    //  Pin number that detects if decoder is in programming mode

#define NUM_HEADS         3                                           //  Defines 3 heads on signal decoder if the searchlight configuration is selected
#define NUM_LED_PER_HEAD  3                                           //  Defines 3 LEDs in each signal head
#define NUM_ASPECTS       9                                           //  Defines 9 possible aspects (controlled by aspectTable)
#define NUM_LENSES        4                                           //  Defines 4 lenses per signal head (order controlled by lensArrangement)
#define NUM_PINS          9                                           //  Defines 9 pins on the decoder
#define NUM_COLORS        5                                           //  Defines 5 possible colors for output

#define BLACK   0                                                     //  Black color ID
#define RED     1                                                     //  Red color ID                                        
#define GREEN   2                                                     //  Green color ID
#define YELLOW  3                                                     //  Yellow color ID
#define LUNAR   4                                                     //  Lunar color ID                                       

#define STATE_IDLE          0                                         //  Idle state ID for switch case
#define STATE_DIMMING       1                                         //  Dimming ID for switch case

#define OFF 0                                                         //  Defines off as 0
#define ON  1                                                         //  Defines on as 1

#define NO_EFFECT 0                                                   //  No special effect
#define EFFECT_ON 1                                                   //  Vane movement effect
#define EFFECT_FLASHING 2                                             //  Flashing effect
#define EFFECT_ON_FLASHING 3                                          //  Flashing and vane movement effects

#define CV_OPS_MODE_ADDRESS_LSB 33                                    //  Because most DCC Command Stations don't support the DCC Accessory Decoder OPS Mode programming,
                                                                      //  we fake a DCC Mobile Decoder Address for OPS Mode Programming. This is one of the two CV numbers 
                                                                      //  that controls it.

struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};

CVPair FactoryDefaultCVs[] =
{
  {CV_ACCESSORY_DECODER_ADDRESS_LSB, DEFAULT_ADDRESS},                //  Set the accessory decoder address
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},                             
  
  {CV_OPS_MODE_ADDRESS_LSB,DEFAULT_ADDRESS},                          //  Set the OPS mode programming address
  {CV_OPS_MODE_ADDRESS_LSB+1, 0},
  
  {30, 0},                                                            //  Polarity for all heads - 0 = Common Anode, 1 = Common Cathode      

  {35, 0},                                                            //  Dark aspect red LED intensity (0 = 0%, 63 = 100%)
  {36, 0},                                                            //  Dark aspect green LED intensity
  {37, 0},                                                            //  Dark aspect blue LED intensity
  
  {38, 63},                                                           //  Red aspect red LED intensity
  {39, 0},                                                            //  Red aspect green LED intensity
  {40, 0},                                                            //  Red aspect blue LED intensity
  
  {41, 0},                                                            //  Green aspect red LED intensity
  {42, 63},                                                           //  Green aspect green LED intensity
  {43, 0},                                                            //  Green aspect blue LED intensity
  
  {44, 63},                                                           //  Yellow aspect red LED intensity
  {45, 63},                                                           //  Yellow aspect green LED intensity
  {46, 0},                                                            //  Yellow aspect blue LED intensity
  
  {47, 63},                                                           //  Lunar aspect red LED intensity
  {48, 63},                                                           //  Lunar aspect green LED intensity
  {49, 63},                                                           //  Lunar aspect blue LED intensity
  
  {50, 1},                                                            //  Head A enable/disable fading
  {51, 1},                                                            //  Head B enable/disable fading
  {52, 1},                                                            //  Head C enable/disable fading
  
  {53, 1},                                                            //  Head A enable/disable vane movement
  {54, 1},                                                            //  Head B enable/disable vane movement
  {55, 1},                                                            //  Head C enable/disable vane movement
  
  {56, 5},                                                            //  Head A fade steps per tick
  {57, 5},                                                            //  Head B fade steps per tick
  {58, 5},                                                            //  Head C fade steps per tick

  {59, 2},                                                            //  Head A vane steps per tick
  {60, 2},                                                            //  Head B vane steps per tick
  {61, 2},                                                            //  Head C vane steps per tick   

  {66, RED},                                                          //  De-energized solenoid color for all heads (RED, YELLOW, LUNAR, GREEN, or DARK)

  {67, 1},                                                            //  Flashes per second, signal head A
  {68, 1},                                                            //  Flashes per second, signal head B  
  {69, 1},                                                            //  Flashes per second, signal head C
};

byte  lensArrangement[] = {GREEN, RED, YELLOW, LUNAR};                //  Defines which order colors are arranged on vane (2, 1, 3, 4)

struct colorInfo
{
  byte    red;
  byte    grn;
  byte    blu;
};

colorInfo colorCache[ NUM_COLORS ] =
{
  {Dcc.getCV(35), Dcc.getCV(36), Dcc.getCV(37)},
  {Dcc.getCV(38), Dcc.getCV(39), Dcc.getCV(40)},
  {Dcc.getCV(41), Dcc.getCV(42), Dcc.getCV(43)},
  {Dcc.getCV(44), Dcc.getCV(45), Dcc.getCV(46)},
  {Dcc.getCV(47), Dcc.getCV(48), Dcc.getCV(49)},
};

struct headState                                                      //  headState structure controls the behavior of each head
{
  byte    prevAspect = RED;                                           //  Color from last command
  byte    currAspect = RED;                                           //  Color from current command
  
  byte    headStatus = STATE_IDLE;
  byte    effect = NO_EFFECT;                                         

  byte    currLens = lensArrangement[RED];

  byte    on_off = ON;

  colorInfo currBlend = colorCache[1];
};   

headState headStates[3];

struct aspectInfo
{
  byte    lensNumber;
  byte    on_off;
  byte    effect;
};

aspectInfo aspectTable[ NUM_ASPECTS ] = 
{
  {RED,     ON,   EFFECT_ON},                                         // Aspect 0 is lens 1 (red) with a vane effect (if enabled)
  {YELLOW,  ON,   EFFECT_ON},                                         // Aspect 1 is lens 2 (yellow) with a vane effect (if enabled)
  {GREEN,   ON,   EFFECT_ON},                                         // Aspect 2 is lens 0 (green) with a vane effect (if enabled)
  {LUNAR,   ON,   EFFECT_ON},                                         // Aspect 3 is lens 3 (lunar) with a vane effect (if enabled)
  {RED,     ON,   EFFECT_ON_FLASHING},                                // Aspect 4 is lens 1 (red) with a flashing (always) vane (if enabled) effect
  {YELLOW,  ON,   EFFECT_ON_FLASHING},                                // Aspect 5 is lens 2 (yellow) with a flashing (always) vane (if enabled) effect
  {GREEN,   ON,   EFFECT_ON_FLASHING},                                // Aspect 6 is lens 0 (green) with a flashing (always) vane (if enabled) effect
  {LUNAR,   ON,   EFFECT_ON_FLASHING},                                // Aspect 7 is lens 3 (lunar) with a flashing (always) vane (if enabled) effect
  {BLACK,   OFF,  NO_EFFECT}                                          // Aspect 8 is dark
};


