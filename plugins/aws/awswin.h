#ifndef __AWS_WINDOW_H__
#define __AWS_WINDOW_H__
/**************************************************************************
    Copyright (C) 2000-2001 by Christopher Nelson
    
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
*****************************************************************************/
#include "ivaria/aws.h"
#include "isys/plugin.h"
#include "csgeom/csrect.h"
#include "awscomp.h"

/***************************************************************************************************************************
*   This file details the interface of awsWindow components.  Windows, while they are just normal components, have some    *
* special properties that most other components will not have.  For example, windows are moveable, hideable, maximizeable, *
* windows can be minimized, closed, and they can sit on top of or below other windows.  Windows, while being peers, also   *
* have a depth ordering that is implicit in their hierarchy.                                                               *
*                                                                                                                          *
***************************************************************************************************************************/

class awsWindow : public awsComponent
{
private:
    /// Pointer to the window above this one.  Is null if there isn't one.
    awsWindow *above;

    /// Pointer to the window below this one.  Is null if there isn't one.
    awsWindow *below;

    /// Unlinks this window from the window hierarchy.
    void Unlink();

    /// Links this window in above the passed in window.  This window must be unlinked!
    void LinkAbove(awsWindow *win);

    /// Links this window in below the passed in window.  This window must be unlinked!
    void LinkBelow(awsWindow *win);

private:
    /** Contains the redraw tag.  This tag changes everytime we redraw the window system, but only once per frame.
      we use it to keep track of which windows have been redrawn and which haven't.
     */
    unsigned int redraw_tag;

    /// The frame style of the window
    unsigned int frame_style;

    /// Individual frame options
    unsigned int frame_options;

    /// The size of the title bar as of last draw
    int  title_bar_height;

    /// The last values for x and y so that we can create deltas for moving
    int  last_x, last_y;

    /// True if we are currently resizing
    bool resizing_mode;

    /// True if we are currently moving
    bool moving_mode;

public:
   static const unsigned long sWindowRaised;
   static const unsigned long sWindowLowered;
   static const unsigned long sWindowHidden;
   static const unsigned long sWindowShown;
   static const unsigned long sWindowClosed;

   /******* Frame Styles **********************/

   /// A normal frame that may have a title bar, buttons, 3d border, and a grip.
   static const unsigned int fsNormal;

   /// A frame with only a 3d border.  No controls or decorations allowed.
   static const unsigned int fsToolbar;

   /// A frame drawn with the background and overlay elements.  Nothing else is drawn.
   static const unsigned int fsBitmap;

   /******* Frame Options **********************/

   /// Should draw control box
   static const unsigned int foControl;

   /// Should draw zoom (maximize) button
   static const unsigned int foZoom;

   /// Should draw minimize button
   static const unsigned int foMin;

   /// Should draw close button
   static const unsigned int foClose;

   /// Should draw title
   static const unsigned int foTitle;

   /// Should draw grip
   static const unsigned int foGrip;

   /// Should draw round border (default)
   static const unsigned int foRoundBorder;
   
   /// Should draw beveled border
   static const unsigned int foBeveledBorder;
   
   SCF_DECLARE_IBASE;
   
   /// This is a component of type window
   virtual char *Type()
   {
      return "Window";
   }

   /// Sets the value of the redraw tag
   void SetRedrawTag(unsigned int tag);

   /// Gets the value of the redraw tag
   unsigned int RedrawTag()
   { return redraw_tag; }

public:
    /// Constructs window class, clear some variables to defaults
    awsWindow();

    /// empty deconstructor
    virtual ~awsWindow();

public:
    /// Raises a window to the top.
    void Raise();

    /// Lowers a window to the bottom.
    void Lower();

    /// Get's the window above this one, NULL if there is none.
    awsWindow *WindowAbove()
    { return above; }

    /// Get's the window below this one, NULL if there is none.
    awsWindow *WindowBelow()
    { return below; }

    /// Does some additional setup for windows, including linking into the window hierarchy.
    virtual bool Setup(iAws *_wmgr, awsComponentNode *settings);

public:
    /// Event triggered when a window is about to be raised
    virtual void OnRaise();

    /// Event triggered when a window is about to be lowered
    virtual void OnLower();

    /// Triggered when the component needs to draw
    virtual void OnDraw(csRect clip);

    /// Triggered when the user presses a mouse button down
    virtual bool OnMouseDown(int button, int x, int y);
    
    /// Triggered when the user unpresses a mouse button 
    virtual bool OnMouseUp(int button, int x, int y);
    
    /// Triggered when the user moves the mouse
    virtual bool OnMouseMove(int button, int x, int y);

    /// Triggered when the user clicks the mouse
    virtual bool OnMouseClick(int button, int x, int y);

    /// Triggered when the user double clicks the mouse
    virtual bool OnMouseDoubleClick(int button, int x, int y);

    /// Triggered when this component loses mouse focus
    virtual bool OnMouseExit();

    /// Triggered when this component gains mouse focus
    virtual bool OnMouseEnter();

    /// Triggered when the user presses a key
    virtual bool OnKeypress(int key, int modifiers);
    
    /// Triggered when the keyboard focus is lost
    virtual bool OnLostFocus();

    /// Triggered when the keyboard focus is gained
    virtual bool OnGainFocus();
};

#endif
