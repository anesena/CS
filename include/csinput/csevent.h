/*
    Crystal Space 3D engine: Event class interface
    Copyright (C) 1998 by Jorrit Tyberghein
    Written by Andrew Zabolotny <bit@eltech.ru>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CSEVENT_H__
#define __CSEVENT_H__

#include "csutil/csbase.h"

/// Windowing System Events: take care to not define more than 32 event types
enum
{
  csevNothing = 0,
  csevKeyDown,			// A key has been pressed
  csevKeyUp,			// A key has been released
  csevMouseMove,		// Mouse has been moved
  csevMouseDown,		// A mouse button has been pressed
  csevMouseUp,			// A mouse button has been released
  csevMouseDoubleClick,		// A mouse button has been clicked twice
  csevCommand,			// Somebody(-thing) sent a command
  csevBroadcast			// Somebody(-thing) sent a broadcast command
};

// Event masks
#define CSMASK_Nothing		(1 << csevNothing)
#define CSMASK_KeyDown		(1 << csevKeyDown)
#define CSMASK_KeyUp		(1 << csevKeyUp)
#define CSMASK_MouseMove	(1 << csevMouseMove)
#define CSMASK_MouseDown	(1 << csevMouseDown)
#define CSMASK_MouseUp		(1 << csevMouseUp)
#define CSMASK_MouseDoubleClick	(1 << csevMouseDoubleClick)
#define CSMASK_Command		(1 << csevCommand)
#define CSMASK_Broadcast	(1 << csevBroadcast)

// Some handy macros
#define IS_KEYBOARD_EVENT(e)	((1 << (e).Type) & \
 (CSMASK_KeyDown | CSMASK_KeyUp))
#define IS_MOUSE_EVENT(e)	((1 << (e).Type) & \
 (CSMASK_MouseMove | CSMASK_MouseDown | \
  CSMASK_MouseUp | CSMASK_MouseDoubleClick))

/**
 * Modifier key masks
 */
/// "Shift" key mask
#define CSMASK_SHIFT		0x00000001
/// "Ctrl" key mask
#define CSMASK_CTRL		0x00000002
/// "Alt" key mask
#define CSMASK_ALT		0x00000004
/// All shift keys
#define CSMASK_ALLSHIFTS	(CSMASK_SHIFT | CSMASK_CTRL | CSMASK_ALT)
/// Key is pressed for first time or it is an autorepeat?
#define CSMASK_FIRST		0x80000000

/**
 * Control key codes
 */
/// ESCape key
#define CSKEY_ESC		27
/// Enter key
#define CSKEY_ENTER		'\n'
/// Tab key
#define CSKEY_TAB		'\t'
/// Back-space key
#define CSKEY_BACKSPACE	'\b'
/// Up arrow key
#define CSKEY_UP		1000
/// Down arrow key
#define CSKEY_DOWN		1001
/// Left arrow key
#define CSKEY_LEFT		1002
/// Right arrow key
#define CSKEY_RIGHT		1003
/// PageUp key
#define CSKEY_PGUP		1004
/// PageDown key
#define CSKEY_PGDN		1005
/// Home key
#define CSKEY_HOME		1006
/// End key
#define CSKEY_END		1007
/// Insert key
#define CSKEY_INS		1008
/// Delete key
#define CSKEY_DEL		1009
/// Control key
#define CSKEY_CTRL		1010
/// Alternative shift key
#define CSKEY_ALT		1011
/// Shift key
#define CSKEY_SHIFT		1012
/// Function key F1
#define CSKEY_F1		1013
/// Function key F2
#define CSKEY_F2		1014
/// Function key F3
#define CSKEY_F3		1015
/// Function key F4
#define CSKEY_F4		1016
/// Function key F5
#define CSKEY_F5		1017
/// Function key F6
#define CSKEY_F6		1018
/// Function key F7
#define CSKEY_F7		1019
/// Function key F8
#define CSKEY_F8		1020
/// Function key F9
#define CSKEY_F9		1021
/// Function key F10
#define CSKEY_F10		1022
/// Function key F11
#define CSKEY_F11		1023
/// Function key F12
#define CSKEY_F12		1024
/// The "center" key ("5" on numeric keypad)
#define CSKEY_CENTER		1025
/// Numeric keypad '+'
#define CSKEY_PADPLUS		1026
/// Numeric keypad '-'
#define CSKEY_PADMINUS		1027
/// Numeric keypad '*'
#define CSKEY_PADMULT		1028
/// Numeric keypad '/'
#define CSKEY_PADDIV		1029

/// First and last control key code
#define CSKEY_FIRST		1000
#define CSKEY_LAST		1029

/**
 * Predefined Command Codes<p>
 * The list below does not contain all defined messages; these are only the
 * most general ones. Crystal Space Windowing System has a broad range of
 * predefined commands; look in CSWS header files for more info.
 */
enum
{
  /**
   * No command
   */
  cscmdNothing = 0,
  /**
   * The event below causes application to quit immediately, no matter
   * which window posted the event.
   */
  cscmdQuit,
  /**
   * Application has changed its "focused" status.
   * This command is posted (or is not posted) by system-dependent driver.
   * <pre>
   * IN: false -> window lose focus, true -> window got focus
   * </pre>
   */
  cscmdFocusChanged,
  /**
   * This event is broadcasted to all plugins inside csSystemDriver::Open
   * right after all drivers were initialized and opened.
   */
  cscmdSystemOpen,
  /**
   * This event is broadcasted to all plugins inside csSystemDriver::Close
   * right before starting to close all drivers.
   */
  cscmdSystemClose
};

/**
 * This class represents a windowing system event.<p>
 * Events can be generated by hardware (keyboard, mouse)
 * as well as by software. There are so much constructors of
 * this class as much different types of events exists.
 */
class csEvent : public csBase
{
public:
  int Type;
  long Time;			// Time when the event occured
  union
  {
    struct
    {
      int Code;			// Key code
      int ShiftKeys;		// Control key state
    } Key;
    struct
    {
      int x,y;			// Mouse coords
      int Button;		// Button number: 1-left, 2-right, 3-middle
      int ShiftKeys;		// Control key state
    } Mouse;
    struct
    {
      int dx, dy;		// Joystick deltax, deltay
      int Button;		// Joystick button number
      int ShiftKeys;		// Control key state
    } Joystick;
    struct
    {
      unsigned int Code;	// Command code
      void *Info;		// Command info
    } Command;			// to allow virtual destructors
  };

  /// Empty initializer
  csEvent () {}

  /// Create a keyboard event object
  csEvent (long iTime, int eType, int kCode, int kShiftKeys);

  /// Create a mouse event object
  csEvent (long iTime, int eType, int mx, int my, int mbutton, int mShiftKeys);

  /// Create a command event object
  csEvent (long iTime, int eType, int cCode, void *cInfo = NULL);

  /// Destroy an event object
  virtual ~csEvent ();
};

#endif // __CSEVENT_H__
