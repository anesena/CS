/*
    DOS support for Crystal Space 3D library
    Copyright (C) 1998 by Jorrit Tyberghein
    Written by David N. Arnold <derek_arnold@fuse.net>
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

#ifndef __IDJGPP_H__
#define __IDJGPP_H__

#include "csutil/scf.h"

SCF_VERSION (iDosSystemDriver, 0, 0, 1);

struct iDosSystemDriver : public iBase
{
  /// Enable or disable text-mode CsPrintf
  virtual void EnablePrintf (bool Enable) = 0;
  /// Set mouse position since mouse driver is part of system driver
  virtual bool SetMousePosition (int x, int y) = 0;
};

#endif // __IDJGPP_H__
