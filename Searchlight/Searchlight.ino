

#include <NmraDcc.h>                                                  //  Include the NMRADcc library from Alex Shepard
#include <SoftPWM.h>                                                  //  Include SoftPWM library from here: https://github.com/Palatis/arduino-softpwm/ 

#include "Searchlight.h"                                              //  Include the Searchlight header file
#include "Config.h"                                                   //  Include the Configuration header file

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

// Because most DCC Command Stations don't support the DCC Accessory Decoder OPS Mode Programming
// we fake a DCC Mobile Decoder Address for OPS Mode Programming
#define CV_OPS_MODE_ADDRESS_LSB 33

int on_off[NUM_HEADS];

CVPair FactoryDefaultCVs[] =
{
  {CV_ACCESSORY_DECODER_ADDRESS_LSB, DEFAULT_ADDRESS},
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},
  {CV_OPS_MODE_ADDRESS_LSB,       0xB8},	// 0x22B8 = 8888 Decimal for OPS Mode Programming  // 0x270F = 9999 Decimal for OPS Mode Programming
  {CV_OPS_MODE_ADDRESS_LSB+1,     0x22},
  
  {30, 0},          //  Set decoder to common Anode       

  {35, 0},        //  Dark aspect red LED intensity
  {36, 0},          //  Dark aspect green LED intensity
  {37, 0},          //  Dark aspect blue LED intensity
  
  {38, 63},        //  Red aspect red LED intensity
  {39, 0},          //  Red aspect green LED intensity
  {40, 0},          //  Red aspect blue LED intensity
  
  {41, 0},          //  Green aspect red LED intensity
  {42, 63},        //  Green aspect green LED intensity
  {43, 0},          //  Green aspect blue LED intensity
  
  {44, 63},        //  Yellow aspect red LED intensity
  {45, 63},        //  Yellow aspect green LED intensity
  {46, 0},          //  Yellow aspect blue LED intensity
  
  {47, 63},        //  Lunar aspect red LED intensity
  {48, 63},        //  Lunar aspect green LED intensity
  {49, 63},        //  Lunar aspect blue LED intensity
  
  {50, 1},          //  Head A enable/disable fading
  {51, 1},          //  Head B enable/disable fading
  {52, 1},          //  Head C enable/disable fading
  
  {53, 1},          //  Head A enable/disable vane movement
  {54, 1},          //  Head B enable/disable vane movement
  {55, 1},          //  Head C enable/disable vane movement
  
  {56, 5},          //  Head A fade steps per tick
  {57, 5},          //  Head B fade steps per tick
  {58, 5},          //  Head C fade steps per tick

  {59, 2},          //  Head A vane steps per tick
  {60, 2},          //  Head B vane steps per tick
  {61, 2},          //  Head C vane steps per tick   

  {66, RED},        //  De-energized solenoid color for all heads (DARK = 0, RED = 1, GREEN = 2, YELLOW = 3, LUNAR = 4)

  {67, 1},         //  Flashes per second, signal head A (more like number of on and offs per second)
  {68, 1},         //  Flashes per second, signal head B (more like number of on and offs per second)  
  {69, 1},         //  Flashes per second, signal head C (more like number of on and offs per second)
};

void notifyDccSigOutputState( uint16_t Addr, uint8_t State) //TODO: 0 or 1 based determination for headIndex
{
  #ifdef  DEBUG
  Serial.print(F("notifyDccSigState: Addr: "));
  Serial.print(Addr);
  Serial.print(F(" State: "));
  Serial.println(State);
  #endif
  
  if( (Addr < baseAddress) || (Addr >= (baseAddress + NUM_HEADS)))  // Make sure we're only looking at our addresses
  {
    #ifdef  DEBUG
    Serial.println(F("notifyDccSigState: Address out of range"));
    #endif
    
    return;
  }
  if( State > NUM_ASPECTS)  // Check we've got a valid Aspect
  {
    #ifdef  DEBUG
    Serial.println(F("notifyDccSigState: State out of range"));
    #endif
    
    return;
  }

  uint8_t headIndex = Addr - baseAddress ;
  
  #ifdef  DEBUG
  Serial.print(F("notifyDccSigState: Index: "));
  Serial.println(headIndex);
  #endif
 
  headStates[headIndex].currLens = aspectTable[State].lensNumber;
  on_off[headIndex] = aspectTable[State].on_off;

  #ifdef DEBUG
  Palatis::SoftPWM.set( headIndex * 3,      (uint8_t) Dcc.getCV(38 + ( ( headStates[headIndex].currLens - 1) * NUM_LED_PER_HEAD )) );
  Palatis::SoftPWM.set(( headIndex * 3) + 1, (uint8_t) Dcc.getCV(38 + ( ( headStates[headIndex].currLens - 1) * NUM_LED_PER_HEAD ) + 1));
  Palatis::SoftPWM.set(( headIndex * 3) + 2, (uint8_t) Dcc.getCV(38 + ( ( headStates[headIndex].currLens - 1) * NUM_LED_PER_HEAD ) + 2));
  #endif
  
  headStates[headIndex].prevAspect = headStates[headIndex].currAspect;
  headStates[headIndex].currAspect = State;
  headStates[headIndex].headStatus = STATE_IDLE;
}

#ifdef  NOTIFY_DCC_MSG
void notifyDccMsg( DCC_MSG * Msg )
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

void notifyCVChange( uint16_t CV, uint8_t Value ) 
{
  switch(CV)
  {
    case CV_ACCESSORY_DECODER_ADDRESS_LSB:
    case CV_ACCESSORY_DECODER_ADDRESS_MSB:
      baseAddress = Dcc.getAddr();
      break;

    case 30:
      commonPole = Value;
  }
}

void notifyDccAccOutputAddrSet( uint16_t OutputAddr)
{
  #ifdef DEBUG
  Serial.print(F("notifyDccAccOutputAddrSet Output Addr: "));
  Serial.println( OutputAddr );
  #endif
  
  baseAddress = Dcc.getAddr();
  
  Dcc.setCV(CV_OPS_MODE_ADDRESS_LSB,     OutputAddr & 0x00FF);
  Dcc.setCV(CV_OPS_MODE_ADDRESS_LSB + 1, (OutputAddr >> 8) & 0x00FF);
  
  AddrSetModeEnabled = 0;
}

void setup() 
{
  #ifdef DEBUG
  Serial.begin( 115200 );
  Serial.println(F("\nCESM_Searchlight_Decoder"));
  #endif
 
  pinMode(DCC_READ_PIN, INPUT);
  pinMode(PROG_JUMPER_PIN, INPUT_PULLUP);
  
  Dcc.pin( 0, 1, 0 );  // Setup which External Interrupt, the Pin it's associated with that we're using and enable the Pull-Up
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

  if(fadeTick)
  {  
    fadeTick = 0;
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
}

void notifyCVResetFactoryDefault()
{
  // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset
  // to flag to the loop() function that a reset to Factory Defaults needs to be done
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
};
