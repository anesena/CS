/*
    Copyright (C) 2001 by Martin Geisse <mgeisse@gmx.net>

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
#include "cssysdef.h"
#include "csutil/garray.h"
#include "plugins/engine/3d/engine.h"
#include "plugins/engine/3d/rdrprior.h"
#include "iengine/mesh.h"
#include "iengine/rview.h"
#include "iengine/camera.h"
#include "iengine/engine.h"

csRenderQueueSet::csRenderQueueSet ()
{
}

csRenderQueueSet::~csRenderQueueSet ()
{
}

void csRenderQueueSet::ClearVisible ()
{
  int i;
  for (i = 0 ; i < visible.Length () ; i++)
    if (visible[i])
      visible[i]->SetLength (0);
}

void csRenderQueueSet::AddVisible (iMeshWrapper *mesh)
{
  long pri = mesh->GetRenderPriority ();

  // look if the desired priority queue exists, and possibly
  // extend the list of visible.
  if (pri >= visible.Length ())
    visible.SetLength (pri+1);

  // look if the desired queue exists, and create it if not
  if (!visible[pri])
  {
    csArrayMeshPtr* mvnd = new csArrayMeshPtr ();
    visible.Put (pri, mvnd);
  }

  // add the mesh wrapper
  visible[pri]->Push (mesh);
}

void csRenderQueueSet::Add (iMeshWrapper *mesh)
{
  long pri = mesh->GetRenderPriority ();

  // If the CS_ENTITY_CAMERA flag is set we automatically mark
  // the render priority with the camera flag.
  bool do_camera = mesh->GetFlags ().Check (CS_ENTITY_CAMERA);
  if (do_camera)
  {
    csEngine::current_engine->SetRenderPriorityCamera (pri, do_camera);
  }
  else
  {
    // We only add CAMERA meshes!
    return;
  }

  camera_meshes.Push (mesh);
}

void csRenderQueueSet::Remove (iMeshWrapper *mesh)
{
  camera_meshes.Delete (mesh);
}

void csRenderQueueSet::RemoveUnknownPriority (iMeshWrapper *mesh)
{
  camera_meshes.Delete (mesh);
}

struct comp_mesh_comp
{
  float z;
  iMeshWrapper *mesh;
};

typedef csDirtyAccessArray<comp_mesh_comp> engine3d_comp_mesh_z;
CS_IMPLEMENT_STATIC_VAR (GetStaticComp_Mesh_Comp, engine3d_comp_mesh_z,())

static int comp_mesh (const void *el1, const void *el2)
{
  comp_mesh_comp *m1 = (comp_mesh_comp *)el1;
  comp_mesh_comp *m2 = (comp_mesh_comp *)el2;
  if (m1->z < m2->z)
    return -1;
  else if (m1->z > m2->z)
    return 1;
  else
    return 0;
}

iMeshWrapper** csRenderQueueSet::SortAll (iRenderView* rview,
	int& tot_num, uint32 current_visnr)
{
  tot_num = 0;

  int tot_objects = 0;
  int priority;
  for (priority = 0 ; priority < visible.Length () ; priority++)
  {
    Sort (rview, priority);
    csArrayMeshPtr* v = visible[priority];
    if (v)
      tot_objects += v->Length ();
  }
  if (!tot_objects) return 0;

  iMeshWrapper** meshes = new iMeshWrapper* [tot_objects];
  for (priority = 0 ; priority < visible.Length () ; priority++)
  {
    csArrayMeshPtr* v = visible[priority];
    if (v)
      for (int i = 0 ; i < v->Length () ; i++)
      {
        iMeshWrapper *sp = v->Get (i);
        meshes[tot_num++] = sp;
      }
  }

  return meshes;
}

void csRenderQueueSet::Sort (iRenderView *rview, int priority)
{
  static engine3d_comp_mesh_z &comp_mesh_z = *GetStaticComp_Mesh_Comp ();
  if (!visible[priority]) return ;

  int rendsort = csEngine::current_engine->GetRenderPrioritySorting (
      priority);
  if (rendsort == CS_RENDPRI_NONE) return ;

  csArrayMeshPtr *v = visible[priority];
  if (v->Length () > comp_mesh_z.Length ())
    comp_mesh_z.SetLength (v->Length ());

  const csReversibleTransform &camtrans = rview->GetCamera ()->GetTransform ();
  int i;
  for (i = 0; i < v->Length (); i++)
  {
    iMeshWrapper *mesh = v->Get (i);
    csVector3 rad, cent;
    mesh->GetRadius (rad, cent);

    csReversibleTransform tr_o2c = camtrans;
    iMovable* movable = mesh->GetMovable ();
    if (!movable->IsFullTransformIdentity ())
      tr_o2c /= movable->GetFullTransform ();
    csVector3 tr_cent = tr_o2c.Other2This (cent);
    comp_mesh_z[i].z = rendsort == CS_RENDPRI_FRONT2BACK
    	? tr_cent.z
	: -tr_cent.z;
    comp_mesh_z[i].mesh = mesh;
  }

  qsort (
    comp_mesh_z.GetArray (),
    v->Length (),
    sizeof (comp_mesh_comp),
    comp_mesh);

  for (i = 0; i < v->Length (); i++)
  {
    (*v)[i] = comp_mesh_z[i].mesh;
  }

  return ;
}

