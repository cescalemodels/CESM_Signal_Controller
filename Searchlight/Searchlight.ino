/**********************************************************************

Searchlight.ino
COPYRIGHT (C) 2017 David J. Cutting, Alex Shepard

**********************************************************************/

#include "Searchlight.h"                                              //  Include the Searchlight header file
#include "Config.h"                                                   //  Include the Configuration header file

#define DCC_READ_PIN  PIN_B1                                          //  Pin number for the pin that reads the DCC signal
#define PROG_JUMPER_PIN PIN_B2                                        //  Pin number that detects if decoder is in programming mode

int fadeTick = 0;                                                     //  Counter variable that times the dimming of the signal            
int stepLength = 25;                                                  //  Sets amount of time (microseconds) between dimming steps

uint16_t baseAddress = 40;                                            //  Keeps track of the base address of the decoder
uint8_t AddrSetModeEnabled = 0;                                       //  Keeps track of whether the programming jumper is set
uint8_t FactoryDefaultCVIndex = 0;                                    //  Controls reset of the decoder
uint8_t commonPole;                                                   //  Keeps track of whether the decoder is set up as common anode or common cathode based on CVs

#ifndef SERIAL_DEBUG                                                         
SOFTPWM_DEFINE_CHANNEL(0, DDRA, PORTA, PORTA0);                       //  Creates soft PWM channel 0 on Port A0 (Pin 1)
SOFTPWM_DEFINE_CHANNEL(1, DDRA, PORTA, PORTA1);                       //  Creates soft PWM channel 1 on Port A1 (Pin 2)
SOFTPWM_DEFINE_CHANNEL(2, DDRA, PORTA, PORTA2);                       //  Creates soft PWM channel 2 on Port A2 (Pin 3)
SOFTPWM_DEFINE_CHANNEL(3, DDRA, PORTA, PORTA3);                       //  Creates soft PWM channel 3 on Port A3 (Pin 4)
SOFTPWM_DEFINE_CHANNEL(4, DDRA, PORTA, PORTA4);                       //  Creates soft PWM channel 4 on Port A4 (Pin 5)
SOFTPWM_DEFINE_CHANNEL(5, DDRA, PORTA, PORTA5);                       //  Creates soft PWM channel 5 on Port A5 (Pin 6)
SOFTPWM_DEFINE_CHANNEL(6, DDRA, PORTA, PORTA6);                       //  Creates soft PWM channel 6 on Port A6 (Pin 7)
SOFTPWM_DEFINE_CHANNEL(7, DDRA, PORTA, PORTA7);                       //  Creates soft PWM channel 7 on Port A7 (Pin 8)
SOFTPWM_DEFINE_CHANNEL(8, DDRB, PORTB, PORTB0);                       //  Creates soft PWM channel 8 on Port B0 (Pin 9)
SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS(9, 64);                         //  Defines the 9 soft PWM channels with 64 step resolution
#endif

#ifndef SERIAL_DEBUG
void setSoftPWMValues( uint8_t headNumber, uint8_t redVal, uint8_t grnVal, uint8_t bluVal )  //  Sets the PWM values of each LED in a certain head
{                                                                   
  Palatis::SoftPWM.set(   headNumber * 3      , (uint8_t) abs( redVal - ( 63 * commonPole ) ) );
  Palatis::SoftPWM.set( ( headNumber * 3 ) + 1, (uint8_t) abs( grnVal - ( 63 * commonPole ) ) );
  Palatis::SoftPWM.set( ( headNumber * 3 ) + 2, (uint8_t) abs( bluVal - ( 63 * commonPole ) ) );
}
#endif

void notifyDccSigOutputState( uint16_t Addr, uint8_t State )          //  Notifies program of an incoming signal packet
{
  #ifdef  SERIAL_DEBUG
  Serial.print( F( "notifyDccSigState: Addr: " ) );
  Serial.print( Addr );                                               //  Prints signal address from incoming packet
  Serial.print( F( " State: " ) );
  Serial.println( State );                                            //  Print aspect number from incoming packet
  #endif
  
  if( ( Addr < baseAddress ) || ( Addr >= ( baseAddress + NUM_HEADS ) ) )     //  Make sure we're only looking at our addresses
  {
    #ifdef  SERIAL_DEBUG
    Serial.println( F( "notifyDccSigState: Address out of range" ) );
    #endif
    
    return;
  }
  if( State > NUM_ASPECTS )                                           //  Check we've got a valid Aspect
  {
    #ifdef  SERIAL_DEBUG
    Serial.println( F( "notifyDccSigState: State out of range" ) );
    #endif
    
    return;
  }

  uint8_t headIndex = Addr - baseAddress ;                            //  Determines which head we're talking about (0, 1, 2, ...)

  headStates[headIndex].prevAspect = headStates[headIndex].currAspect;//  Stores the old aspect in the aspect archive
  headStates[headIndex].currAspect = State;                           //  Stores the state in the current aspect  
  headStates[headIndex].headStatus = STATE_IDLE;                      //  Sets the status to idle (not dimming or brightening)
  headStates[headIndex].currLens = aspectTable[State].lensNumber;     //  Looks up the lens number in the aspect table, stores to head info table.
  headStates[headIndex].on_off = aspectTable[State].on_off;           //  Looks up whether the aspect is on or off, stores to head info table
  headStates[headIndex].effect = aspectTable[State].effect;           //  Looks up the aspect effect and stores it to head info table

  #ifdef SERIAL_DEBUG
  Serial.print( F( "notifyDccSigState: Index: " ) );
  Serial.println( headIndex );                                        //  Prints which head we're working with 
  #endif

  #ifdef LIGHT_DEBUG
  setSoftPWMValues( headIndex, colorCache[headStates[headIndex].currLens].red, colorCache[headStates[headIndex].currLens].grn, colorCache[headStates[headIndex].currLens].blu ); 
  #endif
}

#if defined( NOTIFY_DCC_MSG ) && defined( SERIAL_DEBUG )                                             
void notifyDccMsg( DCC_MSG * Msg )                                    //  Prints all DCC packets to serial line
{
  Serial.print( F( "notifyDccMsg: " ) );
  for( uint8_t i = 0; i < Msg->Size; i++ )
  {
    Serial.print( Msg->Data[i], HEX );
    Serial.write( ' ' );
  }
  Serial.println();
}
#endif

void notifyCVChange( uint16_t CV, uint8_t Value )                     //  Runs if a CV value is changed
{
  #ifdef SERIAL_DEBUG
  Serial.print(F("notifyCVChange CV: "));
  Serial.print( CV );
  Serial.print(F(" Value: "));
  Serial.println( Value );
  #endif
  
  switch(CV)                                                          //  Changes the locally stored (not EEPROM) values of the CVs if one changes.
  {
    case CV_ACCESSORY_DECODER_ADDRESS_LSB:
      break;
    case CV_ACCESSORY_DECODER_ADDRESS_MSB:
      baseAddress = Dcc.getAddr();
      break;
    case 30:
      commonPole = Value;
      break;
  }
  if( ( CV >= 35 ) && ( CV <= 49 ) )
  {
    uint8_t CVindex = CV - 35;
    uint8_t row = 0;
    while( CVindex >= 3 ) 
    {
      CVindex -= 3;
      row++;
    }
    if( CVindex == 0 )
    {
      colorCache[row].red = Value;
    }
    else if( CVindex == 1 )
    {
      colorCache[row].grn = Value;
    }
    else if( CVindex == 2 )
    {
      colorCache[row].blu = Value;
    }
  }
}

void notifyDccAccOutputAddrSet( uint16_t OutputAddr )
{
  #ifdef SERIAL_DEBUG
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
  #ifdef SERIAL_DEBUG
  Serial.begin( 115200 );
  Serial.println(F("\nCESM_Searchlight_Decoder"));
  Serial.print(F("Base Address: "));
  Serial.println(baseAddress);
  #endif
 
  pinMode(DCC_READ_PIN, INPUT);
  pinMode(PROG_JUMPER_PIN, INPUT);
  
  Dcc.pin( 0, DCC_READ_PIN, 0 );  // Setup which External Interrupt, the Pin it's associated with that we're using and enable the Pull-Up
  Dcc.init( MAN_ID_DIY, 10, FLAGS_DCC_ACCESSORY_DECODER | FLAGS_OUTPUT_ADDRESS_MODE, CV_OPS_MODE_ADDRESS_LSB );

  commonPole = Dcc.getCV(30);
  
  baseAddress = Dcc.getAddr(); // this is broken for now      TODO: Fix or replace

  #ifndef SERIAL_DEBUG
  Palatis::SoftPWM.begin(60); // begin with 60hz pwm frequency
  #endif

  #ifdef RESET_CVS_ON_POWER
  notifyCVResetFactoryDefault(); // Force Restore to Factory Defaults Every time the Decoder is Restarted
  #endif
}

void loop() 
{
  Dcc.process();                                                // Read the DCC bus and process the signal. Needs to be called frequently.

  #if !defined( SERIAL_DEBUG ) && !defined( LIGHT_DEBUG )
  for(int headIndex = 0; headIndex < NUM_HEADS; headIndex++) 
  {
    switch( headStates[headIndex].headStatus )
    {
      case STATE_IDLE:
        
        break;
      case STATE_DIMMING:
        break;
      case STATE_BRIGHTENING:
        break;
    }
  }
  #endif
  
  if ( FactoryDefaultCVIndex && Dcc.isSetCVReady()) 
  {
    FactoryDefaultCVIndex--; // Decrement first as initially it is the size of the array
    Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV, FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
    #ifdef SERIAL_DEBUG
    Serial.print(F("FactoryDefault CV: "));
    Serial.print( FactoryDefaultCVs[FactoryDefaultCVIndex].CV );
    Serial.print(F("  Value: "));
    Serial.println( FactoryDefaultCVs[FactoryDefaultCVIndex].Value );
    #endif

    if(FactoryDefaultCVIndex == 0)
    {    
      baseAddress = Dcc.getAddr();
      #ifdef SERIAL_DEBUG
      Serial.print(F("Base Address: "));
      Serial.println(baseAddress);
      #endif
    }
  }

  // Enable Decoder Address Setting from next received Accessory Decoder packet
  if( !AddrSetModeEnabled && digitalRead(PROG_JUMPER_PIN) == LOW)
  {
    #ifdef SERIAL_DEBUG
    Serial.println(F("Enable DCC Address Set Mode"));
    #endif
    AddrSetModeEnabled = 1;
    Dcc.setAccDecDCCAddrNextReceived(AddrSetModeEnabled);  
  }

  if( AddrSetModeEnabled && digitalRead(PROG_JUMPER_PIN) == HIGH)
  {
    #ifdef SERIAL_DEBUG
    Serial.println(F("Disable DCC Address Set Mode"));
    #endif
    AddrSetModeEnabled = 0;
    Dcc.setAccDecDCCAddrNextReceived(AddrSetModeEnabled);  
  }
}

void notifyCVResetFactoryDefault()
{
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair); // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset
                                                                      // to flag to the loop() function that a reset to Factory Defaults needs 
                                                                      // to be done
};
