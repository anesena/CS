//=============================================================================
//
//	Copyright (C)1999-2001 by Eric Sunshine <sunshine@sunshineco.com>
//
// The contents of this file are copyrighted by Eric Sunshine.  This work is
// distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE.  You may distribute this file provided that this
// copyright notice is retained.  Send comments to <sunshine@sunshineco.com>.
//
//=============================================================================
//-----------------------------------------------------------------------------
// NeXTDelegate.m
//
//	The application's delegate.  Acts as a gateway between the AppKit and
//	Crystal Space by forwarding Objective-C messages and events to the C++
//	system driver, NeXTSystemDriver.
//
//-----------------------------------------------------------------------------
#include "NeXTDelegate.h"
#include "NeXTKeymap.h"
#include "NeXTMenu.h"
#include "isys/ievdefs.h"
#include "cssys/next/NeXTSystemDriver.h"
#include <errno.h>
#include <libc.h>
#include <string.h>
#include <unistd.h>
#import <appkit/Application.h>
#import <appkit/Menu.h>
#import <appkit/View.h>
#import <defaults/defaults.h>
#import <dpsclient/wraps.h>

typedef void* NeXTDelegateHandle;
typedef void* NeXTEventHandle;
typedef void* NeXTViewHandle;
#define ND_PROTO(RET,FUNC) RET NeXTDelegate_##FUNC

#if !defined(NX_FUNCTIONSET)
#define NX_FUNCTIONSET 0xfe /* Cousin to NX_ASCIISET and NX_SYMBOLSET. */
#endif

//-----------------------------------------------------------------------------
// For each keystroke, Crystal Space expects a raw key code and a cooked
// character code.  For ASCII codes, Crystal Space expects the raw key code to
// be in canonic lower-case form, and the cooked character code to be in
// native cooked form.  Here are some examples:
//
//      Input    Raw  Cooked
//      -------  ---  ------
//      ctrl-C   c    ^C
//      d        d    d
//      shift-e  e    E
//      F        f    F
//      alt-m    m    "mu" *
//      shift-4  4    $
//
// (*) For alt-m, the cooked character depends upon the user's current key
// mapping.  It may actually be mapped to any character, but is often mapped to
// Greek "mu", as it was in this example.
//
// The following table translates all raw ASCII key codes to their lower-case
// equivalents in order to satisfy Crystal Space's canonic form requirement.
//-----------------------------------------------------------------------------
static char const CS_DOWN_CASE[] =
{
'2', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\',']', '6', '-',
' ', '1', '\'','3', '4', '5', '7', '\'','9', '0', '8', '=', ',', '-', '.', '/',
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ';', ';', ',', '=', '.', '/',
'2', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\',']', '6', '-',
'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\',']', '`', 127
};


//-----------------------------------------------------------------------------
// Keystrokes which must be translated to Crystal Space-specific codes.
//-----------------------------------------------------------------------------
enum
    {
    K_TAB		= '\t',		// ascii-set
    K_RETURN		= '\r',		// ascii-set
    K_BACKSPACE		= '\b',		// ascii-set
    K_ESCAPE		= 0x1b,		// ascii-set
    K_DELETE		= 0x7f,		// ascii-set
    K_LEFT		= 0xac,		// symbol-set, numeric-pad mask
    K_UP		= 0xad,		// symbol-set, numeric-pad mask
    K_RIGHT		= 0xae,		// symbol-set, numeric-pad mask
    K_DOWN		= 0xaf,		// symbol-set, numeric-pad mask
    K_KP_CENTER		= '5',		// ascii-set, numeric-pad mask
    K_KP_LEFT		= '4',		// ascii-set, numeric-pad mask
    K_KP_UP		= '8',		// ascii-set, numeric-pad mask
    K_KP_RIGHT		= '6',		// ascii-set, numeric-pad mask
    K_KP_DOWN		= '2',		// ascii-set, numeric-pad mask
    K_KP_PAGE_UP	= '9',		// ascii-set, numeric-pad mask
    K_KP_PAGE_DOWN	= '3',		// ascii-set, numeric-pad mask
    K_KP_HOME		= '7',		// ascii-set, numeric-pad mask
    K_KP_END		= '1',		// ascii-set, numeric-pad mask
    K_KP_INSERT		= '0',		// ascii-set, numeric-pad mask
    K_KP_DELETE		= '.',		// ascii-set, numeric-pad mask
    K_KP_MULTIPLY	= '*',		// ascii-set, numeric-pad mask
    K_KP_DIVIDE		= '/',		// ascii-set, numeric-pad mask
    K_KP_PLUS		= '+',		// ascii-set, numeric-pad mask
    K_KP_MINUS		= '-',		// symbol-set, numeric-pad mask
    K_KP_ENTER		= 0x03,		// function-set
    K_ED_PAGE_UP	= 0x30,		// function-set
    K_ED_PAGE_DOWN	= 0x31,		// function-set
    K_ED_HOME		= 0x2e,		// function-set
    K_ED_END		= 0x2f,		// function-set
    K_ED_INSERT		= 0x2c,		// function-set
    K_ED_DELETE		= 0x2d,		// function-set
    K_F1		= 0x20,		// function-set
    K_F2		= 0x21,		// function-set
    K_F3		= 0x22,		// function-set
    K_F4		= 0x23,		// function-set
    K_F5		= 0x24,		// function-set
    K_F6		= 0x25,		// function-set
    K_F7		= 0x26,		// function-set
    K_F8		= 0x27,		// function-set
    K_F9		= 0x28,		// function-set
    K_F10		= 0x29,		// function-set
    K_F11		= 0x2a,		// function-set
    K_F12		= 0x2b,		// function-set
    };


//-----------------------------------------------------------------------------
// continue_running
//-----------------------------------------------------------------------------
static inline BOOL continue_running( NeXTSystemDriver driver )
    {
    int run;
    NeXTSystemDriver_system_extension( driver, "continuerunning", &run, 0, 0 );
    return run;
    }


//=============================================================================
// Category of Application which supports recursive run loops.
//=============================================================================
@interface Application (NeXTDelegate)
- (void)runRecursively:(NeXTSystemDriver)driver;
@end
@implementation Application (NeXTDelegate)
- (void)runRecursively:(NeXTSystemDriver)driver
    {
    int const was_running = running; // The old run-loop invocation depth.
    [self run];
    if (continue_running( driver ))
	running = was_running; // Restore old depth rather than terminating.
    }
@end


//=============================================================================
// NeXTDelegate Implementation
//=============================================================================
@implementation NeXTDelegate

//-----------------------------------------------------------------------------
// timer_handler
//	Target of timer; forwards timer event to the NeXTSystemDriver.
//-----------------------------------------------------------------------------
static void timer_handler( DPSTimedEntry timer, double now, void* data )
    {
    (void)timer; (void)now;
    NeXTSystemDriver_system_extension(
	(NeXTSystemDriver)data, "timerfired", 0, 0, 0 );
    }


//-----------------------------------------------------------------------------
// startTimer
//-----------------------------------------------------------------------------
- (void)startTimer
    {
    if (timer == 0)
	{
	float const CS_FPS = 15;
	timer = DPSAddTimedEntry( 1.0 / CS_FPS, timer_handler,
		driver, NX_BASETHRESHOLD );
	}
    }


//-----------------------------------------------------------------------------
// stopTimer
//-----------------------------------------------------------------------------
- (void)stopTimer
    {
    if (timer != 0)
	{
	DPSRemoveTimedEntry( timer );
	timer = 0;
	}
    }


//-----------------------------------------------------------------------------
// resetTimer
//-----------------------------------------------------------------------------
- (void)resetTimer
    {
    [self stopTimer];
    [self startTimer];
    }


//-----------------------------------------------------------------------------
// showMouse
//-----------------------------------------------------------------------------
- (void)showMouse
    {
    if (mouseHidden)
	{
	mouseHidden = NO;
	PSshowcursor();
	}
    }

ND_PROTO(void,show_mouse)( NeXTDelegateHandle handle )
    { [(NeXTDelegate*)handle showMouse]; }


//-----------------------------------------------------------------------------
// hideMouse
//-----------------------------------------------------------------------------
- (void)hideMouse
    {
    if (!mouseHidden)
	{
	mouseHidden = YES;
	PShidecursor();
	}
    }

ND_PROTO(void,hide_mouse)( NeXTDelegateHandle handle )
    { [(NeXTDelegate*)handle hideMouse]; }


//-----------------------------------------------------------------------------
// flushGraphicsContext
//-----------------------------------------------------------------------------
- (void)flushGraphicsContext
    {
    DPSFlush();
    }

ND_PROTO(void,flush_graphics_context)( NeXTDelegateHandle handle )
    { [(NeXTDelegate*)handle flushGraphicsContext]; }


//-----------------------------------------------------------------------------
// quit: -- Target of "Quit" menu item.  Terminate the application.
//-----------------------------------------------------------------------------
- (id)quit:(id)sender
    {
    NeXTSystemDriver_system_extension( driver, "requestshutdown", 0, 0, 0 );
    return self;
    }


//-----------------------------------------------------------------------------
// localize:toView:x:y: -- Convert event location to view coordinate system.
//-----------------------------------------------------------------------------
- (BOOL)localize:(NXEvent const*)event toView:(View*)view x:(int*)x y:(int*)y
    {
    NXPoint p = event->location;
    NXRect r; [view getBounds:&r];
    [view convertPoint:&p fromView:0];
    *x = (int)p.x;
    *y = (int)(r.size.height - p.y - 1); // Crystal Space coordinates flipped.
    return (*x >= 0 && *y >= 0 && *x < r.size.width && *y < r.size.height);
    }


//-----------------------------------------------------------------------------
// classifyFunctionSet::
//-----------------------------------------------------------------------------
- (void)classifyFunctionSet:(int*)raw :(int*)cooked
    {
    *cooked = -1;
    switch (*raw)
	{
	case K_ED_PAGE_UP:   *raw = CSKEY_PGUP; break;
	case K_ED_PAGE_DOWN: *raw = CSKEY_PGDN; break;
	case K_ED_HOME:      *raw = CSKEY_HOME; break;
	case K_ED_END:       *raw = CSKEY_END;  break;
	case K_ED_INSERT:    *raw = CSKEY_INS;  break;
	case K_ED_DELETE:    *raw = CSKEY_DEL;  break;
	case K_F1:           *raw = CSKEY_F1;   break;
	case K_F2:           *raw = CSKEY_F2;   break;
	case K_F3:           *raw = CSKEY_F3;   break;
	case K_F4:           *raw = CSKEY_F4;   break;
	case K_F5:           *raw = CSKEY_F5;   break;
	case K_F6:           *raw = CSKEY_F6;   break;
	case K_F7:           *raw = CSKEY_F7;   break;
	case K_F8:           *raw = CSKEY_F8;   break;
	case K_F9:           *raw = CSKEY_F9;   break;
	case K_F10:          *raw = CSKEY_F10;  break;
	case K_F11:          *raw = CSKEY_F11;  break;
	case K_F12:          *raw = CSKEY_F12;  break;
	}
    }


//-----------------------------------------------------------------------------
// classifyAsciiSet::
//	*NOTE* The so-called "backspace" key on the keyboard actually sends
//	DEL, however Crystal Space would like to see it as CSKEY_BACKSPACE.
//-----------------------------------------------------------------------------
- (void)classifyAsciiSet:(int*)raw :(int*)cooked
    {
    switch (*raw)
	{
	case K_ESCAPE:    *raw = CSKEY_ESC;       break;
	case K_RETURN:    *raw = CSKEY_ENTER;     break;
	case K_TAB:       *raw = CSKEY_TAB;       break;
	case K_BACKSPACE: *raw = CSKEY_BACKSPACE; break;
	case K_DELETE:    *raw = CSKEY_BACKSPACE; break; // *NOTE*
	default:
	    if (*raw <= 0x7f) // Is it 7-bit ASCII?
		*raw = CS_DOWN_CASE[ *raw ];
	    break;
	}
    }


//-----------------------------------------------------------------------------
// classifyKeypad::
//-----------------------------------------------------------------------------
- (void)classifyKeypad:(int*)raw :(int*)cooked
    {
    switch (*raw)
	{
	case K_LEFT:         *raw = CSKEY_LEFT;  *cooked = -1;   break;
	case K_RIGHT:        *raw = CSKEY_RIGHT; *cooked = -1;   break;
	case K_UP:           *raw = CSKEY_UP;    *cooked = -1;   break;
	case K_DOWN:         *raw = CSKEY_DOWN;  *cooked = -1;   break;
	case K_KP_CENTER:    *raw = CSKEY_CENTER;                break;
	case K_KP_LEFT:      *raw = CSKEY_LEFT;                  break;
	case K_KP_UP:        *raw = CSKEY_UP;                    break;
	case K_KP_RIGHT:     *raw = CSKEY_RIGHT;                 break;
	case K_KP_DOWN:      *raw = CSKEY_DOWN;                  break;
	case K_KP_PAGE_UP:   *raw = CSKEY_PGUP;                  break;
	case K_KP_PAGE_DOWN: *raw = CSKEY_PGDN;                  break;
	case K_KP_HOME:      *raw = CSKEY_HOME;                  break;
	case K_KP_END:       *raw = CSKEY_END;                   break;
	case K_KP_INSERT:    *raw = CSKEY_INS;                   break;
	case K_KP_DELETE:    *raw = CSKEY_DEL;                   break;
	case K_KP_MULTIPLY:  *raw = CSKEY_PADMULT;               break;
	case K_KP_DIVIDE:    *raw = CSKEY_PADDIV;                break;
	case K_KP_PLUS:      *raw = CSKEY_PADPLUS;               break;
	case K_KP_MINUS:     *raw = CSKEY_PADMINUS;              break;
	case K_KP_ENTER:     *raw = CSKEY_ENTER; *cooked = '\n'; break;
	};
    }


//-----------------------------------------------------------------------------
// classifyKeyDown:raw:cooked:
//	Translate NextStep keystroke to Crystal Space codes.
//-----------------------------------------------------------------------------
- (BOOL)classifyKeyDown:(NXEvent const*)p raw:(int*)raw cooked:(int*)cooked
    {
    BOOL ok = NO;
    *raw = *cooked = 0;
    if ((p->flags & NX_COMMANDMASK) == 0)
	{
	if ([keymap isScanCodeBound:p->data.key.keyCode])
	    {
	    NeXTKeymapBinding const* binding =
		[keymap bindingForScanCode:p->data.key.keyCode];

	    *raw = binding->code;
	    *cooked = p->data.key.charCode;
	    if ((p->flags & NX_NUMERICPADMASK) != 0)
		[self classifyKeypad:raw:cooked];
	    else if (binding->character_set == NX_FUNCTIONSET)
		[self classifyFunctionSet:raw:cooked];
	    else if (binding->character_set == NX_ASCIISET)
		[self classifyAsciiSet:raw:cooked];
	    ok = YES;
	    }
	}
    return ok;
    }


//-----------------------------------------------------------------------------
// check:nx:cs:key:
//	Track state of modifier (shift, alt, ctrl) keys.  NextStep does not
//	supply key up/down events for modifier flags so simulate these events
//	whenever a -flagsChanged: notification is posted.
//-----------------------------------------------------------------------------
- (void)check:(NXEvent const*)p nx:(unsigned long)nxmask
    cs:(unsigned long)csmask key:(int)key
    {
    BOOL const new_state = ((p->flags  & nxmask) != 0);
    BOOL const old_state = ((modifiers & csmask) != 0);
    if (new_state != old_state)
	{
	char const* request;
	if (new_state)
	    {
	    modifiers |= csmask;
	    request = "keydown";
	    }
	else
	    {
	    modifiers &= ~csmask;
	    request = "keyup";
	    }
	NeXTSystemDriver_system_extension(
	    driver, request, (void*)key, (void*)-1, 0 );
	}
    }


//-----------------------------------------------------------------------------
// Keyboard
//-----------------------------------------------------------------------------
- (void)keyEvent:(NXEvent*)p down:(BOOL)flag
    {
    if (!paused)
	{
	int raw, cooked;
	if ([self classifyKeyDown:p raw:&raw cooked:&cooked])
	    {
	    char const* request = flag ? "keydown" : "keyup";
	    NeXTSystemDriver_system_extension(
		driver, request, (void*)raw, (void*)cooked, 0 );
	    }
	}
    }

- (void)keyDown:(NXEvent*)p forView:(View*)v
    {
    [self keyEvent:p down:YES];
    }

- (void)keyUp:(NXEvent*)p forView:(View*)v
    {
    [self keyEvent:p down:NO];
    }

- (void)flagsChanged:(NXEvent*)p forView:(View*)v
    {
    if (!paused)
	{
	[self check:p nx:NX_SHIFTMASK     cs:CSMASK_SHIFT key:CSKEY_SHIFT];
	[self check:p nx:NX_ALTERNATEMASK cs:CSMASK_ALT   key:CSKEY_ALT  ];
	[self check:p nx:NX_CONTROLMASK   cs:CSMASK_CTRL  key:CSKEY_CTRL ];
	}
    }


//-----------------------------------------------------------------------------
// Mouse -- Note: Crystal Space button numbers start at 1.
//-----------------------------------------------------------------------------
- (void)mouseMoved:(NXEvent*)p forView:(View*)v
    {
    if (!paused)
	{
	int x, y;
	if ([self localize:p toView:v x:&x y:&y])
	    NeXTSystemDriver_system_extension(
		driver, "mousemoved", (void*)x, (void*)y, 0 );
	}
    }

- (void)mouseUp:(NXEvent*)p forView:(View*)v button:(int)button
    {
    if (!paused)
	{
	int x, y;
	[self localize:p toView:v x:&x y:&y];
	NeXTSystemDriver_system_extension(
	    driver, "mouseup", (void*)button, (void*)x, (void*)y );
	}
    }

- (void)mouseDown:(NXEvent*)p forView:(View*)v button:(int)button
    {
    if (!paused)
	{
	int x, y;
	[self localize:p toView:v x:&x y:&y];
	NeXTSystemDriver_system_extension(
	    driver, "mousedown", (void*)button, (void*)x, (void*)y );
	}
    }

- (void)mouseDragged:(NXEvent*)p forView:(View*)v
    { [self mouseMoved:p forView:v]; }

- (void)mouseUp:(NXEvent*)p forView:(View*)v
    { [self mouseUp:p forView:v button:1]; }

- (void)mouseDown:(NXEvent*)p forView:(View*)v
    { [self mouseDown:p forView:v button:1]; }

- (void)rightMouseDragged:(NXEvent*)p forView:(View*)v
    { [self mouseMoved:p forView:v]; }

- (void)rightMouseUp:(NXEvent*)p forView:(View*)v
    { [self mouseUp:p forView:v button:2]; }

- (void)rightMouseDown:(NXEvent*)p forView:(View*)v
    {
    if (paused)
	[[v nextResponder] rightMouseDown:p]; // Allow main menu to pop up.
    else
	[self mouseDown:p forView:v button:2];
    }

- (void)dispatchEvent:(NXEvent*)e forView:(View*)v
    {
    switch (e->type)
	{
	case NX_KEYDOWN:       [self keyDown:e           forView:v]; break;
	case NX_KEYUP:         [self keyUp:e             forView:v]; break;
	case NX_FLAGSCHANGED:  [self flagsChanged:e      forView:v]; break;
	case NX_MOUSEMOVED:    [self mouseMoved:e        forView:v]; break;
	case NX_LMOUSEDOWN:    [self mouseDown:e         forView:v]; break;
	case NX_LMOUSEUP:      [self mouseUp:e           forView:v]; break;
	case NX_LMOUSEDRAGGED: [self mouseDragged:e      forView:v]; break;
	case NX_RMOUSEDOWN:    [self rightMouseDown:e    forView:v]; break;
	case NX_RMOUSEUP:      [self rightMouseUp:e      forView:v]; break;
	case NX_RMOUSEDRAGGED: [self rightMouseDragged:e forView:v]; break;
	default:                                                     break;
	}
    }

ND_PROTO(void,dispatch_event)
    ( NeXTDelegateHandle h, NeXTEventHandle event, NeXTViewHandle view )
    { [(NeXTDelegate*)h dispatchEvent:(NXEvent*)event forView:(View*)view]; }


//-----------------------------------------------------------------------------
// Activation / Deactivation
//-----------------------------------------------------------------------------
- (void)pause
    {
    if (!paused)
	{
	paused = YES;
	[self showMouse];
	[self stopTimer];
	NeXTSystemDriver_system_extension( driver, "appdeactivated", 0, 0, 0 );
	}
    }

- (void)unpause
    {
    if (paused)
	{
	paused = NO;
	[self startTimer];
	NeXTSystemDriver_system_extension( driver, "appactivated", 0, 0, 0 );
	}
    }

- (id)togglePause:(id)sender
    {
    if (paused)
	[self unpause];
    else
	[self pause];
    return self;
    }

- (id)appDidBecomeActive:(id)sender
    {
    if (autoResume)
	[self unpause];
    return self;
    }

- (id)appDidResignActive:(id)sender
    {
    autoResume = !paused;
    [self pause];
    return self;
    }


//-----------------------------------------------------------------------------
// initApplicationMenu:style:
//	Generate application menu based upon platform configuration.
//-----------------------------------------------------------------------------
- (void)initApplicationMenu:(NeXTConfigHandle)config style:(char const*)style
    {
    Menu* const menu = NeXTMenuGenerate( style, config );
    [menu setTitle:[NXApp appName]];
    [NXApp setMainMenu:menu];
    }

ND_PROTO(void,init_app_menu)
    ( NeXTDelegateHandle handle, NeXTConfigHandle config, char const* style )
    { [(NeXTDelegate*)handle initApplicationMenu:config style:style]; }


//-----------------------------------------------------------------------------
// startEventLoop
//	Begin running an event-loop.  May be called recursively by CSWS.  Uses
//	special -runRecursively: method to handle recursive invocations of
//	event-loop.
//
// *CONTINUE-RUNNING*
//	Since -startEventLoop may be invoked recursively, special care must be
//	taken to avoid starting the new event-loop if the user has requested
//	application termination via an external means, such as closing the
//	AppKit window.  When the user does perform an external request for
//	termination, we forcibly terminate all CSWS modal loops.
//	Unfortunately, however, code which employs CSWS may not itself check
//	that termination has been requested, and may try launching another
//	modal session by recursively invoking -startEventLoop immediately
//	following the one which was just forcibly aborted.  In order to prevent
//	another modal session from actually launching in this circumstance we
//	check continue_running() before even starting the event-loop.
//
// *RESET-TIMER*
//	Because of the way timers are handled by the AppKit, if -startEventLoop
//	is called recursively while the timer is being fired, the timer will
//	not fire again until the recursive event-loop invocation exits.  Since
//	we want the timer to fire even in recursive invocations, it is
//	necessary to cancel it and re-install it.  Furthermore, since
//	-stopEventLoop cancels the timer when exiting an event-loop, we restart
//	the timer again after the recursive event-loop invocation has exited,
//	but only if the user has not requested application termination.
//-----------------------------------------------------------------------------
- (void)startEventLoop
    {
    if (continue_running( driver ))	// *CONTINUE-RUNNING*
	{
	[self resetTimer];		// *RESET-TIMER*
	[NXApp runRecursively:driver];
	if (continue_running( driver ))
	    [self resetTimer];		// *RESET-TIMER*
	}
    }

ND_PROTO(void,start_event_loop)( NeXTDelegateHandle handle )
    { [(NeXTDelegate*)handle startEventLoop]; }


//-----------------------------------------------------------------------------
// stopEventLoop
//	Stops the application's event-loop.  Unfortunately the run-loop does
//	not actually stop until another event arrives, so we fake one up.
//-----------------------------------------------------------------------------
- (void)stopEventLoop
    {
    NXEvent e;
    e.type = NX_APPDEFINED;
    e.ctxt = [NXApp context];
    e.time = 0;
    e.flags = 0;
    e.window = 0;

    [self stopTimer];
    [NXApp stop:0];
    DPSPostEvent( &e, TRUE );
    }

ND_PROTO(void,stop_event_loop)( NeXTDelegateHandle handle )
    { [(NeXTDelegate*)handle stopEventLoop]; }


//-----------------------------------------------------------------------------
// initWithDriver:
//-----------------------------------------------------------------------------
- (id)initWithDriver:(NeXTSystemDriver)p
    {
    [super init];
    driver = p;
    keymap = [[NeXTKeymap alloc] init];
    timer = 0;
    modifiers = 0;
    mouseHidden = NO;
    paused = NO;
    autoResume = NO;
    return self;
    }


//-----------------------------------------------------------------------------
// free
//-----------------------------------------------------------------------------
- (id)free
    {
    [self stopTimer];
    [keymap free];
    return [super free];
    }


//-----------------------------------------------------------------------------
// locateResourceDirectory
//	If the CrystalSpaceRoot default value is set, then its value is used to
//	set the current working directory.  This will allow Crystal Space to
//	locate its resource files even when launched from the Workspace which
//	does not otherwise provide a meaningful working directory.  However, if
//	CrystalSpaceRootIgnore is set to "yes", then ignore the
//	CrystalSpaceRoot setting.  This can be used to force a particular
//	application to ignore a CrystalSpaceRoot setting inherited from the
//	"global" domain.
//-----------------------------------------------------------------------------
+ (void)locateResourceDirectory
    {
    char const* s =
	NXGetDefaultValue( [NXApp appName], "CrystalSpaceRootIgnore" );
    if (s == 0 || strchr( "YyTtOo1", *s ) == 0) // Yes, True, On, 1
	{
	s = NXGetDefaultValue( [NXApp appName], "CrystalSpaceRoot" );
	if (s != 0 && chdir(s) != 0)
	    fprintf( stderr,
		"\nWARNING: Unable to set working directory from "
		    "`CrystalSpaceRoot'"
		"\nWARNING: Value: %s"
		"\nWARNING: Reason: %s\n\n", s, strerror(errno) );
	}
    }


//-----------------------------------------------------------------------------
// startup
//	Interaction with AppKit is initiated here with instantiation of an
//	Application object and a NeXTDelegate which oversees AppKit-related
//	events and messages.
//-----------------------------------------------------------------------------
+ (NeXTDelegate*)startup:(NeXTSystemDriver)handle
    {
    NeXTDelegate* controller;
    NXApp = [Application new];
    DPSSetDeadKeysEnabled( [NXApp context], 0 );
    controller = [[NeXTDelegate alloc] initWithDriver:handle];
    [NXApp setDelegate:controller];
    [self locateResourceDirectory];
    return controller;
    }

ND_PROTO(NeXTDelegateHandle,startup)( NeXTSystemDriver handle )
    { return (NeXTDelegateHandle)[NeXTDelegate startup:handle]; }


//-----------------------------------------------------------------------------
// shutdown:
//-----------------------------------------------------------------------------
+ (void)shutdown:(NeXTDelegate*)controller
    {
    [NXApp setDelegate:0];
    [controller showMouse];
    [controller flushGraphicsContext]; // Flush any pending `showcursor'.
    [controller free];
    }

ND_PROTO(void,shutdown)( NeXTDelegateHandle handle )
    { [NeXTDelegate shutdown:(NeXTDelegate*)handle]; }

@end

#undef ND_PROTO
