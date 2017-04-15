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

  {35, 1}, // LEDs are set to searchlight mode
  
  {60, 1}, // LEDs are common cathode

  {61, 100}, // Discrete aspect 1 brightness
  {62, 100}, // Discrete aspect 2 brightness
  {63, 100}, // Discrete aspect 3 brightness
  {64, 100}, // Discrete aspect 4 brightness
  
  {70, 100}, // Reds are 100% on when RGB heads are red
  {71, 0}, // Reds are 0% on when RGB heads are green
  {72, 0}, // Reds are 0% on when RGB heads are yellow

  {73, 0}, // Greens are 0% on when RGB heads are red
  {74, 100}, // Greens are 100% on when RGB heads are green
  {75, 0}, // Greens are 0% on when RGB heads are yellow

  {76, 100}, // Blues are 100% on when RGB heads are red
  {77, 100}, // Blues are 100% on when RGB heads are green
  {78, 20}, // Blues are 20% on when RGB heads are yellow

  {79, 80}, // Reds are 80% on when RGB heads are lunar
  {80, 80}, // Greens are 80% on when RGB heads are lunar
  {81, 100}, // Blues are 100% on when RGB heads are lunar
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

void setHeadState(int headIndex, int State) {
  if(Dcc.getCV(35) == 1) 
  {
    if(State == 0) 
    {
        ledfreq[0 + (headIndex+1)*3] = 0;
        ledfreq[1 + (headIndex+1)*3] = 0;
        ledfreq[2 + (headIndex+1)*3] = 0;
    }
    if(State == 1 || State == 2 || State == 3) 
    {
      ledfreq[0 + (headIndex+1)*3] = Dcc.getCV(70 + State - 1);
      ledfreq[1 + (headIndex+1)*3] = Dcc.getCV(73 + State - 1);
      ledfreq[2 + (headIndex+1)*3] = Dcc.getCV(76 + State - 1);
    }
    else if(State == 4) 
    {
      ledfreq[0 + (headIndex+1)*3] = Dcc.getCV(79);
      ledfreq[1 + (headIndex+1)*3] = Dcc.getCV(80);
      ledfreq[2 + (headIndex+1)*3] = Dcc.getCV(81);
    }
  }
  
  if(Dcc.getCV(35) == 0) 
  {
    if(State == 1)
    {
      ledfreq[0 + (headIndex)*3] = Dcc.getCV(61);
      ledfreq[1 + (headIndex)*3] = 0;
      ledfreq[2 + (headIndex)*3] = 0;
    }
    if(State == 2)
    {
      ledfreq[0 + (headIndex)*3] = 0;
      ledfreq[1 + (headIndex)*3] = Dcc.getCV(62);
      ledfreq[2 + (headIndex)*3] = 0;
    }
    if(State == 3)
    {
      ledfreq[0 + (headIndex)*3] = 0;
      ledfreq[1 + (headIndex)*3] = 0;
      ledfreq[2 + (headIndex)*3] = Dcc.getCV(63);
    }
  }
  
  if(Dcc.getCV(35) == 2) 
  {
    if(headIndex == 0 || headIndex == 1) 
    {
      if(State == 1)
      {
        ledfreq[0 + headIndex*4] = Dcc.getCV(61);
        ledfreq[1 + headIndex*4] = 0;
        ledfreq[2 + headIndex*4] = 0;
        ledfreq[3 + headIndex*4] = 0;
      }
      else if(State == 2)
      {
        ledfreq[0 + headIndex*4] = 0;
        ledfreq[1 + headIndex*4] = Dcc.getCV(62);
        ledfreq[2 + headIndex*4] = 0;
        ledfreq[3 + headIndex*4] = 0;
      }
      else if(State == 3)
      {
        ledfreq[0 + headIndex*4] = Dcc.getCV(63);
        ledfreq[1 + headIndex*4] = 0;
        ledfreq[2 + headIndex*4] = 0;
        ledfreq[3 + headIndex*4] = 0;
      }
      else if(State == 4)
      {
        ledfreq[0 + headIndex*4] = Dcc.getCV(63);
        ledfreq[1 + headIndex*4] = 0;
        ledfreq[2 + headIndex*4] = 0;
        ledfreq[3 + headIndex*4] = 0;
      }
      else 
      {
        ledfreq[0 + headIndex*4] = 0;
        ledfreq[1 + headIndex*4] = 0;
        ledfreq[2 + headIndex*4] = 0;
        ledfreq[3 + headIndex*4] = 0;
      }
    }
    else if(headIndex == 2)
    {
      if(State == 0) 
      {
        ledfreq[8] = 0; // TODO: add new CV
      }
      if(State == 1) 
      {
        ledfreq[8] = Dcc.getCV(64);
      }
    }
  }
  */
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

  timer++;
  
  if(timer > 100) {
    timer = 0;
  }
  
  if(ledfreq[0] >= timer) { digitalWrite(ledA, HIGH); }
  else { digitalWrite(ledA, LOW); }
  
  if(ledfreq[1] >= timer) { digitalWrite(ledB, HIGH); }
  else { digitalWrite(ledB, LOW); }
  
  if(ledfreq[2] >= timer) { digitalWrite(ledC, HIGH); }
  else { digitalWrite(ledC, LOW); }
  
  if(ledfreq[3] >= timer) { digitalWrite(ledD, HIGH); }
  else { digitalWrite(ledD, LOW); }
  
  if(ledfreq[4] >= timer) { digitalWrite(ledE, HIGH); }
  else { digitalWrite(ledE, LOW); }
  
  if(ledfreq[5] >= timer) { digitalWrite(ledF, HIGH); }
  else { digitalWrite(ledF, LOW); }
  
  if(ledfreq[6] >= timer) { digitalWrite(ledG, HIGH); }
  else { digitalWrite(ledG, LOW); }
  
  if(ledfreq[7] >= timer) { digitalWrite(ledH, HIGH); }
  else { digitalWrite(ledH, LOW); }
  
  if(ledfreq[8] >= timer) { digitalWrite(ledI, HIGH); }
  else { digitalWrite(ledI, LOW); }
}
