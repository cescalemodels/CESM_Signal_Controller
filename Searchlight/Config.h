/**********************************************************************

Config.h
COPYRIGHT (C) 2017 David J. Cutting

Part of CESM_SEARCHLIGHT_CONTROLLER

**********************************************************************/

#include <NmraDcc.h>                                                  //  You must use the branch available here: https://github.com/mrrwa/NmraDcc/tree/AddOutputModeAddressing

//////////////////////////////////////////////////////////////////////////////////////
//
//  Set default address for the decoder that it assumes upon reset.

#define DEFAULT_ADDRESS 40

//////////////////////////////////////////////////////////////////////////////////////
//
//  Debug - Choose ONE:
//
//  Enable serial debug to print messages to the serial line, and disable all LEDs
//  Enable light debugging which will disable all special effects.
//  Enable serial debug, print all recieved DCC packets to serial line (no other debug messages)

//#define SERIAL_DEBUG
//#define NOTIFY_DCC_MESSAGE

#define DEBUG_BAUD_RATE 115200

//////////////////////////////////////////////////////////////////////////////////////
//
//  Force Restore to Factory Defaults Every time the Decoder is Restarted

//#define RESET_CVS_ON_POWER

//////////////////////////////////////////////////////////////////////////////////////
//
//  Define Manufacturer ID (default is 10

#define MAN_ID  MAN_ID_DIY

//////////////////////////////////////////////////////////////////////////////////////
