/**********************************************************************

CESM_Searchlight_Decoder.h
COPYRIGHT (C) 2017 David J. Cutting

**********************************************************************/

#include "Config.h"

const int LED_CONTROL_PINS[9] = {PIN_A0, PIN_A1, PIN_A2, PIN_A3, PIN_A4, PIN_A5, PIN_A6, PIN_A7, PIN_B0};

#define DCC_READ_PIN        PIN_B1                                //  Pin number for the pin that reads the DCC signal
#define PROG_JUMPER_PIN     PIN_B2                                //  Pin number that detects if decoder is in programming mode

#define BLACK   0                                                 //  Defines clear aspect as 0
#define RED     1                                                 //  Defines red aspect as 1
#define GREEN   2                                                 //  Defines green aspect as 2
#define YELLOW  3                                                 //  Defines yellow aspect as 3
#define LUNAR   4                                                 //  Defines lunar aspect as 4

byte  lensArrangement[] = {GREEN, RED, YELLOW, LUNAR};

#define STATE_IDLE          0
#define STATE_DIMMING       1
#define STATE_BRIGHTENING   2
#define STATE_SIMUL         3

#define NUM_HEADS     3                                           //  Defines 3 heads on signal decoder if the searchlight configuration is selected
#define NUM_ASPECTS   5                                           //  Defines 5 aspects
#define NUM_LENSES    4
#define NUM_PINS      9

struct headState                                                 //  headState structure controls the behavior of each head
{
  byte    prevAspect = 1;                                          //  Color from last command
  byte    currAspect = 1;                                          //  Color from current command
  
  byte    headStatus = STATE_IDLE;                                         
  
  byte    currLens = lensArrangement[1];
};   

struct aspectInfo
{
  byte    lensNumber;
  byte    effect;
};

#define EFFECT_ON 1
#define EFFECT_FLASHING 2

aspectInfo aspectTable[NUM_ASPECTS] = 
{
  {1, 0},
  {1, EFFECT_ON},
  {0, EFFECT_ON},
  {2, EFFECT_ON},
  {3, EFFECT_ON}  
};

headState headStates[3];

struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};
