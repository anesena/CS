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
#ifndef __DLLENTRY_H__
#define __DLLENTRY_H__

#include "sysdef.h"
#include "cscom/com.h"

#ifdef OS_WIN32
// ModuleHandle is defined once in libCsCOM
extern HINSTANCE ModuleHandle;

// Main DLL entry point... should be called when we're loaded.
#define IMPLEMENT_DLLENTRY extern "C" BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID /*lpvReserved*/) \
{ \
  if (fdwReason == DLL_PROCESS_ATTACH) \
  { \
    ModuleHandle = hinstDLL; \
    DllInitialize (); \
  } \
 \
  return TRUE; \
}

#elif (defined(OS_UNIX)&&(!defined(CS_STATIC_LINKED)))
#define IMPLEMENT_DLLENTRY int main (int /* argc */, char** /*argv*/) \
{ \
  return 0; \
}
#else
#define IMPLEMENT_DLLENTRY
#endif // OS_WIN32

#endif // ! __DLLENTRY_H__
