/*
    Copyright (C) 2000 by Jorrit Tyberghein

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

#ifndef __CS_DTMESH_H__
#define __CS_DTMESH_H__

#include "ivideo/igraph3d.h"
class csClipper;

void DefaultDrawTriangleMesh (
  G3DTriangleMesh& mesh,
  iGraphics3D* g3d,
  csReversibleTransform& o2c,
  csClipper* clipper,
  float aspect,
  int width2,
  int height2);

#endif // __CS_DTMESH_H__
