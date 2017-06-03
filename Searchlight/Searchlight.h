/**********************************************************************

CESM_Searchlight_Decoder.h
COPYRIGHT (C) 2017 David J. Cutting

**********************************************************************/

#include "Config.h"

#define RED     1                                              
#define YELLOW  3                                           
#define GREEN   2                                              
#define LUNAR   4                                         
#define BLACK   0                                                 

byte  lensArrangement[] = {GREEN, RED, YELLOW, LUNAR};

#define STATE_IDLE          0
#define STATE_DIMMING       1
#define STATE_BRIGHTENING   2
#define STATE_SIMUL         3

#define NUM_HEADS     3                                           //  Defines 3 heads on signal decoder if the searchlight configuration is selected
#define NUM_ASPECTS   9                                           //  Defines 5 aspects
#define NUM_LENSES    4
#define NUM_PINS      9

struct headState                                                 //  headState structure controls the behavior of each head
{
  byte    prevAspect = 1;                                          //  Color from last command
  byte    currAspect = 1;                                          //  Color from current command
  
  byte    headStatus = STATE_IDLE;                                         
  
  byte    currLens = lensArrangement[RED];
};   

struct aspectInfo
{
  byte    lensNumber;
  byte    on_off;
  byte    effect;
};

#define OFF 0                // Turns head off
#define ON  1                  // Turns head on

#define NO_EFFECT 0            // No special effect
#define EFFECT_ON 1           // Vane movement effect
#define EFFECT_FLASHING 2     // Flashing effect
#define EFFECT_ON_FLASHING 3  // Flashing and vane movement effects

aspectInfo aspectTable[NUM_ASPECTS] = 
{
  //{lens, effect},     This table translates aspects into lens numbers

  {1, ON, EFFECT_ON},               // Aspect 0 is lens 1 (red) with a vane effect (if enabled)
  {2, ON, EFFECT_ON},               // Aspect 1 is lens 2 (yellow) with a vane effect (if enabled)
  {0, ON, EFFECT_ON},               // Aspect 2 is lens 0 (green) with a vane effect (if enabled)
  {3, ON, EFFECT_ON},               // Aspect 3 is lens 3 (lunar) with a vane effect (if enabled)
  {1, ON, EFFECT_ON_FLASHING},      // Aspect 4 is lens 1 (red) with a flashing (always) vane (if enabled) effect
  {2, ON, EFFECT_ON_FLASHING},      // Aspect 5 is lens 2 (yellow) with a flashing (always) vane (if enabled) effect
  {0, ON, EFFECT_ON_FLASHING},      // Aspect 6 is lens 0 (green) with a flashing (always) vane (if enabled) effect
  {3, ON, EFFECT_ON_FLASHING},      // Aspect 7 is lens 3 (lunar) with a flashing (always) vane (if enabled) effect
  {0, OFF, NO_EFFECT}                      // Aspect 8 is dark
};

headState headStates[3];

struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};
