/*
    Sound Driver interface DLL

    Copyright (C) 1998, 1999 by Nathaniel 'NooTe' Saint Martin
    Copyright (C) 1998, 1999 by Jorrit Tyberghein
    Written by Nathaniel 'NooTe' Saint Martin

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

#include <stdlib.h>
#include "sysdef.h"
#include "cscom/com.h"
#include "cssnddrv/macsnd/macsdrv.h"
#include "isnddrv.h"

static unsigned int gRefCount = 0;
static DllRegisterData gRegData =
{
  &CLSID_MacSoundDriver,
  "crystalspace.sound.driver.mac",
  "Crystal Space Mac Sound Driver"
};

#ifdef CS_STATIC_LINKED

void SoundDriverMacRegister ()
{
  static csSoundDriverMacFactory gNullFactory;
  gRegData.pClass = &gNullFactory;
  csRegisterServer (&gRegData);
}

void SoundDriverMacUnregister ()
{
  csUnregisterServer (&gRegData);
}

#else

// This is the name of the DLL. Make sure to change this if you change the DLL name!
// DAN: this might have to be changed for each OS, cuz each OS has a different extension for DLLs.
#define DLL_NAME "snddrv.shlb"

// our main entry point...should be called when we're loaded.
STDAPI DllInitialize ()
{
  csCoInitialize (0);
  gRegData.szInProcServer = DLL_NAME;
  return TRUE;
}

void STDAPICALLTYPE ModuleRelease ()
{
  gRefCount--;
}

void STDAPICALLTYPE ModuleAddRef ()
{
  gRefCount++;
}

// return S_OK if it's ok to unload us now.
STDAPI DllCanUnloadNow ()
{
  return gRefCount ? S_FALSE : S_OK;
}

// used to get a COM class object from us.
STDAPI DllGetClassObject (REFCLSID rclsid, REFIID riid, void** ppv)
{
  static csSoundDriverMacFactory gFactory;
  if (rclsid == CLSID_MacSoundDriver)
    return gFactory.QueryInterface(riid, ppv);

  //  if we get here, rclsid is a class we don't implement
  *ppv = NULL;
  return CLASS_E_CLASSNOTAVAILABLE;
}

// Called by RegSvr32.exe
STDAPI DllRegisterServer ()
{
  return csRegisterServer (&gRegData);
}

// Called by RegSvr32.exe
STDAPI DllUnregisterServer ()
{
  return csUnregisterServer(&gRegData);
}

#endif

// Implementation of csSoundDriverMacFactory

IMPLEMENT_UNKNOWN_NODELETE (csSoundDriverMacFactory)

BEGIN_INTERFACE_TABLE (csSoundDriverMacFactory)
  IMPLEMENTS_INTERFACE (ISoundDriverFactory)
END_INTERFACE_TABLE ()

STDMETHODIMP csSoundDriverMacFactory::CreateInstance (REFIID riid, ISystem* piSystem, void** ppv)
{
  if (!piSystem)
  {
    *ppv = 0;
    return E_UNEXPECTED;
  }

  csSoundDriverMac* pNew = new csSoundDriverMac (piSystem);
  if (!pNew)
  {
    *ppv = 0;
    return E_OUTOFMEMORY;
  }

  return pNew->QueryInterface (riid, ppv);
}

STDMETHODIMP csSoundDriverMacFactory::LockServer (COMBOOL bLock)
{
  if (bLock)
    gRefCount++;
  else
    gRefCount--;

  return S_OK;
}
