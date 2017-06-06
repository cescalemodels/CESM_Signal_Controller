/**********************************************************************

Searchlight Driver Code
COPYRIGHT (C) 2017 David J. Cutting

Special thanks to Alex Shepard for his work on this code. Without his
    help this project wouldn't have been possible.

Special thanks to Mike Weber, who graciously provided his wonderful 
    BLMA searchlight driver code, which was heavily modified and
    integrated into this code.

**********************************************************************/

/////////////////////////////////////
//  INCLUDE REQUIRED HEADER FILES  //
/////////////////////////////////////

#include "Searchlight.h"                                              //  Include the Searchlight header file
#include "BoardDefine.h"                                              //  Include the header file that defines the pin numbers, etc.
#include "Config.h"                                                   //  Include the configuration header file

/////////////////////////////////////
//  SET UP EFFECT TIMER VARIABLES  //
///////////////////////////////////// 

int fadeTick = 0;                                                     //  Counter variable that times the dimming of the signal            
#define FADE_STEP_LENGTH 25                                           //  Sets amount of time (milliseconds) between dimming steps
int brightTime;                                                       //  Brightness timer
byte brightCounter;
#define FADE_STEPS 20
#define ANIMATE_DELAY_TIME 20
unsigned long animateTimer[NUM_HEADS] = {0, 0, 0};
unsigned long animateTimerHead1 = 0;
unsigned long animateTimerHead2 = 0; 

////////////////////////////
//  SET UP DCC VARIABLES  //
//////////////////////////// 

ADDR_SET_MODE AddrSetMode = ADDR_SET_DISABLED ;                       //  Keeps track of whether the programming jumper is set

uint16_t baseAddress;                                                 //  Keeps track of the base address of the decoder
uint8_t FactoryDefaultCVIndex = 0;                                    //  Controls reset of the decoder
uint8_t commonPole;                                                   //  Keeps track of whether the decoder is set up as common anode or common cathode

/////////////
//  SETUP  //
/////////////

void setup() 
{
  pinMode(DCC_READ_PIN, INPUT);                                       //  Set the DCC read pin as an input
  pinMode(PROG_JUMPER_PIN, INPUT_PULLUP);                             //  Set the Programming jumper input as an input and enable its pullup resistor
  
  Dcc.pin( 0, DCC_READ_PIN, 0 );                                      //  Setup which External Interrupt, the Pin it's associated with, and
                                                                      //  that we're using and enable the Pull-Up

  Dcc.init( MAN_ID, 10, FLAGS_DCC_ACCESSORY_DECODER | FLAGS_OUTPUT_ADDRESS_MODE, CV_OPS_MODE_ADDRESS_LSB );   //  Initialize the DCC interface. Set the 
                                                                      //  manufacturer ID to the one set in Config.h, set the CV29 flags, and set the 
                                                                      //  operations mode address.
  
  baseAddress = Dcc.getAddr();                                        //  Preload the decoder address
  commonPole = Dcc.getCV(30);                                         //  Preload commonpole variable
  
  #ifdef SERIAL_DEBUG
  Serial.begin( DEBUG_BAUD_RATE );                                    //  Start the serial line at baud rate specified in Config.h
  Serial.println(F("\nCESM_Searchlight_Decoder"));
  Serial.print(F("Base Address: "));
  Serial.println(baseAddress);
  #endif

  #ifndef SERIAL_DEBUG
  Palatis::SoftPWM.begin(60);                                         //  Begin soft PWM with 60hz pwm frequency
  #endif

  #ifdef RESET_CVS_ON_POWER
  notifyCVResetFactoryDefault();                                      //  Force Restore to factory defaults whenever decoder is restarted (enable in config)
  #endif
}

/////////////////////////
//  CV RESET FUNCTION  //
/////////////////////////

void notifyCVResetFactoryDefault()
{
  #ifdef SERIAL_DEBUG
  Serial.println(F("notifyCVResetFactoryDefault"));
  #endif

  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair); //  Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset
                                                                      //  to flag to the loop() function that a reset to Factory Defaults needs 
                                                                      //  to be done
};

//////////////////////////////////////////////////////////////
//  CV PROGRAMMING FUNCTION - Runs if a CV value is change  //
//////////////////////////////////////////////////////////////

void notifyCVChange( uint16_t CV, uint8_t Value )
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

/////////////////////////////////////////////////////////////////////////
//  ADDRESS PROGRAMMING FUNCTION - Runs from loop while jumper is set  //
/////////////////////////////////////////////////////////////////////////

void notifyDccAccOutputAddrSet( uint16_t OutputAddr )
{
  #ifdef SERIAL_DEBUG
  Serial.print(F("notifyDccAccOutputAddrSet Output Addr: "));
  Serial.println( OutputAddr );
  #endif
  
  baseAddress = Dcc.getAddr();
  
  Dcc.setCV(CV_OPS_MODE_ADDRESS_LSB,     OutputAddr & 0x00FF);
  Dcc.setCV(CV_OPS_MODE_ADDRESS_LSB + 1, (OutputAddr >> 8) & 0x00FF);
  
  AddrSetMode = ADDR_SET_DONE;
}

/////////////////////////////////
//  SOFT PWM SETTING FUNCTION  //
/////////////////////////////////

void setSoftPWMValues( uint8_t headNumber, uint8_t redVal, uint8_t grnVal, uint8_t bluVal )  //  Sets the PWM values of each LED in a certain head
{          
  //  Functions that set the soft PWM values based on the requested value and the common pole variable                                                         
  Palatis::SoftPWM.set(   headNumber * 3      , (uint8_t) abs( redVal - ( 63 * commonPole ) ) );
  Palatis::SoftPWM.set( ( headNumber * 3 ) + 1, (uint8_t) abs( grnVal - ( 63 * commonPole ) ) );
  Palatis::SoftPWM.set( ( headNumber * 3 ) + 2, (uint8_t) abs( bluVal - ( 63 * commonPole ) ) );

  //  Update the head states with the current values of the LEDs
  headStates[headNumber].currBlend.red = redVal;
  headStates[headNumber].currBlend.grn = grnVal;
  headStates[headNumber].currBlend.blu = bluVal;
}


///////////////////////////////////////////
//  DCC SIGNAL PACKET DECODING FUNCTION  //
///////////////////////////////////////////

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

  #ifndef SERIAL_DEBUG
  headStates[headIndex].currColor = State;                           //  Stores the state in the current aspect  
  headStates[headIndex].headStatus = STATE_IDLE;                      //  Sets the status to idle (not dimming or brightening)
  headStates[headIndex].currColor = aspectTable[State].lensNumber;     //  Looks up the lens number in the aspect table, stores to head info table.
  headStates[headIndex].on_off = aspectTable[State].on_off;           //  Looks up whether the aspect is on or off, stores to head info table
  headStates[headIndex].effect = aspectTable[State].effect;           //  Looks up the aspect effect and stores it to head info table
  #endif

  #ifdef SERIAL_DEBUG
  Serial.print( F( "notifyDccSigState: Index: " ) );
  Serial.println( headIndex );                                        //  Prints which head we're working with 
  #endif

  #ifdef LIGHT_DEBUG
  setSoftPWMValues( headIndex, colorCache[headStates[headIndex].currColor].red, colorCache[headStates[headIndex].currColor].grn, colorCache[headStates[headIndex].currColor].blu ); 
  #endif
}

#ifdef  SERIAL_DEBUG_NOTIFY_DCC_MSG                                            
void notifyDccMsg( DCC_MSG * Msg )                                    //  Prints all DCC packets to serial line (enable in Config.h)
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

/////////////////
//  MAIN LOOP  //
/////////////////

void loop() 
{
  Dcc.process();                                                //  Read the DCC bus and process the signal. Needs to be called frequently.

  ////////////////////////////
  //  RESET ALL CV TRIGGER  //
  ////////////////////////////

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

  ///////////////////////////
  //  ADDRESS SET TRIGGER  //
  ///////////////////////////

  switch(AddrSetMode)                                             //  Enable Decoder Address Setting from next received Accessory Decoder packet
  {
    case ADDR_SET_DISABLED:
      if(digitalRead(PROG_JUMPER_PIN) == LOW)
      {
        #ifdef SERIAL_DEBUG
        Serial.println(F("Enable DCC Address Set Mode"));
        #endif
        AddrSetMode = ADDR_SET_ENABLED;
        Dcc.setAccDecDCCAddrNextReceived(1);  
      }
      break;
    default:
      if(digitalRead(PROG_JUMPER_PIN) == HIGH)
      {
        #ifdef SERIAL_DEBUG
        Serial.println(F("Disable DCC Address Set Mode"));
        #endif
        if(AddrSetMode == ADDR_SET_ENABLED)
          Dcc.setAccDecDCCAddrNextReceived(0);
            
        AddrSetMode = ADDR_SET_DISABLED;
      }
      break;
  }

  ////////////////////////////
  //  SPECIAL EFFECTS CODE  //
  ////////////////////////////
  
  #if !defined( SERIAL_DEBUG ) && !defined( LIGHT_DEBUG ) && !defined( SERIAL_DEBUG_NOTIFY_DCC_MSG )
  
  for(int headIndex = 0; headIndex < NUM_HEADS; headIndex++) 
  { 
    if (headStates[headIndex].inputStabilizeCount < 60)
    {                               
      headStates[headIndex].inputStabilizeCount ++;                   //  Make sure both inputs are stable before calculating requests
    }
    //  Check if head is actively animating or is between consecutive segments of animation.
    else if( headStates[headIndex].headStatus == STATE_ANIMATE || headStates[headIndex].headStatus == STATE_ANIMATE_BLACK )  
    {            
      if( millis() - headStates[headIndex].lastAnimateTime >= ANIMATE_DELAY_TIME ) //  And check if it's time to fire off the next frame of animation (every 20ms)
      { 
        // If the signal's starting color does not equal it's target color, if the signal is not in a black animation state, 
        // and if the current color of the signal is the starting color of the signal...
        if( ( headStates[headIndex].startColor != headStates[headIndex].nextColor ) && ( headStates[headIndex].headState != STATE_ANIMATE_BLACK )
                                                                        && ( headStates[headIndex].startColor == headStates[headIndex].currColor ) ) 
        {                                                                
          leaveColorFunc( headIndex, headStates[headIndex].startColor )   //  Leave the current color of the signal head (animation)
        }
        else if( headStates[headIndex].tempColor == BLACK ) // If we're in the middle of an animation transition...
        {
          if( abs( headStates[headIndex].colorCache[headStates[headIndex].currColor].vanePos - headStates[headIndex].colorCache[headStates[headIndex].nextColor].vanePos ) == 1 ) 
          {          // Get vane position for current color                                  // Get vane position for target color           
            // If the signal is currently one lens away from its target, and the next color is red...
            if(headStates[headIndex].nextColor == RED)
            {
              //  If the next color is RED...
              stopAtDeEnergizedFunc( headIndex, RED );                        //  ...move to the red aspect, which is deenergized in most setups (safety precaution).
            }
            else 
            {
              //  If the next color is NOT red...
              stopAtEnergizedFunc( headIndex, headStates[headIndex].nextColor );         //  ...move to the specified color, which is energized in most setups.
            }
          }
          else if( ( headStates[headIndex].colorCache[headStates[headIndex].currColor].vanePos - headStates[headIndex].colorCache[headStates[headIndex].nextColor].vanePos ) >= 2 ) 
          {          // Get vane position for current color                                  // Get vane position for target color 
            
            bypassColorFunc( headNumber, vaneOrder[headStates[headIndex].colorCache[headStates[headIndex].currColor].vanePos - 1] );
                                          // Look up the vane ID that is one less than the current vane
          }
          else if( ( headStates[headIndex].colorCache[headStates[headIndex].currColor].vanePos - headStates[headIndex].colorCache[headStates[headIndex].nextColor].vanePos ) <= -2 ) 
          {          // Get vane position for current color                                  // Get vane position for target color 
            
            bypassColorFunc( headNumber, vaneOrder[headStates[headIndex].colorCache[headStates[headIndex].currColor].vanePos + 1] );
                                          // Look up the vane ID that is one less than the current vane
          }
        }
      }
    }
  }
  #endif
}
