/*
    Copyright (C) 2003 by Jorrit Tyberghein
	      (C) 2003 by Frank Richter
              (C) 2003 by Anders Stenberg

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

#ifndef __CS_CSENGINE_RENDERLOOP_H__
#define __CS_CSENGINE_RENDERLOOP_H__

//#include "csutil/hashmapr.h"
#include "csutil/hashr.h"
#include "csutil/refarr.h"
#include "iengine/renderloop.h"
#include "iutil/strset.h"
#include "ivideo/graph3d.h"
#include "ivideo/shader/shader.h"
#include "ivideo/rendersteps/irenderstep.h"

class csEngine;
class csRenderView;
class csRenderLoop;

class csRenderLoop : public iRenderLoop
{
protected:
  friend class csLightIteratorRenderStep;
  friend class csGenericRenderStep;

  csEngine* engine;

  csRefArray<iRenderStep> steps;
public:
  SCF_DECLARE_IBASE;

  csRenderLoop (csEngine* engine);
  virtual ~csRenderLoop();

  void StartDraw (iCamera *c, iClipper2D *view, csRenderView &rview);
  virtual void Draw (iCamera* c, iClipper2D* clipper);

  virtual int AddStep (iRenderStep* step);
  virtual int GetStepCount ();
};

class StringHashKeyHandler
{
public:
  static uint32 ComputeHash (const char* const& key)
  {
    return (csHashCompute (key));
  }

  static bool CompareKeys (const char* const& key1, const char* const& key2)
  {
    return (strcmp (key1, key2) == 0);
  }
};

template <class T>
class csRefHashKeyHandler
{
public:
  static uint32 ComputeHash (const csRef<T>& key)
  {
    return (uint32)((T*)key);
  }

  static bool CompareKeys (const csRef<T>& key1, const csRef<T>& key2)
  {
    return ((T*)key1 == (T*)key2);
  }
};

class csRenderLoopManager : public iRenderLoopManager
{
  //csHashMapReversible loops;
  csHashReversible<csRef<iRenderLoop>, const char*, 
    StringHashKeyHandler, csRefHashKeyHandler<iRenderLoop> > loops;
  csStringSet strings;

  csEngine* engine;
public:
  SCF_DECLARE_IBASE;

  csRenderLoopManager(csEngine* engine);
  virtual ~csRenderLoopManager();

  virtual csPtr<iRenderLoop> Create ();
  
  virtual bool Register (const char* name, iRenderLoop* loop);
  virtual iRenderLoop* Retrieve (const char* name);
  virtual const char* GetName (iRenderLoop* loop);
  virtual bool Unregister (iRenderLoop* loop);

  /**
   * Load a renderloop from VFS file named \p file.
   */
  csPtr<iRenderLoop> Load (const char* fileName);
};

#endif
