#include <NmraDcc.h>

NmraDcc  Dcc ;
DCC_MSG  Packet ;

const int ledA = PIN_A0;
const int ledB = PIN_A1;
const int ledC = PIN_A2;
const int ledD = PIN_A3;
const int ledE = PIN_A4;
const int ledF = PIN_A5;
const int ledG = PIN_A6;
const int ledH = PIN_A7;
const int ledI = PIN_B0;
const int dccRcv = PIN_B1;
const int prgJump = PIN_B2;

// Variables for PWM generation and timing
int timer = 0; // Counts up to 100 and then resets to 0 in ISR
int on = 0; // Inverts all LED polarities based on CV60
int ledfreq[] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // Stores current percentages for all LEDs
int stepLength = 25; // Amount of time between steps

struct CVPair
{
  uint16_t  CV;
  uint8_t   Value;
};

CVPair FactoryDefaultCVs [] =
{
  {CV_ACCESSORY_DECODER_ADDRESS_LSB, 40},
  {CV_ACCESSORY_DECODER_ADDRESS_MSB, 0},
};

uint8_t FactoryDefaultCVIndex = 0;

void notifyCVResetFactoryDefault()
{
  // Make FactoryDefaultCVIndex non-zero and equal to num CV's to be reset
  // to flag to the loop() function that a reset to Factory Defaults needs to be done
  FactoryDefaultCVIndex = sizeof(FactoryDefaultCVs) / sizeof(CVPair);
};

void notifyDccSigState( uint16_t Addr, uint8_t headIndex, uint8_t State) {
  // Whenever a DCC Signal Aspect Packet is received, set the head state for the recieved head
  setHeadState(headIndex, State);
}

void setHeadState(int function, int State) {
  if(State == 0) 
  {
    digitalWrite(ledA, LOW);
    digitalWrite(ledB, LOW);
    digitalWrite(ledC, LOW);
  }
  if(State == 1)
  {
    digitalWrite(ledA, HIGH);
    digitalWrite(ledB, LOW);
    digitalWrite(ledC, LOW);
  }
  if(State == 2)
  {
    digitalWrite(ledA, LOW);
    digitalWrite(ledB, HIGH);
    digitalWrite(ledC, LOW);
  }
  if(State == 3)
  {
    digitalWrite(ledA, LOW);
    digitalWrite(ledB, LOW);
    digitalWrite(ledC, HIGH);
  }
}

void setup() {
  // Setup which External Interrupt, the Pin it's associated with that we're using and enable the Pull-Up
  pinMode(ledA, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(ledC, OUTPUT);
  pinMode(ledD, OUTPUT);
  pinMode(ledE, OUTPUT);
  pinMode(ledF, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledH, OUTPUT);
  pinMode(ledI, OUTPUT);
  pinMode(dccRcv, INPUT);
  pinMode(prgJump, INPUT);
  Dcc.pin(0, dccRcv, 1);
  Dcc.init( MAN_ID_DIY, 10, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE, 0 );
}

void loop() {
  
  Dcc.process();
  
  if ( FactoryDefaultCVIndex && Dcc.isSetCVReady()) {
    FactoryDefaultCVIndex--; // Decrement first as initially it is the size of the array
    Dcc.setCV( FactoryDefaultCVs[FactoryDefaultCVIndex].CV, FactoryDefaultCVs[FactoryDefaultCVIndex].Value);
  }
}
