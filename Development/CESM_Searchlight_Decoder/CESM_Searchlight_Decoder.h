/**********************************************************************

CESM_Searchlight_Decoder.h
COPYRIGHT (C) 2017 David J. Cutting

**********************************************************************/

#include "Config.h"

#define DCC_READ_PIN        PIN_B1                                //  Pin number for the pin that reads the DCC signal
#define PROG_JUMPER_PIN     PIN_B2                                //  Pin number that detects if decoder is in programming mode

#define RED           0                                           //  Defines red aspect as 0
#define GREEN         1                                           //  Defines green aspect as 1
#define YELLOW        2                                           //  Defines yellow aspect as 2
#define LUNAR         3                                           //  Defines lunar aspect as 3
#define FLASH_RED     4                                           //  Defines flashing red aspect as 4
#define FLASH_GREEN   5                                           //  Defines flashing green aspect as 5
#define FLASH_YELLOW  6                                           //  Defines flashing yellow aspect as 6
#define FLASH_LUNAR   7                                           //  Defines flashing lunar aspect as 7
#define BLACK         8                                           //  Defines clear/black/unlit aspect as 8

byte  lensArrangement[] = {GREEN, RED, YELLOW, LUNAR};            //  Sets the order of the lenses (used for vane movement simulation)

#define STATE_IDLE          0                                     //  Idle state for effects switch case
#define STATE_DIMMING       1                                     //  Dimming state for effects switch case  
#define STATE_BRIGHTENING   2                                     //  Brightening state for effects switch case
#define STATE_SIMUL         3                                     //  ??? state for effects switch case

#define NUM_HEADS     3                                           //  Defines 3 heads on signal decoder if the searchlight configuration is selected
#define NUM_ASPECTS   5                                           //  Defines 5 aspects
#define NUM_LENSES    4

struct headState                                                  //  headState structure controls the behavior of each head
{
  byte    prevAspect = RED;                                       //  Color from last command
  byte    currAspect = RED;                                       //  Color from current command
  
  byte    headStatus = STATE_IDLE;                                //  Effects state of head         
  
  byte    currLens = lensArrangement[1];                          //  Sets current lens that signal is at (for vane movement simulation)
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
