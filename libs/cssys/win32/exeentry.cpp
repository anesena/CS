/*
    Copyright (C) 1998 by Jorrit Tyberghein
  
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

#include "sysdef.h"
#include "cscom/com.h"

#if defined(COMP_BC)
#include <dos.h>		// For _argc & _argv
#endif

#undef main
extern int csMain (int argc, char* argv[]);

HINSTANCE ModuleHandle;
bool ApplicationActive = true;
int ApplicationShow;

// The main entry for GUI applications
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
  LPSTR lpCmdLine, int nCmdShow)
{
  ModuleHandle = hInstance;
  ApplicationShow = nCmdShow;

#ifdef COMP_BC
  csMain ( _argc,  _argv);
#else
  csMain (__argc, __argv);
#endif

  return TRUE;
}

// The main entry for console applications
int main (int argc, char* argv[])
{
  // hInstance is really the handle of module
  ModuleHandle = GetModuleHandle (NULL);
  return csMain (argc, argv);
}
