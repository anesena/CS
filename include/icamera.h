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

#ifndef __ICAMERA_H__
#define __ICAMERA_H__

#include "csutil/scf.h"

SCF_VERSION (iCamera, 0, 0, 1);

/// temporary - subject to change
struct iCamera : public iBase
{
  ///
  virtual float GetAspect () = 0;
  ///
  virtual float GetInvAspect () = 0;
};

#endif
