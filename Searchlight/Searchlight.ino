/**********************************************************************

CESM_SEARCHLIGHT_CONTROLLER
COPYRIGHT (C) 2017 David J. Cutting

This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
***********************************************************************

CESM_SEARCHLIGHT_CONTROLLER is a C++ program written using Arduino IDE 
  1.8.2 for the following boards/processors:
  
        - CESM Signal Controller ( ATtiny841 )
        - Arduino Uno, Pro Mini, Duemilanove, etc. ( ATtiny168 / ATmega328P )
        - Arduino Mega, Arduino Mega 2560, etc. ( ATmega1280 / ATmega2560 )
        - Arduino Leonardo, Arduino Mini, etc. ( ATmega32u4 )
        
The code can also be compiled for other processors, requiring only a 
  new definition in BoardDefine.h. All pin numbers for these boards can 
  be found in BoardDefine.h.

One wire of the DCC Bus must be connected to DCC_READ_PIN via a 100 ohm 
  resistor or optoisolator. Because of the low cost of a microcontroller, 
  an optoisolator is not usually warranted. More info on the optoisolator
  setup can be found at http://mrrwa.org/dcc-decoder-interface/

***********************************************************************
    
Special thanks to Alex Shepard for his work on this code. Without his
    help this project wouldn't have been possible.

Special thanks to Mike Weber, who graciously provided his wonderful 
    BLMA searchlight driver code, which was heavily modified and
    integrated into this code.

**********************************************************************/

/////////////////////////////////////
//  INCLUDE REQUIRED HEADER FILES  //
/////////////////////////////////////

#include "BoardDefine.h"                                              //  Include the header file that defines the pin numbers, etc.
#include "Config.h"                                                   //  Include the configuration header file
#include "Searchlight.h"                                              //  Include the Searchlight header file

///////////////////////////
//  FUNCTION PROTOTYPES  //
///////////////////////////

void DccBackEndFunc();
void notifyCVResetFactoryDefault();
void notifyCVChange( uint16_t, uint8_t );
void notifyDccMsg( DCC_MSG * Msg );
void leaveColorFunc( uint8_t, byte );
void bypassColorFunc( uint8_t, byte );
void stopAtEnergizedFunc( uint8_t, byte );
void stopAtDeEnergizedFunc( uint8_t, byte=RED );
void setSoftPWMValues( uint8_t, uint8_t, uint8_t, uint8_t, byte=ON );

/////////////
//  SETUP  //
/////////////

void setup() 
{
  pinMode(DCC_READ_PIN, INPUT);                                       //  Set the DCC read pin as an input
  
  #if defined(__AVR_ATtiny841__) 
  pinMode(PROG_JUMPER_PIN, INPUT);                                    //  Set the Programming jumper input as an input ( the CESM signal decoder has a built-in pullup resistor )
  #else
  pinMode(PROG_JUMPER_PIN, INPUT_PULLUP);                             //  Set the Programming jumper input as an input and enable its pullup resistor
  #endif
  
  Dcc.pin( 0, DCC_READ_PIN, 0 );                                      //  Setup which External Interrupt, the Pin it's associated with, and
                                                                      //  that we're using and enable the Pull-Up

  Dcc.init( MAN_ID, 10, FLAGS_DCC_ACCESSORY_DECODER | FLAGS_OUTPUT_ADDRESS_MODE, CV_OPS_MODE_ADDRESS_LSB );   //  Initialize the DCC interface. Set the 
                                                                      //  manufacturer ID to the one set in Config.h, set the CV29 flags, and set the 
                                                                      //  operations mode address.
  
  #ifdef SERIAL_DEBUG
  Serial.begin( DEBUG_BAUD_RATE );                                    //  Start the serial line at baud rate specified in Config.h
  Serial.println(F("\nCESM_Searchlight_Decoder"));
  Serial.print(F("Base Address: "));
  Serial.println(baseAddress);
  #endif

  #ifndef SERIAL_DEBUG
  Palatis::SoftPWM.begin(120);                                         //  Begin soft PWM with 60hz pwm frequency
  #endif

  #ifdef RESET_CVS_ON_POWER
  notifyCVResetFactoryDefault();                                      //  Force Restore to factory defaults whenever decoder is restarted (enable in config)
  #endif

  baseAddress = Dcc.getAddr();                                        //  Preload the decoder address
  commonPole = Dcc.getCV(55);                                         //  Preload commonpole variable

  #ifndef SERIAL_DEBUG
  setSoftPWMValues( 0, 0, 0, 0 );
  delay(5000);
  setSoftPWMValues( 0, 31, 0, 0 );
  delay(5000);
  setSoftPWMValues( 0, 0, 31, 0 );
  delay(5000);
  setSoftPWMValues( 0, 0, 0, 31 );
  delay(5000);
  setSoftPWMValues( 0, 31, 31, 31 );
  delay(5000);
  setSoftPWMValues( 0, colorCache[1].red, colorCache[1].grn, colorCache[1].blu );
  #endif
}

/////////////////
//  MAIN LOOP  //
/////////////////

void loop() 
{
  Dcc.process();                                                      //  Read the DCC bus and process the signal. Needs to be called frequently.
  DccBackEndFunc();                                                   //  Runs all DCC Back-end operations ( separate function keeps code clean )
  
  /////////////////////////////////
  //  VANE SPECIAL EFFECTS CODE  //
  /////////////////////////////////
  
  for(int headIndex = 0; headIndex < NUM_HEADS; headIndex++)          //  For each signal head...
  { 
    if (headStates[headIndex].inputStabilizeCount < 60)               
    {                               
      headStates[headIndex].inputStabilizeCount ++;                   //  Make sure both inputs are stable before executing an animation sequence
    }
    else
    {            
      if( millis() - headStates[headIndex].lastAnimateTime >= ANIMATE_DELAY_TIME ) //  And check if it's time to fire off the next frame of animation (every 20ms)
      { 
        if( headStates[headIndex].headStatus == STATE_ANIMATE ) {
          if( headStates[headIndex].startColor == headStates[headIndex].nextColor )
          {
            #ifdef SERIAL_DEBUG
            Serial.println(": n");
            #endif 
            headStates[headIndex].headStatus = STATE_IDLE;
            headStates[headIndex].currColor = headStates[headIndex].startColor;
            headStates[headIndex].frame = 0;
          }
          
          // If the signal's starting color does not equal it's target color, if the signal is not in a black animation state, 
          // and if the current color of the signal is the starting color of the signal...
          else if( ( headStates[headIndex].startColor != headStates[headIndex].nextColor ) && ( headStates[headIndex].headStatus != STATE_ANIMATE_BLACK )
                                                                          && ( headStates[headIndex].startColor == headStates[headIndex].currColor ) ) 
          {                                                                
            leaveColorFunc( headIndex, headStates[headIndex].startColor );   //  Leave the current color of the signal head (animation)
          #ifdef SERIAL_DEBUG
            Serial.println(": l");
          #endif  
          }
        }
        else if( headStates[headIndex].headStatus == STATE_ANIMATE_BLACK ) // If we're in the middle of an animation transition...
        {
          if( abs( colorCache[headStates[headIndex].currColor].vanePos - colorCache[headStates[headIndex].nextColor].vanePos ) == 1 ) 
          {          // Get vane position for current color                                  // Get vane position for target color           
            // If the signal is currently one lens away from its target, and the next color is red...
            if(headStates[headIndex].nextColor == RED)
            {
              //  If the next color is RED...
              stopAtDeEnergizedFunc( headIndex, RED );                        //  ...move to the red aspect, which is deenergized in most setups (safety precaution).
            #ifdef SERIAL_DEBUG
              Serial.println(": de");
            #endif 
            }
            else 
            {
              //  If the next color is NOT red...
              stopAtEnergizedFunc( headIndex, headStates[headIndex].nextColor );         //  ...move to the specified color, which is energized in most setups.
            #ifdef SERIAL_DEBUG
              Serial.println(": en");
            #endif 
            }
          }
          else if( ( colorCache[headStates[headIndex].currColor].vanePos - colorCache[headStates[headIndex].nextColor].vanePos ) >= 2 ) 
          {          // Get vane position for current color                                  // Get vane position for target color 
            
            bypassColorFunc( headIndex, vaneOrder[colorCache[headStates[headIndex].currColor].vanePos - 1] );
                                          // Look up the vane ID that is one less than the current vane
          #ifdef SERIAL_DEBUG
            Serial.println(":by");
          #endif 
          }
          else if( ( colorCache[headStates[headIndex].currColor].vanePos - colorCache[headStates[headIndex].nextColor].vanePos ) <= -2 ) 
          {          // Get vane position for current color                                  // Get vane position for target color 
            
            bypassColorFunc( headIndex, vaneOrder[colorCache[headStates[headIndex].currColor].vanePos + 1] );
                                          // Look up the vane ID that is one less than the current vane
          #ifdef SERIAL_DEBUG
            Serial.println(":by");
          #endif 
          }
        }
      }
    }
    //  If the signal is not currently animating, and it's either set to flashing or vane plus flashing mode...
    if( ( headStates[headIndex].headStatus != STATE_ANIMATE ) && ( headStates[headIndex].effect == EFFECT_FLASHING ) )
    {
      if( ( millis() - headStates[headIndex].lastAnimateTime ) > FLASH_PULSE_DELAY ) 
      {
        headStates[headIndex].lastAnimateTime = millis();
      }
      if( ( millis() - headStates[headIndex].lastAnimateTime ) <= ( FLASH_PULSE_DELAY / 2 ) )
      { 
        setSoftPWMValues( headIndex, 0, 0, 0, OFF );
      }
      else
      {
        setSoftPWMValues( headIndex, headStates[headIndex].colorInfo.red, headStates[headIndex].colorInfo.blu, headStates[headIndex].colorInfo.blu );
      }   
    }
  }
}

///////////////////////////////////////////
//  DCC SIGNAL PACKET DECODING FUNCTION  //
///////////////////////////////////////////

void notifyDccSigOutputState( uint16_t Addr, uint8_t State )          //  Notifies program of an incoming signal packet
{
  #ifdef  SERIAL_DEBUG
  //Serial.print( F( "notifyDccSigState: Addr: " ) );
  Serial.print( Addr );                                               //  Prints signal address from incoming packet
  //Serial.print( F( " State: " ) );
  Serial.println( State );                                            //  Print aspect number from incoming packet
  #endif
  
  if( ( Addr < baseAddress ) || ( Addr >= ( baseAddress + NUM_HEADS ) ) )     //  Make sure we're only looking at our addresses
  {
    #ifdef  SERIAL_DEBUG
    //Serial.println( F( "notifyDccSigState: Address out of range" ) );
    #endif
    
    return;
  }
  if( State > NUM_ASPECTS )                                           //  Check we've got a valid Aspect
  {
    #ifdef  SERIAL_DEBUG
    //Serial.println( F( "notifyDccSigState: State out of range" ) );
    #endif
    
    return;
  }

  uint8_t headIndex = Addr - baseAddress ;                            //  Determine which head we're talking about (0, 1, 2, ...)
 
  headStates[headIndex].headStatus = STATE_ANIMATE;                   //  Sets the status to idle (not dimming or brightening)
  headStates[headIndex].nextColor = aspectTable[State].colorID;       //  Looks up the color number in the aspect table, stores to head info table.
  headStates[headIndex].effect = aspectTable[State].effect;           //  Looks up the aspect effect and stores it to head info table
  headStates[headIndex].inputStabilizeCount = 0;                      //  Reset the stabilization timer

  #ifdef SERIAL_DEBUG
  Serial.print( F( "notifyDccSigState: Index: " ) );
  Serial.println( headIndex );                                        //  Prints which head we're working with 
  #endif

  #ifdef LIGHT_DEBUG
  setSoftPWMValues( headIndex, colorCache[headStates[headIndex].nextColor].red, colorCache[headStates[headIndex].nextColor].grn, colorCache[headStates[headIndex].nextColor].blu ); 
  #endif
}

#if defined( NOTIFY_DCC_MSG ) && defined( SERIAL_DEBUG )                                            
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

/////////////////////////////////////////////////////////////////////////////
//  DCC BACKEND - RUNS EVERY TIME THE LOOP HAPPENS (FOR CODE CLEANLINESS)  //
/////////////////////////////////////////////////////////////////////////////

void DccBackEndFunc()
{
  ////////////////////////////
  //  RESET ALL CV TRIGGER  //
  ////////////////////////////

  if ( FactoryDefaultCVIndex && Dcc.isSetCVReady()) 
  {
    FactoryDefaultCVIndex--; // Decrement first as initially it is the size of the array
    Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV, FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
    #ifdef SERIAL_DEBUG
    //Serial.print(F("FactoryDefault CV: "));
    //Serial.print( FactoryDefaultCVs[FactoryDefaultCVIndex].CV );
    //Serial.print(F("  Value: "));
    //Serial.println( FactoryDefaultCVs[FactoryDefaultCVIndex].Value );
    #endif

    if(FactoryDefaultCVIndex == 0)
    {    
      baseAddress = Dcc.getAddr();
      #ifdef SERIAL_DEBUG
      //Serial.print(F("Base Address: "));
      //Serial.println(baseAddress);
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
  /*
  #ifdef SERIAL_DEBUG
  Serial.print(F("notifyCVChange CV: "));
  Serial.print( CV );
  Serial.print(F(" Value: "));
  Serial.println( Value );
  #endif
  */
  switch(CV)                                                          //  Changes the locally stored (not EEPROM) values of the CVs if one changes.
  {
    case CV_ACCESSORY_DECODER_ADDRESS_LSB:
      break;
    case CV_ACCESSORY_DECODER_ADDRESS_MSB:
      baseAddress = Dcc.getAddr();
      break;
    case 55:
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
  Dcc.setCV(CV_OPS_MODE_ADDRESS_MSB, (OutputAddr >> 8) & 0x00FF);
  
  AddrSetMode = ADDR_SET_DONE;
}

/////////////////////////////////
//  SOFT PWM SETTING FUNCTION  //
/////////////////////////////////

void setSoftPWMValues( uint8_t headIndex, uint8_t redVal, uint8_t grnVal, uint8_t bluVal, byte updateStored )  //  Sets the PWM values of each LED in a certain head
{
  //  Functions that set the soft PWM values based on the requested value and the common pole variable
  uint8_t rV = abs( redVal - ( 31 * commonPole ) );
  uint8_t gV = abs( grnVal - ( 31 * commonPole ) );
  uint8_t bV = abs( bluVal - ( 31 * commonPole ) );    

#ifdef SERIAL_DEBUG          
  Serial.print("setSoftPWMValues: ");
  Serial.print(redVal);
  Serial.print('-');
  Serial.print(rV);
  Serial.print(' ');
  Serial.print(grnVal);
  Serial.print('-');
  Serial.print(gV);
  Serial.print(' ');
  Serial.print(bluVal);
  Serial.print('-');
  Serial.println(bV);
#else
  Palatis::SoftPWM.set(   headIndex * 3      , rV );
  Palatis::SoftPWM.set( ( headIndex * 3 ) + 1, gV );
  Palatis::SoftPWM.set( ( headIndex * 3 ) + 2, bV );
#endif
  //  Update the head states with the current values of the LEDs
  if(updateStored) {
    headStates[headIndex].colorInfo.red = redVal;
    headStates[headIndex].colorInfo.grn = grnVal;
    headStates[headIndex].colorInfo.blu = bluVal;
  }
}

/////////////////////////////////
//  COLOR ANIMATION FUNCTIONS  //
/////////////////////////////////

void leaveColorFunc( uint8_t headIndex, byte colorToLeave )
{
  if(headStates[headIndex].frame < 12)
  {
    setSoftPWMValues( headIndex, (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToLeave].red / 31, 
                                  (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToLeave].grn / 31, 
                                  (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToLeave].blu / 31 );
    headStates[headIndex].frame ++;
    headStates[headIndex].lastAnimateTime = millis();
    return;
  }
  headStates[headIndex].headStatus = STATE_ANIMATE_BLACK;
  headStates[headIndex].currColor = colorToLeave;
  headStates[headIndex].frame = 0;
}

void bypassColorFunc( uint8_t headIndex, byte colorToPass) 
{
  if(headStates[headIndex].frame < 8) 
  {
    setSoftPWMValues( headIndex, (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToPass].red / 31, 
                                  (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToPass].grn / 31, 
                                  (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToPass].blu / 31 );
    headStates[headIndex].frame ++;
    headStates[headIndex].lastAnimateTime = millis();
    return;
  }
  headStates[headIndex].headStatus = STATE_ANIMATE_BLACK;
  headStates[headIndex].currColor = colorToPass;
  headStates[headIndex].frame = 0;
}

void stopAtEnergizedFunc( uint8_t headIndex, byte colorToStopAt ) 
{
  if(headStates[headIndex].frame < 33) 
  {
    setSoftPWMValues( headIndex, (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToStopAt].red / 31, 
                                  (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToStopAt].grn / 31, 
                                  (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToStopAt].blu / 31 );
    headStates[headIndex].frame ++;
    headStates[headIndex].lastAnimateTime = millis();
    return;
  }
  headStates[headIndex].headStatus = STATE_IDLE;
  headStates[headIndex].startColor = colorToStopAt;
  headStates[headIndex].currColor = colorToStopAt;
  headStates[headIndex].nextColor = colorToStopAt;
  headStates[headIndex].frame = 0;
}

void stopAtDeEnergizedFunc( uint8_t headIndex, byte colorToStopAt ) 
{
  if(headStates[headIndex].frame < 52) 
  {
    setSoftPWMValues( headIndex, (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToStopAt].red / 31, 
                                  (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToStopAt].grn / 31, 
                                  (uint8_t) bypassColor[headStates[headIndex].frame] * colorCache[colorToStopAt].blu / 31 );
    headStates[headIndex].frame ++;
    headStates[headIndex].lastAnimateTime = millis();
    return;
  }
  headStates[headIndex].headStatus = STATE_IDLE;
  headStates[headIndex].startColor = colorToStopAt;
  headStates[headIndex].currColor = colorToStopAt;
  headStates[headIndex].nextColor = colorToStopAt;
  headStates[headIndex].frame = 0;
}
