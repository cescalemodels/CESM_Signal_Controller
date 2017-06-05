/*

 Burgett_Walkerford_Micro_08e

 Arduino Micro for C&O Walkerford interlocking plant using BLMA Signal Heads.
 
 This program utilizes the PWM features of the microcontroller to simulate the look of searchlight signals.
 
 Using A0-A3 for inputs from the C/MRI Remote Searchlight Signal Driver (RSSD)
 These lines supply 5V to the signal head LEDs in a common cathode configuration.
 Since the original signals used three-wire bi-color LEDs in the searchlights, 
 only two lines drove the red & green elements.  Yellow was derived by driving 
 both the red & green elements and mixing the colors in the LED.  Part of this code 
 transforms the two-line common-cathode logic into the request for one of three colors.
 
 PWM outputs on pins 3, 5, 6, 9, 10, & 11 are hooked up to the BLMA signal heads.
 Each color (red, yellow & green) uses one pin.  BLMA signals are common-anode.
 
 While the Walkerford plant never has more than one signal head changing at a time,
 I wanted the code to be able to handle that so it could be repurposed for other applications.
 Therfore the loops that perform the animation, have breaks to allow the code to continue
 other work, then return to the animation.
 
 
 *****
 THIS PROGRAM CONTAINS SEPARATE FUNCTIONS FOR ALL POSSIBLE TRANSITIONS
 *****


 Copyright 2013 Mike Weber
 Last modified 23-June-2013
 */

// DEFINE C/MRI INPUT PINS
const byte A1_input = A0;        // Input from C/MRI RSSD Module
const byte A2_input = A1;
const byte B1_input = A2;
const byte B2_input = A3;

// DEFINE SIGNAL HEAD LED CONNECTIONS
const byte A_red = 3;            // "A" head output connections
const byte A_yellow = 5;
const byte A_green = 6;
const byte B_red = 9;            // "B" head output connections
const byte B_yellow = 10;
const byte B_green = 11;
const byte Heartbeat = 13;       // Blinky light to let us know the microconroller is still running

// DIGITAL INPUT VARIABLES
int read_A1;                     // Raw input pin read data prior to debounce calculations
int read_A2;
int read_B1;
int read_B2;
int A1_currentState;             // Current Button state from the input pin
int A1_previousState = LOW;      // Previous Button state from the input pin
int A2_currentState;
int A2_previousState = LOW;
int B1_currentState;
int B1_previousState = LOW;
int B2_currentState;
int B2_previousState = LOW;

// SIGNAL ASPECT VARIABLES
char A_request = 'd';
char A_actual = 'd';
char B_request = 'd';
char B_actual = 'd';

// ASPECT CHANGE ANIMATION VARIABLES
boolean A_animate = false;
boolean B_animate = false;
unsigned long A_lastAnimateTime = 0;
unsigned long B_lastAnimateTime = 0;
const unsigned long animateDelayTime = 20;   // Global "framerate" for signal head animations.  Default = 20ms.

// DEBOUNCE TIMER VARIABLES
long A1_lastDebounceTime = 0;                // Last time the input was determined to be changed by the debounce timer
long A2_lastDebounceTime = 0;
long B1_lastDebounceTime = 0;
long B2_lastDebounceTime = 0;
const long debounceDelay = 300;              // Global C/MRI electrical input debounce time

// HEARTBEAT VARIABLES
int heartbeatState = LOW;
long previousHeartbeat = 0;
unsigned long currentHeartbeat = 0;
const long heartbeatDelay = 1175;            // Delay for Heartbeat LED flashing

// ASPECT CHANGE STABILIZE VARIABLES
int A_inputStabilizeCount = 0;               // Counter for making sure both inputs per head have stabilized before calculating new "requested" values
int B_inputStabilizeCount = 0;

// DEFINE TRANSITION ARRAYS
const byte EtoE[ ] = {                       // Array for "end-to-end" animation - (Y to G) or (G to Y)
  10, 25, 55, 100, 145, 215, 120, 65, 100, 190, 210, 255, 230, 165, 65, 5, 55, 195, 235, 255, 230, 200, 175,
  145, 110, 55, 12, 0, 0, 50, 150, 100, 55, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 65, 100, 75, 25, 0 };

const byte CtoE[ ] = {                       // Array for "center-to-end" animation - away FROM Red to any color
  10, 55, 145, 220, 255, 225, 200, 155, 75, 0, 0, 24, 74, 120, 100, 60, 30, 0, 48, 80, 41, 27, 0, 28, 48, 33, 17, 0 };

const byte EtoC[ ] = {                       // Array for "end-to-center animation - any color TO Red
  5, 12, 28, 43, 71, 112, 125, 197, 255, 250, 240, 208, 125, 105, 75, 35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 25, 45, 85, 140, 180, 215, 230, 215, 200, 170, 140, 110, 75, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 25, 40, 65, 90, 125, 140, 115, 85, 55, 31, 0 };


/********************************************
 *                                           *
 *                S E T U P                  *
 *                                           *
 ********************************************/

void setup() {
  pinMode(A1_input, INPUT);               // Setup input & output pins
  pinMode(A2_input, INPUT);
  pinMode(B1_input, INPUT);
  pinMode(B2_input, INPUT);
  pinMode(A_red, OUTPUT);
  pinMode(A_yellow, OUTPUT);
  pinMode(A_green, OUTPUT);
  pinMode(B_red, OUTPUT);
  pinMode(B_yellow, OUTPUT);
  pinMode(B_green, OUTPUT);

  digitalWrite(A_red, LOW);               //  CYCLE SIGNALS THROUGH ALL COLORS WITH NO ANIMATION
  digitalWrite(A_yellow, HIGH);           //  TO VERIFY SIGNAL WIRING AT BEGINNING OF SESSION
  digitalWrite(A_green, HIGH);
  digitalWrite(B_red, LOW);
  digitalWrite(B_yellow, HIGH);
  digitalWrite(B_green, HIGH);
  delay(500);
  digitalWrite(A_red, HIGH);
  digitalWrite(A_yellow, LOW);
  digitalWrite(A_green, HIGH);
  digitalWrite(B_red, HIGH);
  digitalWrite(B_yellow, LOW);
  digitalWrite(B_green, HIGH);
  delay(500);
  digitalWrite(A_red, HIGH);
  digitalWrite(A_yellow, HIGH);
  digitalWrite(A_green, LOW);
  digitalWrite(B_red, HIGH);
  digitalWrite(B_yellow, HIGH);
  digitalWrite(B_green, LOW);
  delay(500);
  digitalWrite(A_red, HIGH);
  digitalWrite(A_yellow, HIGH);
  digitalWrite(A_green, HIGH);
  digitalWrite(B_red, HIGH);
  digitalWrite(B_yellow, HIGH);
  digitalWrite(B_green, HIGH);
}


/********************************************
 *                                           *
 *                 L O O P                   *
 *                                           *
 ********************************************/

void loop() {

  //  **************************  H E A R T B E A T  ******************************************

  currentHeartbeat = millis();
  if(currentHeartbeat - previousHeartbeat > heartbeatDelay) {
    previousHeartbeat = currentHeartbeat;
    if (heartbeatState == LOW)
      heartbeatState = HIGH;
    else
      heartbeatState = LOW;
    digitalWrite(Heartbeat, heartbeatState);
  }


  //  ******************  R E A D   I N P U T S   &   D E B O U N C E  ************************

  read_A1 = digitalRead(A1_input);                                // Read the C/MRI inputs into variables
  read_A2 = digitalRead(A2_input);
  read_B1 = digitalRead(B1_input);
  read_B2 = digitalRead(B2_input); 

  if (read_A1 != A1_previousState) {                              // If the input has changed...
    A1_lastDebounceTime = millis();                               // reset the debounce timer...
    A_inputStabilizeCount = 0;                                    // and reset the Stabilize loop counter.
  } 
  if (read_A2 != A2_previousState) {
    A2_lastDebounceTime = millis();
    A_inputStabilizeCount = 0;
  }
  if (read_B1 != B1_previousState) {
    B1_lastDebounceTime = millis();
    B_inputStabilizeCount = 0;
  } 
  if (read_B2 != B2_previousState) {
    B2_lastDebounceTime = millis();
    B_inputStabilizeCount = 0;
  }

  if ((millis() - A1_lastDebounceTime) > debounceDelay) {         // If the input has been that way longer than the debounce delay...
    A1_currentState = read_A1;                                    // then set the input state as the current state
  }
  if ((millis() - A2_lastDebounceTime) > debounceDelay) {
    A2_currentState = read_A2;
  }
  if ((millis() - B1_lastDebounceTime) > debounceDelay) {
    B1_currentState = read_B1;
  }
  if ((millis() - B2_lastDebounceTime) > debounceDelay) {
    B2_currentState = read_B2;
  }


  //  ***************  C A L C U L A T E   R E Q E S T E D   A S P E C T S  ******************

  if (A_inputStabilizeCount < 60) {                               // *****************  A   H E A D  ****************************
    A_inputStabilizeCount ++;                                     // Make sure both inputs are stable before calculating requests
  }
  else {
    if (A1_previousState == A1_currentState && A2_previousState == A2_currentState) {
      if (A_animate == false) {                                           // If we determine that we are not currently animating the aspect...
        if (A1_currentState == HIGH && A2_currentState == LOW) {          // then we calculate request variables for A head
          if (A_request != 'R'){
            A_request = 'R';                                              // Register the new requested color for this head
            A_animate = true;
          }
        }
        else if (A1_currentState == HIGH && A2_currentState == HIGH) {
          if (A_request != 'Y'){ 
            A_request = 'Y';
            A_animate = true;
          }  
        }
        else if (A1_currentState == LOW && A2_currentState == HIGH) {
          if (A_request != 'G'){
            A_request = 'G';
            A_animate = true;
          }  
        }
        else if (A1_currentState == LOW && A2_currentState == LOW) {
          if (A_request != 'd') {
            A_request = 'd';
            A_animate = true;
          }
        }
      }
    }
  }

  if (B_inputStabilizeCount < 60) {                               // *****************  B   H E A D  ****************************
    B_inputStabilizeCount ++;                                     // Make sure both inputs are stable before calculating requests
  }
  else {
    if (B1_previousState == B1_currentState && B2_previousState == B2_currentState) {
      if (B_animate == false) {                                           // If we determine that we are not currently animating the aspect...
        if (B1_currentState == HIGH && B2_currentState == LOW) {          // then we calculate request variables for B head
          if (B_request != 'R'){
            B_request = 'R';
            B_animate = true;
          }
        }
        else if (B1_currentState == HIGH && B2_currentState == HIGH) {
          if (B_request != 'Y'){ 
            B_request = 'Y';
            B_animate = true;
          }  
        }
        else if (B1_currentState == LOW && B2_currentState == HIGH) {
          if (B_request != 'G'){
            B_request = 'G';
            B_animate = true;
          }  
        }
        else if (B1_currentState == LOW && B2_currentState == LOW) {
          if (B_request != 'd') {
            B_request = 'd';
            B_animate = true;
          }
        }
      }
    }
  }

  A1_previousState = read_A1;                                     // No matter what just happened, save the read state...
  A2_previousState = read_A2;                                     // as the previous state for the next time around the loop
  B1_previousState = read_B1;
  B2_previousState = read_B2;


  //  ***************  C A L L   P R O P E R   A N I M A T I O N   F U N C T I O N  ******************
  //  ***************  A   H E A D  ***************

  //  Look at the Actual State of the signal head and make a case determination to route to the proper function

  if (A_animate == true) {                                        // Check if head is actively animating.
    unsigned long currentTime = millis();                         // Set current time to a variable.
    if (currentTime - A_lastAnimateTime > animateDelayTime) {     // And check if it's time to fire off the next frame of animation (every 20ms)
      switch (A_actual) {
      case 'R':                                                   // If the head is actually red...
        switch (A_request) {
        case 'Y':                                                 // See what is requested and select the proper animation function.
          A_RedYellow();                                          // ...and so on
          break;
        case 'G':
          A_RedGreen();
          break;
        case 'd':
          A_RedDark();
          break;
        }
      }

      switch (A_actual) {
      case 'Y':
        switch (A_request) {
        case 'R':
          A_YellowRed();
          break;
        case 'G':
          A_YellowGreen();
          break;
        case 'd':
          A_YellowDark();
          break;
        }
      }

      switch (A_actual) {
      case 'G':
        switch (A_request) {
        case 'R':
          A_GreenRed();
          break;
        case 'Y':
          A_GreenYellow();
          break;
        case 'd':
          A_GreenDark();
          break;
        }
      }

      switch (A_actual) {
      case 'd':
        switch (A_request) {
        case 'R':
          A_DarkRed();
          break;
        case 'Y':
          A_DarkYellow();
          break;
        case 'G':
          A_DarkGreen();
          break;
        }
      }
    }
  }

  //  ***************  B   H E A D  ***************

  if (B_animate == true) {                                        // Check if head is actively animating.
    unsigned long currentTime = millis();                         // Set current time to a variable.
    if (currentTime - B_lastAnimateTime > animateDelayTime) {     // And check if it's time to fire off the next frame of animation (every 20ms)
      switch (B_actual) {
      case 'R':                                                   // If the head is actually red...
        switch (B_request) {
        case 'Y':                                                 // See what is requested and select the proper animation function.
          B_RedYellow();                                          // ...and so on
          break;
        case 'G':
          B_RedGreen();
          break;
        case 'd':
          B_RedDark();
          break;
        }
      }

      switch (B_actual) {
      case 'Y':
        switch (B_request) {
        case 'R':
          B_YellowRed();
          break;
        case 'G':
          B_YellowGreen();
          break;
        case 'd':
          B_YellowDark();
          break;
        }
      }

      switch (B_actual) {
      case 'G':
        switch (B_request) {
        case 'R':
          B_GreenRed();
          break;
        case 'Y':
          B_GreenYellow();
          break;
        case 'd':
          B_GreenDark();
          break;
        }
      }

      switch (B_actual) {
      case 'd':
        switch (B_request) {
        case 'R':
          B_DarkRed();
          break;
        case 'Y':
          B_DarkYellow();
          break;
        case 'G':
          B_DarkGreen();
          break;
        }
      }
    }
  }

  delay(3);     //  Generally slow things down a bit.

}

/********************************************
 *                                           *
 *      E N D   O F   M A I N   L O O P      *
 *                                           *
 ********************************************/





/****************************************************************************************************************************
 *                                                                                                                           *
 *           A L L   P O S S I B L E   A N I M A T I O N   T R A N S I T I O N S   A S   F U N C T I O N S                   *
 *                                                                                                                           *
 ****************************************************************************************************************************/


  //  ***************  A   H E A D  ***************

/**********************************************************************
FUNCTION:  A_RedYellow
PURPOSE:   Transition the A head from Red to Yellow
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_RedYellow() {
  static int frame = 0;
  for( ; frame < 28; ) {
    if (frame < 5) {
      analogWrite(A_red, CtoE[frame]);
      frame++;
      A_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 28) {
        analogWrite(A_yellow, CtoE[frame]);
        frame++;
        A_lastAnimateTime = millis();
        return;
      }
    }
  }
  A_animate = false;
  A_actual = 'Y';
  frame = 0;
}

/**********************************************************************
FUNCTION:  A_RedGreen
PURPOSE:   Transition the A head from Red to Green
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_RedGreen() {
  static int frame = 0;
  for( ; frame < 28; ) {
    if (frame < 5) {
      analogWrite(A_red, CtoE[frame]);
      frame++;
      A_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 28) {
        analogWrite(A_green, CtoE[frame]);
        frame++;
        A_lastAnimateTime = millis();
        return;
      }
    }
  }
  A_animate = false;
  A_actual = 'G';
  frame = 0;
}

/**********************************************************************
FUNCTION:  A_RedDark
PURPOSE:   Transition the A head from Red to Dark
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_RedDark() {
  static int value = 0;
  for( ; value < 254 ; ) {
    if (255 - value < 6) {
      value = 255;
    }
    else {
      value = value + 3;
    }
    analogWrite(A_red, value);
    return;
  }
  A_animate = false;
  A_actual = 'd';
  value = 0;
}

/**********************************************************************
FUNCTION:  A_YellowRed
PURPOSE:   Transition the A head from Yellow to Red
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_YellowRed() {
  static int frame = 0;
  for( ; frame < 61; ) {
    if (frame < 10) {
      analogWrite(A_yellow, EtoC[frame]);
      frame++;
      A_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 61) {
        analogWrite(A_red, EtoC[frame]);
        frame++;
        A_lastAnimateTime = millis();
        return;
      }
    }
  }
  A_animate = false;
  A_actual = 'R';
  frame = 0;
}

/**********************************************************************
FUNCTION:  A_YellowGreen
PURPOSE:   Transition the A head from Yellow to Green
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_YellowGreen() {
  static int frame = 0;
  for( ; frame < 54; ) {
    if (frame < 12) {
      analogWrite(A_yellow, EtoE[frame]);
      frame++;
      A_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 20) {
        analogWrite(A_red, EtoE[frame]);
        frame++;
        A_lastAnimateTime = millis();
        return;
      }
      else {
        analogWrite(A_green, EtoE[frame]);
        frame++;
        A_lastAnimateTime = millis();
        return;
      }
    }
  }
  A_animate = false;
  A_actual = 'G';
  frame = 0;
}

/**********************************************************************
FUNCTION:  A_YellowDark
PURPOSE:   Transition the A head from Yellow to Dark
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_YellowDark() {
  static int value = 0;
  for( ; value < 254 ; ) {
    if (255 - value < 6) {
      value = 255;
    }
    else {
      value = value + 3;
    }
    analogWrite(A_yellow, value);
    return;
  }
  A_animate = false;
  A_actual = 'd';
  value = 0;
}

/**********************************************************************
FUNCTION:  A_GreenRed
PURPOSE:   Transition the A head from Green to Red
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_GreenRed() {
  static int frame = 0;
  for( ; frame < 61; ) {
    if (frame < 10) {
      analogWrite(A_green, EtoC[frame]);
      frame++;
      A_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 61) {
        analogWrite(A_red, EtoC[frame]);
        frame++;
        A_lastAnimateTime = millis();
        return;
      }
    }
  }
  A_animate = false;
  A_actual = 'R';
  frame = 0;
}

/**********************************************************************
FUNCTION:  A_GreenYellow
PURPOSE:   Transition the A head from Green to Yellow
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_GreenYellow() {
  static int frame = 0;
  for( ; frame < 54; ) {
    if (frame < 12) {
      analogWrite(A_green, EtoE[frame]);
      frame++;
      A_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 20) {
        analogWrite(A_red, EtoE[frame]);
        frame++;
        A_lastAnimateTime = millis();
        return;
      }
      else {
        analogWrite(A_yellow, EtoE[frame]);
        frame++;
        A_lastAnimateTime = millis();
        return;
      }
    }
  }
  A_animate = false;
  A_actual = 'Y';
  frame = 0;
}

/**********************************************************************
FUNCTION:  A_GreenDark
PURPOSE:   Transition the A head from Green to Dark
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_GreenDark() {
  static int value = 0;
  for( ; value < 254 ; ) {
    if (255 - value < 6) {
      value = 255;
    }
    else {
      value = value + 3;
    }
    analogWrite(A_green, value);
    return;
  }
  A_animate = false;
  A_actual = 'd';
  value = 0;
}

/**********************************************************************
FUNCTION:  A_DarkRed
PURPOSE:   Transition the A head from Dark to Red
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_DarkRed() {
  static int value = 255;
  for( ; value > 0 ; ) {
    if (255 - value > 249) {
      value = 0;
    }
    else {
      value = value - 4;
    }
    analogWrite(A_red, value);
    return;
  }
  A_animate = false;
  A_actual = 'R';
  value = 255;
}

/**********************************************************************
FUNCTION:  A_DarkYellow
PURPOSE:   Transition the A head from Dark to Yellow
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_DarkYellow() {
  static int value = 255;
  for( ; value > 0 ; ) {
    if (255 - value > 249) {
      value = 0;
    }
    else {
      value = value - 4;
    }
    analogWrite(A_yellow, value);
    return;
  }
  A_animate = false;
  A_actual = 'Y';
  value = 255;
}

/**********************************************************************
FUNCTION:  A_DarkGreen
PURPOSE:   Transition the A head from Dark to Green
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void A_DarkGreen() {
  static int value = 255;
  for( ; value > 0 ; ) {
    if (255 - value > 249) {
      value = 0;
    }
    else {
      value = value - 4;
    }
    analogWrite(A_green, value);
    return;
  }
  A_animate = false;
  A_actual = 'G';
  value = 255;
}


  //  ***************  B   H E A D  ***************

/**********************************************************************
FUNCTION:  B_RedYellow
PURPOSE:   Transition the B head from Red to Yellow
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_RedYellow() {
  static int frame = 0;
  for( ; frame < 28; ) {
    if (frame < 5) {
      analogWrite(B_red, CtoE[frame]);
      frame++;
      B_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 28) {
        analogWrite(B_yellow, CtoE[frame]);
        frame++;
        B_lastAnimateTime = millis();
        return;
      }
    }
  }
  B_animate = false;
  B_actual = 'Y';
  frame = 0;
}

/**********************************************************************
FUNCTION:  B_RedGreen
PURPOSE:   Transition the B head from Red to Green
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_RedGreen() {
  static int frame = 0;
  for( ; frame < 28; ) {
    if (frame < 5) {
      analogWrite(B_red, CtoE[frame]);
      frame++;
      B_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 28) {
        analogWrite(B_green, CtoE[frame]);
        frame++;
        B_lastAnimateTime = millis();
        return;
      }
    }
  }
  B_animate = false;
  B_actual = 'G';
  frame = 0;
}

/**********************************************************************
FUNCTION:  B_RedDark
PURPOSE:   Transition the B head from Red to Dark
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_RedDark() {
  static int value = 0;
  for( ; value < 254 ; ) {
    if (255 - value < 6) {
      value = 255;
    }
    else {
      value = value + 3;
    }
    analogWrite(B_red, value);
    return;
  }
  B_animate = false;
  B_actual = 'd';
  value = 0;
}

/**********************************************************************
FUNCTION:  B_YellowRed
PURPOSE:   Transition the B head from Yellow to Red
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_YellowRed() {
  static int frame = 0;
  for( ; frame < 61; ) {
    if (frame < 10) {
      analogWrite(B_yellow, EtoC[frame]);
      frame++;
      B_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 61) {
        analogWrite(B_red, EtoC[frame]);
        frame++;
        B_lastAnimateTime = millis();
        return;
      }
    }
  }
  B_animate = false;
  B_actual = 'R';
  frame = 0;
}

/**********************************************************************
FUNCTION:  B_YellowGreen
PURPOSE:   Transition the B head from Yellow to Green
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_YellowGreen() {
  static int frame = 0;
  for( ; frame < 54; ) {
    if (frame < 12) {
      analogWrite(B_yellow, EtoE[frame]);
      frame++;
      B_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 20) {
        analogWrite(B_red, EtoE[frame]);
        frame++;
        B_lastAnimateTime = millis();
        return;
      }
      else {
        analogWrite(B_green, EtoE[frame]);
        frame++;
        B_lastAnimateTime = millis();
        return;
      }
    }
  }
  B_animate = false;
  B_actual = 'G';
  frame = 0;
}

/**********************************************************************
FUNCTION:  B_YellowDark
PURPOSE:   Transition the B head from Yellow to Dark
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_YellowDark() {
  static int value = 0;
  for( ; value < 254 ; ) {
    if (255 - value < 6) {
      value = 255;
    }
    else {
      value = value + 3;
    }
    analogWrite(B_yellow, value);
    return;
  }
  B_animate = false;
  B_actual = 'd';
  value = 0;
}

/**********************************************************************
FUNCTION:  B_GreenRed
PURPOSE:   Transition the B head from Green to Red
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_GreenRed() {
  static int frame = 0;
  for( ; frame < 61; ) {
    if (frame < 10) {
      analogWrite(B_green, EtoC[frame]);
      frame++;
      B_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 61) {
        analogWrite(B_red, EtoC[frame]);
        frame++;
        B_lastAnimateTime = millis();
        return;
      }
    }
  }
  B_animate = false;
  B_actual = 'R';
  frame = 0;
}

/**********************************************************************
FUNCTION:  B_GreenYellow
PURPOSE:   Transition the B head from Green to Yellow
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_GreenYellow() {
  static int frame = 0;
  for( ; frame < 54; ) {
    if (frame < 12) {
      analogWrite(B_green, EtoE[frame]);
      frame++;
      B_lastAnimateTime = millis();
      return;
    }
    else {
      if (frame < 20) {
        analogWrite(B_red, EtoE[frame]);
        frame++;
        B_lastAnimateTime = millis();
        return;
      }
      else {
        analogWrite(B_yellow, EtoE[frame]);
        frame++;
        B_lastAnimateTime = millis();
        return;
      }
    }
  }
  B_animate = false;
  B_actual = 'Y';
  frame = 0;
}

/**********************************************************************
FUNCTION:  B_GreenDark
PURPOSE:   Transition the B head from Green to Dark
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_GreenDark() {
  static int value = 0;
  for( ; value < 254 ; ) {
    if (255 - value < 6) {
      value = 255;
    }
    else {
      value = value + 3;
    }
    analogWrite(B_green, value);
    return;
  }
  B_animate = false;
  B_actual = 'd';
  value = 0;
}

/**********************************************************************
FUNCTION:  B_DarkRed
PURPOSE:   Transition the B head from Dark to Red
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_DarkRed() {
  static int value = 255;
  for( ; value > 0 ; ) {
    if (255 - value > 249) {
      value = 0;
    }
    else {
      value = value - 4;
    }
    analogWrite(B_red, value);
    return;
  }
  B_animate = false;
  B_actual = 'R';
  value = 255;
}

/**********************************************************************
FUNCTION:  B_DarkYellow
PURPOSE:   Transition the B head from Dark to Yellow
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_DarkYellow() {
  static int value = 255;
  for( ; value > 0 ; ) {
    if (255 - value > 249) {
      value = 0;
    }
    else {
      value = value - 4;
    }
    analogWrite(B_yellow, value);
    return;
  }
  B_animate = false;
  B_actual = 'Y';
  value = 255;
}

/**********************************************************************
FUNCTION:  B_DarkGreen
PURPOSE:   Transition the B head from Dark to Green
ARGUMENTS: None
RETURNS:   Nothing
NOTES:
**********************************************************************/
void B_DarkGreen() {
  static int value = 255;
  for( ; value > 0 ; ) {
    if (255 - value > 249) {
      value = 0;
    }
    else {
      value = value - 4;
    }
    analogWrite(B_green, value);
    return;
  }
  B_animate = false;
  B_actual = 'G';
  value = 255;
}

