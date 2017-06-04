/**********************************************************************

Config.h
COPYRIGHT (C) 2017 David J. Cutting, Alex Shepard

***********************************************************************/

//////////////////////////////////////////////////////////////////////////////////////
//
//  Choose the manufacturer ID for the decoder (default: 10 (NMRA DIY ID number))

#define MFG_ID  10

//////////////////////////////////////////////////////////////////////////////////////
//
//  Choose ONE:
//
//  Enable serial debug to print messages to the serial line, and disable all LEDs
//  Enable light debugging which will disable all special effects.

//#define SERIAL_DEBUG
//#define LIGHT_DEBUG

//////////////////////////////////////////////////////////////////////////////////////
//
//  Enable printing of all DCC messages by uncommenting this line and the one above 

//#define NOTIFY_DCC_MSG

//////////////////////////////////////////////////////////////////////////////////////
//
//  Set default address for the decoder that it assumes upon reset.

#define DEFAULT_ADDRESS 40

//////////////////////////////////////////////////////////////////////////////////////
