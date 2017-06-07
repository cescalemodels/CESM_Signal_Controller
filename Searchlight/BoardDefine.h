/**********************************************************************

BoardDefine.h
COPYRIGHT (C) 2017 David J. Cutting

Part of CESM_SEARCHLIGHT_CONTROLLER

**********************************************************************/

//////////////////////////////////
//  INCLUDE REQUIRED LIBRARIES  //
//////////////////////////////////

#include <SoftPWM.h>                                                  //  Include SoftPWM library from here: https://github.com/Palatis/arduino-softpwm/ 

///////////////////////////////////////////
//  DEFINE PINS BASED ON SELECTED BOARD  //
///////////////////////////////////////////

#if defined(__AVR_ATtiny841__)                                        //  PIN DEFINITIONS FOR ATTINY841 BOARDS

#define DCC_READ_PIN      PIN_B1                                      //  Pin number for the pin that reads the DCC signal via a 100K resistor. This
                                                                      //  pin must be on interrupt zero to ensure correct recieving of DCC packets.
                                                                      
#define PROG_JUMPER_PIN   PIN_B2                                      //  Pin number that detects if decoder is in programming mode. Short to 
                                                                      //  ground to start programming mode (on-board pull-ups are enabled)

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

#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)       //  PIN DEFINITIONS FOR ATMEGA328P/168 BOARDS

#define DCC_READ_PIN      2                                           //  Pin number for the pin that reads the DCC signal via a 100K resistor, 
                                                                      //  or optoisolator. This pin must be on interrupt zero to ensure correct 
                                                                      //  recieving of DCC packets.
                                                                      
#define PROG_JUMPER_PIN   13                                          //  Pin number that detects if decoder is in programming mode. Short to 
                                                                      //  ground to start programming mode (on-board pull-ups are enabled)

#ifndef SERIAL_DEBUG                                                         
SOFTPWM_DEFINE_CHANNEL(0, DDRD, PORTD, PORTD3);                       //  Creates soft PWM channel 0 on Port D3 (Pin 3)
SOFTPWM_DEFINE_CHANNEL(1, DDRD, PORTD, PORTD4);                       //  Creates soft PWM channel 1 on Port D4 (Pin 4)
SOFTPWM_DEFINE_CHANNEL(2, DDRD, PORTD, PORTD5);                       //  Creates soft PWM channel 2 on Port D5 (Pin 5)
SOFTPWM_DEFINE_CHANNEL(3, DDRD, PORTD, PORTD6);                       //  Creates soft PWM channel 3 on Port D6 (Pin 6)
SOFTPWM_DEFINE_CHANNEL(4, DDRD, PORTD, PORTD7);                       //  Creates soft PWM channel 4 on Port D7 (Pin 7)
SOFTPWM_DEFINE_CHANNEL(5, DDRB, PORTB, PORTB0);                       //  Creates soft PWM channel 5 on Port B0 (Pin 8)
SOFTPWM_DEFINE_CHANNEL(6, DDRB, PORTB, PORTB1);                       //  Creates soft PWM channel 6 on Port B1 (Pin 9)
SOFTPWM_DEFINE_CHANNEL(7, DDRB, PORTB, PORTB2);                       //  Creates soft PWM channel 7 on Port B2 (Pin 10)
SOFTPWM_DEFINE_CHANNEL(8, DDRB, PORTB, PORTB3);                       //  Creates soft PWM channel 8 on Port B3 (Pin 11)
SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS(9, 64);                         //  Defines the 9 soft PWM channels with 64 step resolution
#endif

#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)      //  PIN DEFINITIONS FOR ATMEGA2560/1280 BOARDS

#define DCC_READ_PIN      2                                           //  Pin number for the pin that reads the DCC signal via a 100K resistor, 
                                                                      //  or optoisolator. This pin must be on interrupt zero to ensure correct 
                                                                      //  recieving of DCC packets.
                                                                      
#define PROG_JUMPER_PIN   13                                          //  Pin number that detects if decoder is in programming mode. Short to 
                                                                      //  ground to start programming mode (on-board pull-ups are enabled)

#ifndef SERIAL_DEBUG                                                         
SOFTPWM_DEFINE_CHANNEL(0, DDRE, PORTE, PORTE5);                       //  Creates soft PWM channel 0 on Port E5 (Pin 3)
SOFTPWM_DEFINE_CHANNEL(1, DDRG, PORTG, PORTG5);                       //  Creates soft PWM channel 1 on Port G5 (Pin 4)
SOFTPWM_DEFINE_CHANNEL(2, DDRE, PORTE, PORTE3);                       //  Creates soft PWM channel 2 on Port E3 (Pin 5)
SOFTPWM_DEFINE_CHANNEL(3, DDRH, PORTH, PORTH3);                       //  Creates soft PWM channel 3 on Port H3 (Pin 6)
SOFTPWM_DEFINE_CHANNEL(4, DDRH, PORTH, PORTH4);                       //  Creates soft PWM channel 4 on Port H4 (Pin 7)
SOFTPWM_DEFINE_CHANNEL(5, DDRH, PORTH, PORTH5);                       //  Creates soft PWM channel 5 on Port H5 (Pin 8)
SOFTPWM_DEFINE_CHANNEL(6, DDRH, PORTH, PORTH6);                       //  Creates soft PWM channel 6 on Port H6 (Pin 9)
SOFTPWM_DEFINE_CHANNEL(7, DDRB, PORTB, PORTB4);                       //  Creates soft PWM channel 7 on Port B4 (Pin 10)
SOFTPWM_DEFINE_CHANNEL(8, DDRB, PORTB, PORTB5);                       //  Creates soft PWM channel 8 on Port B5 (Pin 11)
SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS(9, 64);                         //  Defines the 9 soft PWM channels with 64 step resolution
#endif

#elif defined(__AVR_ATmega32U4__)                                     //  PIN DEFINITIONS FOR ATMEGA32U4 BOARDS

#define DCC_READ_PIN      3                                           //  Pin number for the pin that reads the DCC signal via a 100K resistor, 
                                                                      //  or optoisolator. This pin must be on interrupt zero to ensure correct 
                                                                      //  recieving of DCC packets.
                                                                      
#define PROG_JUMPER_PIN   13                                          //  Pin number that detects if decoder is in programming mode. Short to 
                                                                      //  ground to start programming mode (on-board pull-ups are enabled)
                                                                      
#ifndef SERIAL_DEBUG                                                         
SOFTPWM_DEFINE_CHANNEL(0, DDRD, PORTD, PORTD4);                       //  Creates soft PWM channel 0 on Port E5 (Pin 3)
SOFTPWM_DEFINE_CHANNEL(1, DDRC, PORTC, PORTC6);                       //  Creates soft PWM channel 1 on Port G5 (Pin 4)
SOFTPWM_DEFINE_CHANNEL(2, DDRD, PORTD, PORTD7);                       //  Creates soft PWM channel 2 on Port E3 (Pin 5)
SOFTPWM_DEFINE_CHANNEL(3, DDRE, PORTE, PORTE6);                       //  Creates soft PWM channel 3 on Port H3 (Pin 6)
SOFTPWM_DEFINE_CHANNEL(4, DDRB, PORTB, PORTB4);                       //  Creates soft PWM channel 4 on Port H4 (Pin 7)
SOFTPWM_DEFINE_CHANNEL(5, DDRB, PORTB, PORTB5);                       //  Creates soft PWM channel 5 on Port H5 (Pin 8)
SOFTPWM_DEFINE_CHANNEL(6, DDRB, PORTB, PORTB6);                       //  Creates soft PWM channel 6 on Port H6 (Pin 9)
SOFTPWM_DEFINE_CHANNEL(7, DDRB, PORTB, PORTB7);                       //  Creates soft PWM channel 7 on Port B4 (Pin 10)
SOFTPWM_DEFINE_CHANNEL(8, DDRD, PORTD, PORTD6);                       //  Creates soft PWM channel 8 on Port B5 (Pin 11)
SOFTPWM_DEFINE_OBJECT_WITH_PWM_LEVELS(9, 64);                         //  Defines the 9 soft PWM channels with 64 step resolution
#endif

#else                                                                 //  COMPILER ERROR FOR NON-SUPPORTED BOARDS

#error CANNOT COMPILE - PLEASE USE A SUPPORTED CHIP ( ATMEGA328P, ATMEGA168, ATMEGA2560, ATMEGA32U4, ATMEGA1280 OR ATTINY841 )

#endif
