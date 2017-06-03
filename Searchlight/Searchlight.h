/**********************************************************************

Searchlight.h
COPYRIGHT (C) 2017 David J. Cutting, Alex Shepard

**********************************************************************/

#include "Config.h"

#define DCC_READ_PIN        PIN_B1                                //  Pin number for the pin that reads the DCC signal
#define PROG_JUMPER_PIN     PIN_B2                                //  Pin number that detects if decoder is in programming mode

#define BLACK   0                                                 //  Black color ID
#define RED     1                                                 //  Red color ID                                        
#define GREEN   2                                                 //  Green color ID
#define YELLOW  3                                                 //  Yellow color ID
#define LUNAR   4                                                 //  Lunar color ID                                       

byte  lensArrangement[] = {GREEN, RED, YELLOW, LUNAR};            //  Defines which order colors are arranged on vane (2, 1, 3, 4)

#define STATE_IDLE          0
#define STATE_DIMMING       1
#define STATE_BRIGHTENING   2

#define NUM_HEADS     3                                           //  Defines 3 heads on signal decoder if the searchlight configuration is selected
#define NUM_LED_PER_HEAD  3
#define NUM_ASPECTS   9                                           //  Defines 5 aspects
#define NUM_LENSES    4
#define NUM_PINS      9

#define OFF 0                // Turns head off
#define ON  1                  // Turns head on

#define NO_EFFECT 0            // No special effect
#define EFFECT_ON 1           // Vane movement effect
#define EFFECT_FLASHING 2     // Flashing effect
#define EFFECT_ON_FLASHING 3  // Flashing and vane movement effects

struct headState                                                 //  headState structure controls the behavior of each head
{
  byte    prevAspect = RED;                                          //  Color from last command
  byte    currAspect = RED;                                          //  Color from current command
  
  byte    headStatus = STATE_IDLE;
  byte    effect = NO_EFFECT;                                         

  byte    currLens = lensArrangement[RED];

  byte    on_off = ON;
};   

struct aspectInfo
{
  byte    lensNumber;
  byte    on_off;
  byte    effect;
};

aspectInfo aspectTable[NUM_ASPECTS] = 
{
  //{lens, effect},     This table translates aspects into lens numbers

  {RED,     ON,   EFFECT_ON},               // Aspect 0 is lens 1 (red) with a vane effect (if enabled)
  {YELLOW,  ON,   EFFECT_ON},               // Aspect 1 is lens 2 (yellow) with a vane effect (if enabled)
  {GREEN,   ON,   EFFECT_ON},               // Aspect 2 is lens 0 (green) with a vane effect (if enabled)
  {LUNAR,   ON,   EFFECT_ON},               // Aspect 3 is lens 3 (lunar) with a vane effect (if enabled)
  {RED,     ON,   EFFECT_ON_FLASHING},      // Aspect 4 is lens 1 (red) with a flashing (always) vane (if enabled) effect
  {YELLOW,  ON,   EFFECT_ON_FLASHING},      // Aspect 5 is lens 2 (yellow) with a flashing (always) vane (if enabled) effect
  {GREEN,   ON,   EFFECT_ON_FLASHING},      // Aspect 6 is lens 0 (green) with a flashing (always) vane (if enabled) effect
  {LUNAR,   ON,   EFFECT_ON_FLASHING},      // Aspect 7 is lens 3 (lunar) with a flashing (always) vane (if enabled) effect
  {BLACK,   OFF,  NO_EFFECT}                // Aspect 8 is dark
};

headState headStates[3];

struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};

int on_off[NUM_HEADS];                                                // Creates array that tracks whether each head is on or off
