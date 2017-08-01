/**********************************************************************

Searchlight.h
COPYRIGHT (C) 2017 David J. Cutting

Part of CESM_SEARCHLIGHT_CONTROLLER

**********************************************************************/

///////////////////////////
//  INCLUDE HEADER FILE  //
///////////////////////////

#include "Config.h"

//////////////////////////////////
//  INCLUDE REQUIRED LIBRARIES  //
//////////////////////////////////

#include <NmraDcc.h>                                                  //  You must use the branch available here: https://github.com/mrrwa/NmraDcc/tree/AddOutputModeAddressing
#include <SoftPWM.h>                                                  //  Include SoftPWM library from here: https://github.com/Palatis/arduino-softpwm/ 

//////////////////////////
//  CREATE DCC OBJECTS  //
//////////////////////////

NmraDcc Dcc;                                                          //  Create an NmraDcc object named Dcc
DCC_MSG Packet;                                                       //  Create a DCC_MSG object named Packet

////////////////////////////
//  SET UP DCC VARIABLES  //
//////////////////////////// 

typedef enum
{
  ADDR_SET_DISABLED = 0,
  ADDR_SET_DONE,
  ADDR_SET_ENABLED
} ADDR_SET_MODE;

ADDR_SET_MODE AddrSetMode = ADDR_SET_DISABLED ;                       //  Keeps track of whether the programming jumper is set

uint16_t baseAddress;                                                 //  Keeps track of the base address of the decoder
uint8_t FactoryDefaultCVIndex = 0;                                    //  Controls reset of the decoder
uint8_t commonPole;                                                   //  Keeps track of whether the decoder is set up as common anode or common cathode

/////////////////////////////////////
//  SET UP EFFECT TIMER VARIABLES  //
///////////////////////////////////// 

#define ANIMATE_DELAY_TIME 30
#define FLASH_PULSE_DELAY 1000

////////////////////////////////////
//  SET UP SIGNAL HEAD CONSTANTS  //
////////////////////////////////////

#define NUM_HEADS   3                                                 //  Defines 3 heads on signal decoder if the searchlight configuration is selected
#define NUM_ASPECTS 9                                                 //  Defines 9 possible aspects (controlled by aspectTable)
#define NUM_COLORS  5                                                 //  Defines 5 possible colors for output

#define BLACK   0                                                     //  Black color ID
#define RED     1                                                     //  Red color ID                                        
#define GREEN   2                                                     //  Green color ID
#define YELLOW  3                                                     //  Yellow color ID
#define LUNAR   4                                                     //  Lunar color ID   

#define STATE_IDLE          0                                         //  Idle state ID for switch case
#define STATE_ANIMATE       1                                         //  Dimming ID for switch case
#define STATE_ANIMATE_BLACK 2                                         //  ID for case where animation is being switched and signal is black momentarily

#define OFF 0                                                         //  Defines off as 0
#define ON  1                                                         //  Defines on as 1

#define NO_EFFECT           0                                         //  No special effect
#define EFFECT_FLASHING     1                                         //  Flashing effect

#define CV_OPS_MODE_ADDRESS_LSB 33                                    //  Because most DCC Command Stations don't support the DCC Accessory Decoder OPS Mode programming,
#define CV_OPS_MODE_ADDRESS_MSB 34                                    //  we fake a DCC Mobile Decoder Address for OPS Mode Programming. This is one of the two CV numbers 
                                                                      //  that controls it.
                                                                      

struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};

CVPair FactoryDefaultCVs[] =
{
  {CV_ACCESSORY_DECODER_ADDRESS_LSB, DEFAULT_ADDRESS + 1},            //  Set the accessory decoder address. Needs to be +1 to be compatible with DCC Spec
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},                             
  
  {CV_OPS_MODE_ADDRESS_LSB, DEFAULT_ADDRESS},                         //  Set the OPS mode programming address
  {CV_OPS_MODE_ADDRESS_MSB, 0},     

  {35, 0},                                                            //  Dark aspect red LED intensity (0 = 0%, 63 = 100%)
  {36, 0},                                                            //  Dark aspect green LED intensity (0 = 0%, 63 = 100%)
  {37, 0},                                                            //  Dark aspect blue LED intensity (0 = 0%, 63 = 100%)
  
  {38, 32},                                                           //  Red aspect red LED intensity (0 = 0%, 63 = 100%)
  {39, 0},                                                            //  Red aspect green LED intensity (0 = 0%, 63 = 100%)
  {40, 0},                                                            //  Red aspect blue LED intensity (0 = 0%, 63 = 100%)
  
  {41, 0},                                                            //  Green aspect red LED intensity (0 = 0%, 63 = 100%)
  {42, 32},                                                           //  Green aspect green LED intensity (0 = 0%, 63 = 100%)
  {43, 0},                                                            //  Green aspect blue LED intensity (0 = 0%, 63 = 100%)
  
  {44, 32},                                                           //  Yellow aspect red LED intensity (0 = 0%, 63 = 100%)
  {45, 32},                                                           //  Yellow aspect green LED intensity (0 = 0%, 63 = 100%)
  {46, 0},                                                            //  Yellow aspect blue LED intensity (0 = 0%, 63 = 100%)
  
  {47, 32},                                                           //  Lunar aspect red LED intensity (0 = 0%, 63 = 100%)
  {48, 32},                                                           //  Lunar aspect green LED intensity (0 = 0%, 63 = 100%)
  {49, 32},                                                           //  Lunar aspect blue LED intensity (0 = 0%, 63 = 100%)
  
  {53, 1},                                                            //  Enable/Disable vane movement
  {54, 1},                                                            //  Enable/Disable flashing 

  {55, 1},                                                            //  Polarity for all heads - 1 = Common Anode, 0 = Common Cathode 
};

// The following four lists define the intensities of the colors at different points along the animation. They scale from 0 to 63.
const byte bypassColor[] = {3, 11, 24, 32, 25, 8, 3, 0};
const byte leaveColor[] = {32, 29,  25,  19,  14,  5, 17,  24,  19,  8, 6, 0};
const byte stopAtEnergized[] = {3, 7, 10, 14, 18, 25, 30, 32, 32, 26, 13, 19, 25, 29, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 24, 19, 22, 29, 32};
const byte stopAtNonEnergized[] = {1, 2, 6, 16, 19, 22, 27, 32, 32, 32, 32, 32, 32, 32, 32, 32, 29, 26, 21, 14, 10, 3, 3, 5, 7, 11, 14, 18, 22, 28, 32, 32, 32, 32, 
                                                                                              32, 32, 32, 32, 32, 32, 32, 29, 27, 24, 21, 16, 14, 18, 21, 25, 28, 32};

typedef struct 
{
  byte    vanePos;
  byte    red;
  byte    grn;
  byte    blu;
} ColorInfo;

ColorInfo colorCache[ NUM_COLORS ] =
{
  {0, Dcc.getCV(35), Dcc.getCV(36), Dcc.getCV(37)},  // Black
  {1, Dcc.getCV(38), Dcc.getCV(39), Dcc.getCV(40)},  // Red
  {0, Dcc.getCV(41), Dcc.getCV(42), Dcc.getCV(43)},  // Green
  {2, Dcc.getCV(44), Dcc.getCV(45), Dcc.getCV(46)},  // Yellow
  {3, Dcc.getCV(47), Dcc.getCV(48), Dcc.getCV(49)},  // Lunar
};

const byte vaneOrder[] = {GREEN, RED, YELLOW, LUNAR};

typedef struct                                                      //  headState structure controls the behavior of each head
{
  byte    startColor = RED;                                           //  Color of the signal when command was recieved from DCC system
  byte    currColor = RED;                                            //  Color of the aspect in animation
  byte    nextColor = RED;                                            //  Color that is being targeted
  
  byte    headStatus = STATE_IDLE;
  byte    effect = NO_EFFECT;                                         

  ColorInfo colorInfo = colorCache[RED];

  int     inputStabilizeCount = 0;
  long    lastAnimateTime = 0;
  long    lastFlashTime = 0;
  int     frame = 0;
} HeadState;   

HeadState headStates[3];

typedef struct
{
  byte    colorID;
  byte    on_off;
  byte    effect;
} AspectInfo;

AspectInfo aspectTable[ NUM_ASPECTS ] = 
{
  {RED,     ON,   NO_EFFECT},                                         // Aspect 0 is lens 1 (red) with a vane effect (if enabled)
  {YELLOW,  ON,   NO_EFFECT},                                         // Aspect 1 is lens 2 (yellow) with a vane effect (if enabled)
  {GREEN,   ON,   NO_EFFECT},                                         // Aspect 2 is lens 0 (green) with a vane effect (if enabled)
  {LUNAR,   ON,   NO_EFFECT},                                         // Aspect 3 is lens 3 (lunar) with a vane effect (if enabled)
  {RED,     ON,   EFFECT_FLASHING},                                   // Aspect 4 is lens 1 (red) with a flashing (always) vane (if enabled) effect
  {YELLOW,  ON,   EFFECT_FLASHING},                                   // Aspect 5 is lens 2 (yellow) with a flashing (always) vane (if enabled) effect
  {GREEN,   ON,   EFFECT_FLASHING},                                   // Aspect 6 is lens 0 (green) with a flashing (always) vane (if enabled) effect
  {LUNAR,   ON,   EFFECT_FLASHING},                                   // Aspect 7 is lens 3 (lunar) with a flashing (always) vane (if enabled) effect
  {BLACK,   OFF,  NO_EFFECT}                                          // Aspect 8 is dark
};
