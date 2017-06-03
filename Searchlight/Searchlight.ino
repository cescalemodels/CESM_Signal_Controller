/**********************************************************************

Searchlight.ino
COPYRIGHT (C) 2017 David J. Cutting, Alex Shepard

**********************************************************************/

#define DEBUG                                                         //  Uncomment this line to enable serial ouutput on lines 2 and 3

#define PROG_JUMPER_PIN     PIN_B2

#include "Searchlight.h"                                              //  Include the Searchlight header file
#include "Config.h"                                                   //  Include the Configuration header file

#include <NmraDcc.h>                                                  //  Include the NMRADcc library from Alex Shepard
#include <SoftPWM.h>                                                  //  Include SoftPWM library from here: https://github.com/Palatis/arduino-softpwm/ 

NmraDcc Dcc;                                                          //  Create an NmraDcc object named Dcc
DCC_MSG Packet;                                                       //  Create a DCC_MSG object named Packet

int fadeTick = 0;                                                     //  Counter variable that times the dimming of the signal            
int stepLength = 25;                                                  //  Sets amount of time (microseconds) between dimming steps

uint16_t baseAddress;                                                 //  Keeps track of the base address of the decoder
uint8_t AddrSetModeEnabled = 0;                                       //  Keeps track of whether the programming jumper is set
uint8_t FactoryDefaultCVIndex = 0;                                    //  Controls reset of the decoder
uint8_t commonPole;                                                   //  Keeps track of whether the decoder is set up as common anode or common cathode based on CVs

SOFTPWM_DEFINE_CHANNEL(0, DDRA, PORTA, PORTA0);                       //  Initiates softpwm channel 0
SOFTPWM_DEFINE_CHANNEL(3, DDRA, PORTA, PORTA3);                       //  Initiates softpwm channel 3
SOFTPWM_DEFINE_CHANNEL(4, DDRA, PORTA, PORTA4);                       //  Initiates softpwm channel 4
SOFTPWM_DEFINE_CHANNEL(5, DDRA, PORTA, PORTA5);                       //  Initiates softpwm channel 5
SOFTPWM_DEFINE_CHANNEL(6, DDRA, PORTA, PORTA6);                       //  Initiates softpwm channel 6
SOFTPWM_DEFINE_CHANNEL(7, DDRA, PORTA, PORTA7);                       //  Initiates softpwm channel 7
SOFTPWM_DEFINE_CHANNEL(8, DDRB, PORTB, PORTB0);                       //  Initiates softpwm channel 8

#ifndef DEBUG
SOFTPWM_DEFINE_CHANNEL(1, DDRA, PORTA, PORTA1);                       //  Initiates softpwm channel 1
SOFTPWM_DEFINE_CHANNEL(2, DDRA, PORTA, PORTA2);                       //  Initiates softpwm channel 2
SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS(9, 64);                         //  Sets up 9 channels of softpwm with 64 levels
#else
SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS(7, 64);                         //  Sets up 7 channels of softpwm with 64 levels
#endif

#define CV_OPS_MODE_ADDRESS_LSB 33                                    //  Because most DCC Command Stations don't support the DCC Accessory Decoder OPS Mode programming,
                                                                      //  we fake a DCC Mobile Decoder Address for OPS Mode Programming. This is the CV value that controls it.

CVPair FactoryDefaultCVs[] =
{
  {CV_ACCESSORY_DECODER_ADDRESS_LSB, DEFAULT_ADDRESS},                //  Set the accessory decoder address
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},                             
  
  {CV_OPS_MODE_ADDRESS_LSB,       0x01},	                            
  {CV_OPS_MODE_ADDRESS_LSB+1,     0x00},
  
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

#ifdef DEBUG
void setSoftPWMValueFromCV( uint8_t headNumber )                            //  Sets the PWM values of each LED in a certain head based on CV values and common polarity
{                                                                   
  Palatis::SoftPWM.set(   headNumber * 3      , (uint8_t) abs( Dcc.getCV( 35 + ( headStates[headNumber].currAspect * NUM_LED_PER_HEAD )    ) - ( 63 * commonPole ) ) );
  Palatis::SoftPWM.set( ( headNumber * 3 ) + 1, (uint8_t) abs( Dcc.getCV( 35 + ( headStates[headNumber].currAspect * NUM_LED_PER_HEAD ) + 1) - ( 63 * commonPole ) ) );
  Palatis::SoftPWM.set( ( headNumber * 3 ) + 2, (uint8_t) abs( Dcc.getCV( 35 + ( headStates[headNumber].currAspect * NUM_LED_PER_HEAD ) + 2) - ( 63 * commonPole ) ) );
}
#endif

void notifyDccSigOutputState( uint16_t Addr, uint8_t State )          //  Notifies program of an incoming signal packet
{
  #ifdef  DEBUG
  Serial.print( F( "notifyDccSigState: Addr: " ) );
  Serial.print( Addr );                                               //  Prints signal address from incoming packet
  Serial.print( F( " State: " ) );
  Serial.println( State );                                            //  Print aspect number from incoming packet
  #endif
  
  if( ( Addr < baseAddress ) || ( Addr >= ( baseAddress + NUM_HEADS ) ) )     //  Make sure we're only looking at our addresses
  {
    #ifdef  DEBUG
    Serial.println( F( "notifyDccSigState: Address out of range" ) );
    #endif
    
    return;
  }
  if( State > NUM_ASPECTS )                                           //  Check we've got a valid Aspect
  {
    #ifdef  DEBUG
    Serial.println( F( "notifyDccSigState: State out of range" ) );
    #endif
    
    return;
  }

  uint8_t headIndex = Addr - baseAddress ;                            //  Determines which head we're talking about (0, 1, 2, ...)
  
  #ifdef  DEBUG
  Serial.print( F( "notifyDccSigState: Index: " ) );
  Serial.println( headIndex );                                        //  Prints which head we're working with  
  #endif

  headStates[headIndex].prevAspect = headStates[headIndex].currAspect;//  Stores the old aspect in the aspect archive
  headStates[headIndex].currAspect = State;                           //  Stores the state in the current aspect  
  headStates[headIndex].headStatus = STATE_IDLE;                      //  Sets the status to idle (not dimming or brightening)
  headStates[headIndex].currLens = aspectTable[State].lensNumber;     //  Looks up the lens number in the aspect table, stores to head info table.
  headStates[headIndex].on_off = aspectTable[State].on_off;           //  Looks up whether the aspect is on or off, stores to head info table
  headStates[headIndex].effect = aspectTable[State].effect;           //  Looks up the aspect effect and stores it to head info table

  #ifdef DEBUG
  setSoftPWMValueFromCV(headIndex);                                   //  Runs the simple PWM set function
  #endif
}

#ifdef  NOTIFY_DCC_MSG                                                
void notifyDccMsg( DCC_MSG * Msg )                                    //  Prints DCC packets to serial line if in debug mode
{
  #ifdef DEBUG
  Serial.print(F("notifyDccMsg: ")) ;
  #endif
  
  for(uint8_t i = 0; i < Msg->Size; i++)
  {
    #ifdef DEBUG
    Serial.print(Msg->Data[i], HEX);
    Serial.write(' ');
    #endif
  }
  #ifdef DEBUG
  Serial.println();
  #endif
}
#endif

void notifyCVChange( uint16_t CV, uint8_t Value )                     //  Runs if a CV value is changed
{
  #ifdef DEBUG
  Serial.print(F("notifyCVChange CV: "));
  Serial.print( CV );
  Serial.print(F(" Value: "));
  Serial.println( Value );
  #endif
  
  switch(CV)                                                          //  Changes the locally stored (not EEPROM) values of the CVs if one changes.
  {
    case CV_ACCESSORY_DECODER_ADDRESS_LSB:
    case CV_ACCESSORY_DECODER_ADDRESS_MSB:
      baseAddress = Dcc.getAddr();
      break;

    case 30:
      commonPole = Value;
  }
}

void notifyDccAccOutputAddrSet( uint16_t OutputAddr )
{
  #ifdef DEBUG
  Serial.print(F("notifyDccAccOutputAddrSet Output Addr: "));
  Serial.print( OutputAddr );
  #endif
  
  Dcc.setCV(CV_OPS_MODE_ADDRESS_LSB,     OutputAddr & 0x00FF);
  Dcc.setCV(CV_OPS_MODE_ADDRESS_LSB + 1, (OutputAddr >> 8) & 0x00FF);

  baseAddress = Dcc.getAddr();
  AddrSetModeEnabled = 0;
  
  #ifdef DEBUG
  Serial.print(F(" baseAddress: "));
  Serial.println( baseAddress );
  #endif 
}

void setup() 
{
  #ifdef DEBUG
  Serial.begin( 115200 );
  Serial.println(F("\nCESM_Searchlight_Decoder"));
  #endif
 
  pinMode(DCC_READ_PIN, INPUT);
  pinMode(PROG_JUMPER_PIN, INPUT_PULLUP);
  
  Dcc.pin( 0, DCC_READ_PIN, 0 );  // Setup which External Interrupt, the Pin it's associated with that we're using and enable the Pull-Up
  Dcc.init( MAN_ID_DIY, 10, FLAGS_DCC_ACCESSORY_DECODER | FLAGS_OUTPUT_ADDRESS_MODE, CV_OPS_MODE_ADDRESS_LSB );

  commonPole = Dcc.getCV(30);
  
  baseAddress = Dcc.getAddr(); // this is broken for now      TODO: Fix or replace

  #ifdef DEBUG
  Serial.print(F("Base Address: "));
  Serial.println(baseAddress);
  #endif

  #ifndef DEBUG
  Palatis::SoftPWM.begin(60); // begin with 60hz pwm frequency
  #endif

  #ifdef RESET_CVS_ON_POWER
  notifyCVResetFactoryDefault(); // Force Restore to Factory Defaults Every time the Decoder is Restarted
  #endif
}

void loop() 
{
  Dcc.process();                                                // Read the DCC bus and process the signal. Needs to be called frequently.

  for( int headIndex = 0; headIndex < NUM_HEADS; headIndex++ ) 
  {
    switch( headStates[headIndex].headStatus )
    {
      case STATE_IDLE:
        Palatis::SoftPWM.set( headIndex * NUM_LED_PER_HEAD,        ( uint8_t )Dcc.getCV(38 + ( ( headStates[headIndex].currLens - 1) * NUM_LED_PER_HEAD ) ));
        Palatis::SoftPWM.set(( headIndex * NUM_LED_PER_HEAD ) + 1, ( uint8_t )Dcc.getCV(38 + ( ( headStates[headIndex].currLens - 1) * NUM_LED_PER_HEAD ) + 1 ) );
        Palatis::SoftPWM.set(( headIndex * NUM_LED_PER_HEAD ) + 2, ( uint8_t )Dcc.getCV(38 + ( ( headStates[headIndex].currLens - 1) * NUM_LED_PER_HEAD ) + 2 ) );
        break;
      case STATE_DIMMING:
        break;
      case STATE_BRIGHTENING:
        break;
    }
  }

  if ( FactoryDefaultCVIndex && Dcc.isSetCVReady()) 
  {
    FactoryDefaultCVIndex--; // Decrement first as initially it is the size of the array
    Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV, FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
    #ifdef DEBUG
    Serial.print(F("FactoryDefault CV: "));
    Serial.print( FactoryDefaultCVs[FactoryDefaultCVIndex].CV );
    Serial.print(F("  Value: "));
    Serial.println( FactoryDefaultCVs[FactoryDefaultCVIndex].Value );
    #endif

    if(FactoryDefaultCVIndex == 0)
    {    
      baseAddress = Dcc.getAddr();
      #ifdef DEBUG
      Serial.print(F("Base Address: "));
      Serial.println(baseAddress);
      #endif
    }
  }

  // Enable Decoder Address Setting from next received Accessory Decoder packet
  if( !AddrSetModeEnabled && digitalRead(PROG_JUMPER_PIN) == LOW)
  {
    #ifdef DEBUG
    Serial.println(F("Enable DCC Address Set Mode"));
    #endif
    AddrSetModeEnabled = 1;
    Dcc.setAccDecDCCAddrNextReceived(AddrSetModeEnabled);  
  }

  if( AddrSetModeEnabled && digitalRead(PROG_JUMPER_PIN) == HIGH)
  {
    #ifdef DEBUG
    Serial.println(F("Disable DCC Address Set Mode"));
    #endif
    AddrSetModeEnabled = 0;
    Dcc.setAccDecDCCAddrNextReceived(AddrSetModeEnabled);  
  }
}

void notifyCVResetFactoryDefault()
{
  // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset
  // to flag to the loop() function that a reset to Factory Defaults needs to be done
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
};
