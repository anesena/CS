/*
    Copyright (C) 2002 by Jorrit Tyberghein

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

#include <string.h>
#include "cssysdef.h"
#include "csver.h"
#include "csutil/scf.h"
#include "csutil/util.h"
#include "csutil/scfstr.h"
#include "csgeom/matrix3.h"
#include "iutil/objreg.h"
#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
#include "ivideo/txtmgr.h"
#include "iengine/movable.h"
#include "iengine/rview.h"
#include "iengine/camera.h"
#include "iutil/object.h"
#include "dynavis.h"
#include "kdtree.h"
#include "covbuf.h"

CS_IMPLEMENT_PLUGIN

SCF_IMPLEMENT_FACTORY (csDynaVis)

SCF_EXPORT_CLASS_TABLE (dynavis)
  SCF_EXPORT_CLASS (csDynaVis, "crystalspace.culling.dynavis",
    "Dynamic Visibility System")
SCF_EXPORT_CLASS_TABLE_END

SCF_IMPLEMENT_IBASE (csDynaVis)
  SCF_IMPLEMENTS_INTERFACE (iVisibilityCuller)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iDebugHelper)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csDynaVis::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csDynaVis::DebugHelper)
  SCF_IMPLEMENTS_INTERFACE (iDebugHelper)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

csDynaVis::csDynaVis (iBase *iParent)
{
  SCF_CONSTRUCT_IBASE (iParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiComponent);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiDebugHelper);
  object_reg = NULL;
  kdtree = NULL;
  covbuf = NULL;
  debug_camera = NULL;
}

csDynaVis::~csDynaVis ()
{
  int i;
  for (i = 0 ; i < visobj_vector.Length () ; i++)
  {
    csVisibilityObjectWrapper* visobj_wrap = (csVisibilityObjectWrapper*)
    	visobj_vector[i];
    visobj_wrap->visobj->DecRef ();
    delete visobj_wrap;
  }
  delete kdtree;
  delete covbuf;
}

bool csDynaVis::Initialize (iObjectRegistry *object_reg)
{
  csDynaVis::object_reg = object_reg;

  delete kdtree;
  delete covbuf;

  iGraphics3D* g3d = CS_QUERY_REGISTRY (object_reg, iGraphics3D);
  int w, h;
  if (g3d)
  {
    w = g3d->GetWidth ();
    h = g3d->GetHeight ();
    g3d->DecRef ();
  }
  else
  {
    // If there is no g3d we currently assume we are testing.
    w = 640;
    h = 480;
  }

  kdtree = new csKDTree (NULL);
  covbuf = new csCoverageBuffer (w, h);

  return true;
}

void csDynaVis::Setup (const char* /*name*/)
{
}

void csDynaVis::CalculateVisObjBBox (iVisibilityObject* visobj, csBox3& bbox)
{
  iMovable* movable = visobj->GetMovable ();
  csBox3 box;
  visobj->GetBoundingBox (box);
  csReversibleTransform trans = movable->GetFullTransform ();
  bbox.StartBoundingBox (trans.This2Other (box.GetCorner (0)));
  bbox.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (1)));
  bbox.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (2)));
  bbox.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (3)));
  bbox.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (4)));
  bbox.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (5)));
  bbox.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (6)));
  bbox.AddBoundingVertexSmart (trans.This2Other (box.GetCorner (7)));
}

void csDynaVis::RegisterVisObject (iVisibilityObject* visobj)
{
  csVisibilityObjectWrapper* visobj_wrap = new csVisibilityObjectWrapper ();
  visobj_wrap->visobj = visobj;
  visobj->IncRef ();
  iMovable* movable = visobj->GetMovable ();
  visobj_wrap->update_number = movable->GetUpdateNumber ();
  visobj_wrap->shape_number = visobj->GetShapeNumber ();

  csBox3 bbox;
  CalculateVisObjBBox (visobj, bbox);
  visobj_wrap->child = kdtree->AddObject (bbox, (void*)visobj_wrap);

  visobj_vector.Push (visobj_wrap);
}

void csDynaVis::UnregisterVisObject (iVisibilityObject* visobj)
{
  int i;
  for (i = 0 ; i < visobj_vector.Length () ; i++)
  {
    csVisibilityObjectWrapper* visobj_wrap = (csVisibilityObjectWrapper*)
      visobj_vector[i];
    if (visobj_wrap->visobj == visobj)
    {
      kdtree->RemoveObject (visobj_wrap->child);
      visobj->DecRef ();
      delete visobj_wrap;
      visobj_vector.Delete (i);
      return;
    }
  }
}

void csDynaVis::UpdateObjects ()
{
  int i;
  for (i = 0 ; i < visobj_vector.Length () ; i++)
  {
    csVisibilityObjectWrapper* visobj_wrap = (csVisibilityObjectWrapper*)
      visobj_vector[i];
    iVisibilityObject* visobj = visobj_wrap->visobj;
    iMovable* movable = visobj->GetMovable ();
    if (visobj->GetShapeNumber () != visobj_wrap->shape_number ||
    	movable->GetUpdateNumber () != visobj_wrap->update_number)
    {
      csBox3 bbox;
      CalculateVisObjBBox (visobj, bbox);
      kdtree->MoveObject (visobj_wrap->child, bbox);
      visobj_wrap->shape_number = visobj->GetShapeNumber ();
      visobj_wrap->update_number = movable->GetUpdateNumber ();
    }
    visobj->MarkInvisible ();
    visobj_wrap->reason = INVISIBLE_PARENT;
  }
}

static void Perspective (const csVector3& v, csVector2& p, float fov,
    	float sx, float sy)
{
  float iz = fov / v.z;
  p.x = v.x * iz + sx;
  p.y = v.y * iz + sy;
}

void csDynaVis::ProjectBBox (iCamera* camera, const csBox3& bbox, csBox2& sbox)
{
  // @@@ Can this be done in a smarter way?

  const csReversibleTransform& trans = camera->GetTransform ();
  float fov = camera->GetFOV ();
  float sx = camera->GetShiftX ();
  float sy = camera->GetShiftY ();
  csVector2 oneCorner;

  csBox3 cbox;
  cbox.StartBoundingBox (trans * bbox.GetCorner (0));
  cbox.AddBoundingVertexSmart (trans * bbox.GetCorner (1));
  cbox.AddBoundingVertexSmart (trans * bbox.GetCorner (2));
  cbox.AddBoundingVertexSmart (trans * bbox.GetCorner (3));
  cbox.AddBoundingVertexSmart (trans * bbox.GetCorner (4));
  cbox.AddBoundingVertexSmart (trans * bbox.GetCorner (5));
  cbox.AddBoundingVertexSmart (trans * bbox.GetCorner (6));
  cbox.AddBoundingVertexSmart (trans * bbox.GetCorner (7));

  Perspective (cbox.Max (), oneCorner, fov, sx, sy);
  sbox.StartBoundingBox (oneCorner);
  csVector3 v (cbox.MinX (), cbox.MinY (), cbox.MaxZ ());
  Perspective (v, oneCorner, fov, sx, sy);
  sbox.AddBoundingVertexSmart (oneCorner);
  Perspective (cbox.Min (), oneCorner, fov, sx, sy);
  sbox.AddBoundingVertexSmart (oneCorner);
  v.Set (cbox.MaxX (), cbox.MaxY (), cbox.MinZ ());
  Perspective (v, oneCorner, fov, sx, sy);
  sbox.AddBoundingVertexSmart (oneCorner);
}

bool csDynaVis::TestNodeVisibility (csKDTree* treenode, iRenderView* rview,
  	const csVector3& pos)
{
  const csBox3& node_bbox = treenode->GetNodeBBox ();
  if (node_bbox.Contains (pos)) return true;

  if (node_bbox.Contains (pos))
  {
    return true;
  }

  // First clip to frustum.
  bool l = planeL.Classify (node_bbox.GetCorner (0)) >= 0 ||
	   planeL.Classify (node_bbox.GetCorner (1)) >= 0 ||
	   planeL.Classify (node_bbox.GetCorner (2)) >= 0 ||
	   planeL.Classify (node_bbox.GetCorner (3)) >= 0 ||
	   planeL.Classify (node_bbox.GetCorner (4)) >= 0 ||
	   planeL.Classify (node_bbox.GetCorner (5)) >= 0 ||
	   planeL.Classify (node_bbox.GetCorner (6)) >= 0 ||
	   planeL.Classify (node_bbox.GetCorner (7)) >= 0;
  if (!l) return false;

  bool r = planeR.Classify (node_bbox.GetCorner (0)) >= 0 ||
	   planeR.Classify (node_bbox.GetCorner (1)) >= 0 ||
	   planeR.Classify (node_bbox.GetCorner (2)) >= 0 ||
	   planeR.Classify (node_bbox.GetCorner (3)) >= 0 ||
	   planeR.Classify (node_bbox.GetCorner (4)) >= 0 ||
	   planeR.Classify (node_bbox.GetCorner (5)) >= 0 ||
	   planeR.Classify (node_bbox.GetCorner (6)) >= 0 ||
	   planeR.Classify (node_bbox.GetCorner (7)) >= 0;
  if (!r) return false;

  bool u = planeU.Classify (node_bbox.GetCorner (0)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (1)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (2)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (3)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (4)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (5)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (6)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (7)) >= 0;
  if (!u) return false;

  bool d = planeU.Classify (node_bbox.GetCorner (0)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (1)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (2)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (3)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (4)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (5)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (6)) >= 0 ||
	   planeU.Classify (node_bbox.GetCorner (7)) >= 0;
  if (!d) return false;

  return true;
}

struct VisTest_Front2BackData
{
  csVector3 pos;
  iRenderView* rview;
  csDynaVis* dynavis;
};

static bool VisTest_Front2Back (csKDTree* treenode, void* userdata,
	uint32 cur_timestamp)
{
  VisTest_Front2BackData* data = (VisTest_Front2BackData*)userdata;
  csDynaVis* dynavis = data->dynavis;

  // In the first part of this test we are going to test if the node
  // itself is visible. If it is not then we don't need to continue.
  if (!dynavis->TestNodeVisibility (treenode, data->rview, data->pos))
    return false;

  treenode->Distribute ();

  int num_objects = treenode->GetObjectCount ();
  csKDTreeChild** objects = treenode->GetObjects ();
  int i;
  for (i = 0 ; i < num_objects ; i++)
  {
    if (objects[i]->timestamp != cur_timestamp)
    {
      objects[i]->timestamp = cur_timestamp;
      csVisibilityObjectWrapper* visobj_wrap = (csVisibilityObjectWrapper*)
      	objects[i]->GetObject ();
      visobj_wrap->visobj->MarkVisible ();
      visobj_wrap->reason = VISIBLE;
    }
  }

  return true;
}

bool csDynaVis::VisTest (iRenderView* rview)
{
  debug_camera = rview->GetOriginalCamera ();

  // Initialize the coverage buffer to all empty.
  covbuf->Initialize ();

  // Update all objects (mark them invisible and update in kdtree if needed).
  UpdateObjects ();

  // First get the current view frustum from the rview.
  float lx, rx, ty, by;
  rview->GetFrustum (lx, rx, ty, by);
  csVector3 p0 (lx, by, 1);
  csVector3 p1 (lx, ty, 1);
  csVector3 p2 (rx, ty, 1);
  csVector3 p3 (rx, by, 1);
  const csReversibleTransform& trans = debug_camera->GetTransform ();
  csVector3 origin = trans.This2Other (csVector3 (0));
  p0 = trans.This2Other (p0);
  p1 = trans.This2Other (p1);
  p2 = trans.This2Other (p2);
  p3 = trans.This2Other (p3);
  planeL.Set (origin, p0, p1);
  planeU.Set (origin, p1, p2);
  planeR.Set (origin, p2, p3);
  planeD.Set (origin, p3, p0);

  // The big routine: traverse from front to back and mark all objects
  // visible that are visible. In the mean time also update the coverage
  // buffer for further culling.
  VisTest_Front2BackData data;
  data.pos = rview->GetCamera ()->GetTransform ().GetOrigin ();
  data.rview = rview;
  data.dynavis = this;
  kdtree->Front2Back (data.pos, VisTest_Front2Back, (void*)&data);

  return true;
}

iString* csDynaVis::Debug_UnitTest ()
{
  csKDTree* kdtree = new csKDTree (NULL);
  iDebugHelper* dbghelp = SCF_QUERY_INTERFACE (kdtree, iDebugHelper);
  if (dbghelp)
  {
    iString* rc = dbghelp->UnitTest ();
    dbghelp->DecRef ();
    if (rc)
    {
      delete kdtree;
      return rc;
    }
  }
  delete kdtree;

  csCoverageBuffer* covbuf = new csCoverageBuffer (640, 480);
  dbghelp = SCF_QUERY_INTERFACE (covbuf, iDebugHelper);
  if (dbghelp)
  {
    iString* rc = dbghelp->UnitTest ();
    dbghelp->DecRef ();
    if (rc)
    {
      delete covbuf;
      return rc;
    }
  }
  delete covbuf;

  return NULL;
}

iString* csDynaVis::Debug_StateTest ()
{
  return NULL;
}

iString* csDynaVis::Debug_Dump ()
{
  return NULL;
}

void csDynaVis::Debug_Dump (iGraphics3D* g3d)
{
  struct color { int r, g, b; };
  static color reason_colors[] =
  {
    { 64, 64, 64 },
    { 255, 255, 255 }
  };

  if (debug_camera)
  {
    iGraphics2D* g2d = g3d->GetDriver2D ();
    iTextureManager* txtmgr = g3d->GetTextureManager ();
    g3d->BeginDraw (CSDRAW_2DGRAPHICS);
    int col_cam = txtmgr->FindRGB (0, 255, 0);
    int i;
    int reason_cols[LAST_REASON];
    for (i = 0 ; i < LAST_REASON ; i++)
    {
      reason_cols[i] = txtmgr->FindRGB (reason_colors[i].r,
      	reason_colors[i].g, reason_colors[i].b);
    }

    csReversibleTransform trans = debug_camera->GetTransform ();
    trans = csTransform (csYRotMatrix3 (-PI/2.0), csVector3 (0)) * trans;
    float fov = g3d->GetPerspectiveAspect ();
    int sx, sy;
    g3d->GetPerspectiveCenter (sx, sy);

    csVector3 view_origin;
    // This is the z at which we want to view the origin.
    view_origin.z = 50;
    // The x,y values are then calculated with inverse perspective
    // projection given that we want the view origin to be visualized
    // at view_persp_x and view_persp_y.
    float view_persp_x = sx;
    float view_persp_y = sy;
    view_origin.x = (view_persp_x-sx) * view_origin.z / fov;
    view_origin.y = (view_persp_y-sy) * view_origin.z / fov;
    trans.SetOrigin (trans.This2Other (-view_origin));

    csVector3 trans_origin = trans.Other2This (
    	debug_camera->GetTransform ().GetOrigin ());
    csVector2 to;
    Perspective (trans_origin, to, fov, sx, sy);
    g2d->DrawLine (to.x-3,  to.y-3, to.x+3,  to.y+3, col_cam);
    g2d->DrawLine (to.x+3,  to.y-3, to.x-3,  to.y+3, col_cam);
    g2d->DrawLine (to.x,    to.y,   to.x+30, to.y,   col_cam);
    g2d->DrawLine (to.x+30, to.y,   to.x+24, to.y-4, col_cam);
    g2d->DrawLine (to.x+30, to.y,   to.x+24, to.y+4, col_cam);

    for (i = 0 ; i < visobj_vector.Length () ; i++)
    {
      csVisibilityObjectWrapper* visobj_wrap = (csVisibilityObjectWrapper*)
    	visobj_vector[i];
      int col = reason_cols[visobj_wrap->reason];
      const csBox3& b = visobj_wrap->child->GetBBox ();
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xyz)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_Xyz)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xyz)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xYz)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xyz)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xyZ)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_XYZ)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xYZ)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_XYZ)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_XyZ)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_XYZ)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_XYz)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_Xyz)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_XYz)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_Xyz)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_XyZ)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xYz)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xYZ)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xYz)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_XYz)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xyZ)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_XyZ)), fov, col);
      g3d->DrawLine (
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xyZ)),
      	trans.Other2This (b.GetCorner (CS_BOX_CORNER_xYZ)), fov, col);
    }
  }
}

csTicks csDynaVis::Debug_Benchmark (int num_iterations)
{
  csTicks rc = 0;

  csKDTree* kdtree = new csKDTree (NULL);
  iDebugHelper* dbghelp = SCF_QUERY_INTERFACE (kdtree, iDebugHelper);
  if (dbghelp)
  {
    csTicks r = dbghelp->Benchmark (num_iterations);
    printf ("kdtree:   %d ms\n", r);
    rc += r;
    dbghelp->DecRef ();
  }
  delete kdtree;

  csCoverageBuffer* covbuf = new csCoverageBuffer (640, 480);
  dbghelp = SCF_QUERY_INTERFACE (covbuf, iDebugHelper);
  if (dbghelp)
  {
    csTicks r = dbghelp->Benchmark (num_iterations);
    printf ("covbuf:   %d ms\n", r);
    rc += r;
    dbghelp->DecRef ();
  }
  delete covbuf;

  return rc;
}

