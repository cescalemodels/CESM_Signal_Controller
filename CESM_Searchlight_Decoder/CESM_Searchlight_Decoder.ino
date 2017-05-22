#include <NmraDcc.h>
#include "CESM_Searchlight_Decoder.h"
#include "Config.h"

NmraDcc Dcc;
DCC_MSG Packet;

int fadeTick = 0;
int pwmTimer = 255;                                         // Counts up to 255 and then resets to 0 in ISR
int stepLength = 25;                                        // Amount of time between steps
byte ledInterval[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t FactoryDefaultCVIndex = 0;

uint8_t  commonPole ;
uint16_t baseAddress ;

CVPair FactoryDefaultCVs[] =
{
  {CV_ACCESSORY_DECODER_ADDRESS_LSB, DEFAULT_ADDRESS},
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},

  {30, 0},          //  Set decoder to common Anode       
  
  {38, 255},        //  Red aspect red LED intensity
  {39, 0},          //  Red aspect green LED intensity
  {40, 0},          //  Red aspect blue LED intensity
  
  {41, 0},          //  Green aspect red LED intensity
  {42, 255},        //  Green aspect green LED intensity
  {43, 0},          //  Green aspect blue LED intensity
  
  {44, 255},        //  Yellow aspect red LED intensity
  {45, 255},        //  Yellow aspect green LED intensity
  {46, 0},          //  Yellow aspect blue LED intensity
  
  {47, 255},        //  Lunar aspect red LED intensity
  {48, 255},        //  Lunar aspect green LED intensity
  {49, 255},        //  Lunar aspect blue LED intensity
  
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

  {67, 50},         //  Period of flashing for signal head A (0 = disabled, 50 = 1 flash per second, 100 = 2 flash per second)
  {68, 50},         //  Period of flashing for signal head B (0 = disabled, 50 = 1 flash per second, 100 = 2 flash per second)   
  {69, 50},         //  Period of flashing for signal head C (0 = disabled, 50 = 1 flash per second, 100 = 2 flash per second)
};

void notifyDccSigState( uint16_t Addr, uint8_t headIndex, uint8_t State) //TODO: 0 or 1 based determination for headIndex
{
  if( (Addr < baseAddress) || (Addr >= (baseAddress + NUM_HEADS)))  // Make sure we're only looking at our addresses
    return;

  if( State > NUM_ASPECTS)  // Check we've got a valid Aspect
    return; 
  
  headIndex = Addr - baseAddress; // Compute your own headIndex for now as the library is probably NOT doing the right thing...

  headStates[headIndex].currLens = aspectTable[State].lensNumber;
  
  ledInterval[ headIndex * 3 ] = Dcc.getCV(38 + ( headStates[headIndex].currLens * NUM_LENSES ));
  ledInterval[ (headIndex * 3) + 1 ] = Dcc.getCV(38 + ( headStates[headIndex].currLens * NUM_LENSES ) + 1);
  ledInterval[ (headIndex * 3) + 2 ] = Dcc.getCV(38 + ( headStates[headIndex].currLens * NUM_LENSES ) + 2);
  
  headStates[headIndex].prevAspect = headStates[headIndex].currAspect;
  headStates[headIndex].currAspect = State;
  headStates[headIndex].headStatus = STATE_IDLE;
}

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

ISR(TIM0_OVF_vect) 
{
  for(int i = 0; i < NUM_PINS; i++) 
  {
    if(ledInterval[i] < pwmTimer)                       //  If the pin's current PWM value is less than 
    {
      if(commonPole == 0) 
      { 
        digitalWrite(LED_CONTROL_PINS[i], HIGH);    //  Set the current pin to HIGH if the LED is common anode
      }
      else 
      { 
        digitalWrite(LED_CONTROL_PINS[i], LOW);     //  Set the current pin to LOW if the LED is common cathode
      }
    }
    else                                           
    {
      if(commonPole == 0) 
      { 
        digitalWrite(LED_CONTROL_PINS[i], LOW);     //  Set the current pin to LOW if the LED is common anode
      }
      else 
      { 
        digitalWrite(LED_CONTROL_PINS[i], HIGH);    //  Set the current pin to HIGH if the LED is common cathode
      }
    }
  }

  fadeTick++;                                       //  Increment variable that controls whether next fade step is take in the loop.r
  pwmTimer++;                                       //  Increment the PWM timer variable  

  if(pwmTimer > 255) {                              
    pwmTimer = 0;                                   //  Reset the PWM timer variable to zero when it gets above 255
  }
}

void setup() 
{
  for(int i = 0; i < NUM_PINS; i++) {
    pinMode(LED_CONTROL_PINS[i], OUTPUT);
  }
  
  pinMode(DCC_READ_PIN, INPUT);
  pinMode(PROG_JUMPER_PIN, INPUT);
  
  Dcc.pin(0, DCC_READ_PIN, 1);  // Setup which External Interrupt, the Pin it's associated with that we're using and enable the Pull-Up
  Dcc.init( MAN_ID_DIY, 10, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE, 0 );

  commonPole = Dcc.getCV(30);
  baseAddress = Dcc.getAddr();

  bitClear(TCCR0A, COM0A1);
  bitClear(TCCR0A, COM0A0);
  bitClear(TCCR0A, COM0B1);
  bitClear(TCCR0A, COM0B0);

  bitClear(TCCR0A, WGM00);
  bitClear(TCCR0A, WGM01);
  bitClear(TCCR0B, WGM02);

  // We need a clock frequency of ~1 millisecond (1000 microseconds) to get a good balance of performance. 
  // The prescaler needs to be 64 since:|   1/8,000,000  *      1,000,000          *    64     *       255      =       2048 microseconds       |
  //                                    | Clock freqency | Seconds to Microseconds | Prescaler | 8-bit Overflow | Pretty close to 1 millisecond |
  
  bitSet(TCCR0B, CS02);
  bitSet(TCCR0B, CS01);
  bitClear(TCCR0B, CS00);

  bitClear(TIMSK0, OCIE0B);
  bitClear(TIMSK0, OCIE0A);
  bitSet(TIMSK0, TOIE0);
}

void loop() 
{
  Dcc.process();

  for(int i = 0; i < NUM_HEADS; i++) 
  {
    switch( headStates[i].headStatus )
    {
      case STATE_IDLE:
        
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
  }
}

void notifyCVResetFactoryDefault()
{
  // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset
  // to flag to the loop() function that a reset to Factory Defaults needs to be done
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
};
