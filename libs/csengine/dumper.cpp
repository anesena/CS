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

#include "cssysdef.h"
#include "csutil/bitset.h"
#include "csgeom/math3d.h"
#include "csgeom/math2d.h"
#include "csgeom/polyclip.h"
#include "csgeom/polypool.h"
#include "csgeom/frustum.h"
#include "csengine/dumper.h"
#include "csengine/bsp.h"
#include "csengine/bsp2d.h"
#include "csengine/octree.h"
#include "csengine/quadtree.h"
#include "csengine/quadcube.h"
#include "csengine/lppool.h"
#include "csengine/camera.h"
#include "csengine/polytmap.h"
#include "csengine/polygon.h"
#include "csengine/pol2d.h"
#include "csengine/polytext.h"
#include "csengine/polyset.h"
#include "csengine/triangle.h"
#include "csengine/thing.h"
#include "csengine/sector.h"
#include "csengine/world.h"
#include "csengine/cssprite.h"
#include "csengine/light.h"
#include "csengine/lghtmap.h"

void Dumper::dump (csMatrix3* m, char* name)
{
  CsPrintf (MSG_DEBUG_0, "Matrix '%s':\n", name);
  CsPrintf (MSG_DEBUG_0, "/\n");
  CsPrintf (MSG_DEBUG_0, "| %3.2f %3.2f %3.2f\n", m->m11, m->m12, m->m13);
  CsPrintf (MSG_DEBUG_0, "| %3.2f %3.2f %3.2f\n", m->m21, m->m22, m->m23);
  CsPrintf (MSG_DEBUG_0, "| %3.2f %3.2f %3.2f\n", m->m31, m->m32, m->m33);
  CsPrintf (MSG_DEBUG_0, "\\\n");
}

void Dumper::dump (csVector3* v, char* name)
{
  CsPrintf (MSG_DEBUG_0, "Vector '%s': (%f,%f,%f)\n", name, v->x, v->y, v->z);
}

void Dumper::dump (csVector2* v, char* name)
{
  CsPrintf (MSG_DEBUG_0, "Vector '%s': (%f,%f)\n", name, v->x, v->y);
}

void Dumper::dump (csPlane3* p)
{
  CsPrintf (MSG_DEBUG_0, "A=%2.2f B=%2.2f C=%2.2f D=%2.2f\n",
            p->norm.x, p->norm.y, p->norm.z, p->DD);
}

void Dumper::dump (csBox2* b)
{
  CsPrintf (MSG_DEBUG_0, "(%2.2f,%2.2f)-(%2.2f,%2.2f)",
  	b->MinX (), b->MinY (), b->MaxX (), b->MaxY ());
}

void Dumper::dump (csCamera* c)
{
  CsPrintf (MSG_DEBUG_0, "csSector: %s\n", 
    c->sector ? c->sector->GetName () : "(?)");
  dump (&c->v_o2t, "Camera vector");
  dump (&c->m_o2t, "Camera matrix");
}

void Dumper::dump (csPolyTxtPlane* p)
{
  const char* pl_name = p->GetName ();
  CsPrintf (MSG_DEBUG_0, "PolyTxtPlane '%s' id=%ld:\n", pl_name ? pl_name : "(no name)",
            p->GetID ());
  dump (&p->m_obj2tex, "Mot");
  dump (&p->v_obj2tex, "Vot");
  dump (&p->m_world2tex, "Mwt");
  dump (&p->v_world2tex, "Vwt");
}

void Dumper::dump (csPolygon3D* p)
{
  csPolygonSet* ps = (csPolygonSet*)p->GetParent ();
  CsPrintf (MSG_DEBUG_0, "Dump polygon '%s/%s' id=%ld:\n", ps->GetName (),
  	p->GetName (), p->GetID ());
  if (p->GetUnsplitPolygon ())
  {
    CsPrintf (MSG_DEBUG_0, "    Split from polygon with id=%ld\n",
    	((csPolygon3D*)(p->GetUnsplitPolygon ()))->GetID ());
  }
  CsPrintf (MSG_DEBUG_0, "    num_vertices=%d  max_vertices=%d\n",
  	p->GetVertices ().GetNumVertices (), p->GetVertices ().max_vertices);
  if (p->portal)
  {
    csPortal* portal = p->portal;
    CsPrintf (MSG_DEBUG_0, "    Polygon is a CS portal to sector '%s'.\n", 
      portal->GetSector()->GetName ());
    CsPrintf (MSG_DEBUG_0, "    Alpha=%d\n", p->GetAlpha ());
  }
  if (p->poly_set->cam_verts == NULL)
  {
    int i;
    for (i = 0 ; i < p->GetVertices ().GetNumVertices () ; i++)
      CsPrintf (MSG_DEBUG_0, "        v[%d]: (%f,%f,%f)\n",
    	i, p->Vwor (i).x, p->Vwor (i).y, p->Vwor (i).z);
  }
  else
  {
    int i;
    for (i = 0 ; i < p->GetVertices ().GetNumVertices () ; i++)
      CsPrintf (MSG_DEBUG_0, "        v[%d]: (%f,%f,%f)  camera:(%f,%f,%f)\n",
    	i,
    	p->Vwor (i).x, p->Vwor (i).y, p->Vwor (i).z,
    	p->Vcam (i).x, p->Vcam (i).y, p->Vcam (i).z);
  }
  if (p->GetTextureType () == POLYTXT_LIGHTMAP)
  {
    csLightMapped* lmi = p->GetLightMapInfo ();
    dump (lmi->GetTxtPlane ());
    if (lmi->GetPolyTex ()) dump (lmi->GetPolyTex (), "PolyTexture");
  }

  csLightPatch* lp = p->light_info.lightpatches;
  while (lp)
  {
    CsPrintf (MSG_DEBUG_0, "  LightPatch (num_vertices=%d, light=%08lx)\n", lp->num_vertices,
    	lp->GetLight ());
    lp = lp->GetNextPoly ();
  }
}

void Dumper::dump (csPolygonSet* p)
{
  CsPrintf (MSG_DEBUG_0, "========================================================\n");
  CsPrintf (MSG_DEBUG_0, "Dump sector '%s' id=%ld:\n", 
    p->GetName (), p->GetID ());
  CsPrintf (MSG_DEBUG_0, "    num_vertices=%d max_vertices=%d polygons=%d\n",
    p->num_vertices, p->max_vertices, p->GetNumPolygons ());
  int i;

  CsPrintf (MSG_DEBUG_0, "  Vertices:\n");
  for (i = 0 ; i < p->num_vertices ; i++)
  {
    CsPrintf (MSG_DEBUG_0, "    Vertex[%d]=\n", i);
    dump (&p->wor_verts[i], "world");
    dump (&p->obj_verts[i], "object");
    //dump (&p->cam_verts[i], "camera");
    CsPrintf (MSG_DEBUG_0, "\n");
  }
  CsPrintf (MSG_DEBUG_0, "  Polygons:\n");
  for (i = 0 ; i < p->GetNumPolygons () ; i++)
  {
    CsPrintf (MSG_DEBUG_0, "------------------------------------------\n");
    csPolygon3D* pp = p->GetPolygon3D (i);
    dump (pp);
  }
}

void Dumper::dump (csSector* s)
{
  dump ((csPolygonSet*)s);
  int i;
  for (i = 0 ; i < s->sprites.Length () ; i++)
  {
    csSprite* sp = (csSprite*)s->sprites[i];
    if (sp->GetType () == csSprite3D::Type)
    {
      csSprite3D* sp3d = (csSprite3D*)sp;
      dump (sp3d);
    }
  }
  csThing* th = s->first_thing;
  while (th)
  {
    dump ((csPolygonSet*)th);
    th = (csThing*)(th->next);
  }
}

void Dumper::dump (csWorld* w)
{
  int sn = w->sectors.Length ();
  while (sn > 0)
  {
    sn--;
    csSector* s = (csSector*)w->sectors[sn];
    dump (s);
  }
}

void Dumper::dump (csSpriteTemplate* s)
{
  CsPrintf (MSG_DEBUG_0, "Dump sprite template '%s' id=%ld:\n", 
    s->GetName (), s->GetID ());
  CsPrintf (MSG_DEBUG_0, "%d vertices.\n", s->GetNumTexels());
  CsPrintf (MSG_DEBUG_0, "%d triangles.\n", s->GetTexelMesh ()->GetNumTriangles ());
  CsPrintf (MSG_DEBUG_0, "%d frames.\n", s->GetNumFrames ());
  CsPrintf (MSG_DEBUG_0, "%d actions.\n", s->GetNumActions ());
}
 
void Dumper::dump (csSprite3D* s)
{
  CsPrintf (MSG_DEBUG_0, "Dump sprite '%s' id=%ld:\n", 
    s->GetName (), s->GetID ());
  dump (s->tpl);
  dump (&s->m_obj2world, "Object->world");
  dump (&s->v_obj2world, "Object->world");
#if 0
  int i;
  CsPrintf (MSG_DEBUG_0, "Last transformed frame:\n");
  for (i = 0 ; i < s->tpl->num_vertices ; i++)
    CsPrintf (MSG_DEBUG_0, "  V%d: tex:(%f,%f) cam:(%f,%f,%f) scr:(%f,%f) vis:%d\n",
    	i, s->tr_frame->GetTexel (i).x, s->tr_frame->GetTexel (i).y,
    	s->tr_frame->GetVertex (i).x, s->tr_frame->GetVertex (i).y, s->tr_frame->GetVertex (i).z,
	s->persp[i].x, s->persp[i].y, s->visible[i]);
#endif
}

void Dumper::dump (csPolyTexture* p, char* name)
{
  CsPrintf (MSG_DEBUG_0, "  PolyTexture '%s'\n", name);
  CsPrintf (MSG_DEBUG_0, "    Imin_u=%d Imin_v=%d Imax_u=%d Imax_v=%d\n", p->Imin_u, p->Imin_v, p->Imax_u, p->Imax_v);
  CsPrintf (MSG_DEBUG_0, "    Fmin_u=%f Fmin_v=%f Fmax_u=%f Fmax_v=%f\n", p->Fmin_u, p->Fmin_v, p->Fmax_u, p->Fmax_v);
  if (p->lm)
  {
    CsPrintf (MSG_DEBUG_0, "    lm: lwidth=%d lheight=%d rwidth=%d rheight=%d lm_size=%d\n", p->lm->lwidth,
  	p->lm->lheight, p->lm->rwidth, p->lm->rheight, p->lm->lm_size);
    CsPrintf (MSG_DEBUG_0, "    lm: lm_size=%ld\n",
    	p->lm->lm_size);
  }
  else
    CsPrintf (MSG_DEBUG_0, "    lm: no lightmap\n");
  CsPrintf (MSG_DEBUG_0, "    shf_u=%d and_u=%d\n", p->shf_u, p->and_u);
  CsPrintf (MSG_DEBUG_0, "    fdu=%f fdv=%f\n", p->fdu, p->fdv);
  CsPrintf (MSG_DEBUG_0, "    w=%d h=%d w_orig=%d\n", p->w, p->h, p->w_orig);
}

void Dumper::dump (csPolygon2D* p, char* name)
{
  CsPrintf (MSG_DEBUG_0, "Dump polygon 2D '%s':\n", name);
  CsPrintf (MSG_DEBUG_0, "    num_vertices=%d  max_vertices=%d\n", p->num_vertices, p->max_vertices);
  int i;
  for (i = 0 ; i < p->num_vertices ; i++)
    CsPrintf (MSG_DEBUG_0, "        v[%d]: (%f,%f)\n", i, p->vertices[i].x, p->vertices[i].y);
}

void Dumper::dump (csBspTree* tree, csBspNode* node, int indent)
{
  if (!node) return;
  int i;
  char spaces[256];
  for (i = 0 ; i < indent ; i++) spaces[i] = ' ';
  spaces[indent] = 0;
  CsPrintf (MSG_DEBUG_0, "%sThere are %d polygons in this node.\n", spaces, node->polygons.GetNumPolygons ());
  CsPrintf (MSG_DEBUG_0, "%s Front:\n", spaces);
  dump (tree, node->front, indent+2);
  CsPrintf (MSG_DEBUG_0, "%s Back:\n", spaces);
  dump (tree, node->back, indent+2);
}

void Dumper::dump (csBspTree2D* tree, csBspNode2D* node, int indent)
{
  if (!node) return;
  int i;
  char spaces[256];
  for (i = 0 ; i < indent ; i++) spaces[i] = ' ';
  spaces[indent] = 0;
  CsPrintf (MSG_DEBUG_0, "%sThere are %d segments in this node.\n", spaces,
    node->segments.Length ());
  for (i = 0 ; i < node->segments.Length () ; i++)
  {
    csSegment2* s = node->segments.Get (i);
    CsPrintf (MSG_DEBUG_0, "%s%d: (%f,%f)-(%f,%f)\n", spaces,
      i, s->Start ().x, s->Start ().y, s->End ().x, s->End ().y);
  }
  CsPrintf (MSG_DEBUG_0, "%s Front:\n", spaces);
  dump (tree, node->front, indent+2);
  CsPrintf (MSG_DEBUG_0, "%s Back:\n", spaces);
  dump (tree, node->back, indent+2);
}

void Dumper::dump (csBspTree* tree)
{
  dump (tree, (csBspNode*)(tree->root), 0);
}

void Dumper::dump (csBspTree2D* tree)
{
  dump (tree, tree->root, 0);
}

void Dumper::dump (csPolygonClipper* clipper, char* name)
{
  CsPrintf (MSG_DEBUG_0, "PolygonClipper '%s'\n", name);
  int i;
  for (i = 0 ; i < clipper->ClipPolyVertices ; i++)
    CsPrintf (MSG_DEBUG_0, "  %d: (%f,%f)\n", i, clipper->ClipPoly[i].x, clipper->ClipPoly[i].y);
}

void Dumper::dump (csFrustum* frustum, char* name)
{
  CsPrintf (MSG_DEBUG_0, "csFrustum '%s'\n", name);
  if (!frustum)
  {
    CsPrintf (MSG_DEBUG_0, "  NULL\n");
    return;
  }
  if (frustum->IsEmpty ())
  {
    CsPrintf (MSG_DEBUG_0, "  EMPTY\n");
    return;
  }
  if (frustum->IsInfinite ())
  {
    CsPrintf (MSG_DEBUG_0, "  INFINITE\n");
    return;
  }
  int i;
  CsPrintf (MSG_DEBUG_0, "  "); dump (&frustum->GetOrigin (), "origin");
  for (i = 0 ; i < frustum->GetNumVertices () ; i++)
  {
    CsPrintf (MSG_DEBUG_0, "  ");
    char buf[20];
    sprintf (buf, "[%d]", i);
    dump (&frustum->GetVertex (i), buf);
  }
  if (frustum->GetBackPlane ())
  {
    CsPrintf (MSG_DEBUG_0, "  ");
    dump (frustum->GetBackPlane ());
  }
}

void Dumper::dump (csPoly2DPool* pool, char* name)
{
  int cnt;
  ULong tot_size;
  int max_size;

  cnt = 0;
  tot_size = 0;
  max_size = 0;
  csPoly2DPool::PoolObj* po = pool->alloced;
  while (po)
  {
    tot_size += po->pol2d->max_vertices;
    if (po->pol2d->max_vertices > max_size) max_size = po->pol2d->max_vertices;
    cnt++;
    po = po->next;
  }
  CsPrintf (MSG_DEBUG_0, "Poly2D pool (%s): %d allocated (max=%d, tot=%ld), ",
  	name, cnt, max_size, tot_size);
  cnt = 0;
  tot_size = 0;
  max_size = 0;
  po = pool->freed;
  while (po)
  {
    tot_size += po->pol2d->max_vertices;
    if (po->pol2d->max_vertices > max_size) max_size = po->pol2d->max_vertices;
    cnt++;
    po = po->next;
  }
  CsPrintf (MSG_DEBUG_0, "%d freed (max=%d, tot=%ld).\n",
  	cnt, max_size, tot_size);
}

void Dumper::dump (csLightPatchPool* pool, char* name)
{
  int cnt;
  cnt = 0;
  csLightPatchPool::PoolObj* po = pool->alloced;
  while (po) { cnt++; po = po->next; }
  CsPrintf (MSG_DEBUG_0, "LightPatch pool (%s): %d allocated, ", name, cnt);
  cnt = 0;
  po = pool->freed;
  while (po) { cnt++; po = po->next; }
  CsPrintf (MSG_DEBUG_0, "%d freed.\n", cnt);
}

void Dumper::dump (csQuadtreeNode* node, char* buf, int bufdim, int depth,
  	int x1, int y1, int x2, int y2)
{
  if (node->GetState () == CS_QUAD_PARTIAL && node->children)
  {
    int cx = x1+(x2-x1+1)/2;
    int cy = y1+(y2-y1+1)/2;
    dump (&node->children[0], buf, bufdim, depth-1, x1, y1, cx-1, cy-1);
    dump (&node->children[1], buf, bufdim, depth-1, cx, y1, x2, cy-1);
    dump (&node->children[2], buf, bufdim, depth-1, cx, cy, x2, y2);
    dump (&node->children[3], buf, bufdim, depth-1, x1, cy, cx-1, y2);
  }
  else
  {
    char fchar;
    if (node->GetState () == CS_QUAD_EMPTY) fchar = '.';
    else if (node->GetState () == CS_QUAD_FULL) fchar = '#';
    else fchar = 'x';
    int i, j;
    for (i = x1 ; i <= x2 ; i++)
      for (j = y1 ; j <= y2 ; j++)
        buf[j*bufdim+i] = fchar;
  }
}

void Dumper::dump (csQuadcube* cube)
{
  CsPrintf (MSG_DEBUG_0, "---------------------------------------\n");
  CsPrintf (MSG_DEBUG_0, "Quadtree 0:\n"); dump (cube->trees[0]);
  CsPrintf (MSG_DEBUG_0, "Quadtree 1:\n"); dump (cube->trees[1]);
  CsPrintf (MSG_DEBUG_0, "Quadtree 2:\n"); dump (cube->trees[2]);
  CsPrintf (MSG_DEBUG_0, "Quadtree 3:\n"); dump (cube->trees[3]);
  CsPrintf (MSG_DEBUG_0, "Quadtree 4:\n"); dump (cube->trees[4]);
  CsPrintf (MSG_DEBUG_0, "Quadtree 5:\n"); dump (cube->trees[5]);
  CsPrintf (MSG_DEBUG_0, "---------------------------------------\n");
}

void Dumper::dump (csQuadtree* tree)
{
  // First calculate depth of tree.
  int depth = 0;
  csQuadtreeNode* n = tree->root;
  while (n)
  {
    depth++;
    n = n->children;
  }

  // Calculate 2^depth.
  int depthp = 1;
  int depth2 = depth-1;
  while (depth2 > 0)
  {
    depthp <<= 1;
    depth2--;
  }

  char* buf;
  buf = new char [depthp*depthp];

  CsPrintf (MSG_DEBUG_0, "Depth=%d depthp=%d\n", depth, depthp);
  memset (buf, '?', depthp*depthp);
  dump (tree->root, buf, depthp, depth-1,
  	0, 0, depthp-1, depthp-1);
  char buf2[255];
  int y;
  for (y = 0 ; y < depthp ; y++)
  {
    memcpy (buf2, &buf[y*depthp], depthp);
    buf2[depthp] = 0;
    CsPrintf (MSG_DEBUG_0, "%s\n", buf2);
  }

  delete [] buf;
}

char* spaces (int nr)
{
  static char spc[200];
  spc[0] = 0;
  int i;
  for (i = 0 ; i < nr ; i++) spc[i] = ' ';
  spc[nr] = 0;
  return spc;
}

bool Dumper::check_stubs (csBspNode* node)
{
  if (!node) return false;
  if (node->first_stub || node->todo_stubs) return true;
  if (check_stubs (node->front)) return true;
  if (check_stubs (node->back)) return true;
  return false;
}

bool Dumper::check_stubs (csOctreeNode* node)
{
  if (!node) return false;
  if (node->first_stub || node->todo_stubs) return true;
  if (check_stubs ((csOctreeNode*)node->children[0])) return true;
  if (check_stubs ((csOctreeNode*)node->children[1])) return true;
  if (check_stubs ((csOctreeNode*)node->children[2])) return true;
  if (check_stubs ((csOctreeNode*)node->children[3])) return true;
  if (check_stubs ((csOctreeNode*)node->children[4])) return true;
  if (check_stubs ((csOctreeNode*)node->children[5])) return true;
  if (check_stubs ((csOctreeNode*)node->children[6])) return true;
  if (check_stubs ((csOctreeNode*)node->children[7])) return true;
  if (node->minibsp && check_stubs ((csBspNode*)node->minibsp->root)) return true;
  return false;
}

void Dumper::dump_stubs_node (csPolygonStub* stub, char* name, int level)
{
  while (stub)
  {
    CsPrintf (MSG_DEBUG_0, "%s %s num_poly=%d this=%08lx obj=%08lx node=%08lx\n",
    	spaces (level), name, stub->GetNumPolygons (),
    	stub, stub->object, stub->node);
    if (stub->next_tree && stub->next_tree->prev_tree != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! next_tree link broken !!!\n", spaces (level));
    if (stub->prev_tree && stub->prev_tree->next_tree != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! prev_tree link broken !!!\n", spaces (level));
    if (!stub->prev_tree && stub->node->first_stub != stub &&
    	stub->node->todo_stubs != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! stub should be first in node but it isn't !!!\n", spaces (level));
    if (stub->prev_tree && (stub->node->first_stub == stub ||
    	stub->node->todo_stubs == stub))
      CsPrintf (MSG_DEBUG_0, "%s !!! stub should not be first in node but it is !!!\n", spaces (level));
    if (stub->next_obj && stub->next_obj->prev_obj != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! next_obj link broken !!!\n", spaces (level));
    if (stub->prev_obj && stub->prev_obj->next_obj != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! prev_obj link broken !!!\n", spaces (level));
    if (!stub->prev_obj && stub->object->first_stub != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! stub should be first in object but it isn't !!!\n", spaces (level));
    if (stub->prev_obj && stub->object->first_stub == stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! stub should not be first in object but it is !!!\n", spaces (level));
    stub = stub->next_tree;
  }
}

void Dumper::dump_stubs_obj (csPolygonStub* stub, char* name, int level)
{
  while (stub)
  {
    CsPrintf (MSG_DEBUG_0, "%s %s num_poly=%d this=%08lx obj=%08lx node=%08lx\n",
    	spaces (level), name, stub->GetNumPolygons (),
    	stub, stub->object, stub->node);
    if (stub->next_tree && stub->next_tree->prev_tree != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! next_tree link broken !!!\n", spaces (level));
    if (stub->prev_tree && stub->prev_tree->next_tree != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! prev_tree link broken !!!\n", spaces (level));
    if (!stub->prev_tree && stub->node->first_stub != stub &&
    	stub->node->todo_stubs != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! stub should be first in node but it isn't !!!\n", spaces (level));
    if (stub->prev_tree && (stub->node->first_stub == stub ||
    	stub->node->todo_stubs == stub))
      CsPrintf (MSG_DEBUG_0, "%s !!! stub should not be first in node but it is !!!\n", spaces (level));
    if (stub->next_obj && stub->next_obj->prev_obj != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! next_obj link broken !!!\n", spaces (level));
    if (stub->prev_obj && stub->prev_obj->next_obj != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! prev_obj link broken !!!\n", spaces (level));
    if (!stub->prev_obj && stub->object->first_stub != stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! stub should be first in object but it isn't !!!\n", spaces (level));
    if (stub->prev_obj && stub->object->first_stub == stub)
      CsPrintf (MSG_DEBUG_0, "%s !!! stub should not be first in object but it is !!!\n", spaces (level));
    stub = stub->next_obj;
  }
}

void Dumper::dump_stubs (csBspNode* bnode, char* name, int level)
{
  if (!bnode) return;
  CsPrintf (MSG_DEBUG_0, "%s bnode(%s)\n", spaces (level), name);
  if (!check_stubs (bnode)) { CsPrintf (MSG_DEBUG_0, "%s  ..\n", spaces (level)); return; }
  dump_stubs_node (bnode->first_stub, "stub", level+1);
  dump_stubs_node (bnode->todo_stubs, "todo", level+1);
  dump_stubs (bnode->front, "front", level+1);
  dump_stubs (bnode->back, "back", level+1);
}
void Dumper::dump_stubs (csOctreeNode* onode, char* name, int level)
{
  if (!onode) return;
  CsPrintf (MSG_DEBUG_0, "%s onode(%s) (%f,%f,%f)\n", spaces (level),
  	name,
  	onode->bbox.MaxX () - onode->bbox.MinX (),
  	onode->bbox.MaxY () - onode->bbox.MinY (),
  	onode->bbox.MaxZ () - onode->bbox.MinZ ());
  if (!check_stubs (onode)) { CsPrintf (MSG_DEBUG_0, "%s  ..\n", spaces (level)); return; }
  dump_stubs_node (onode->first_stub, "stub", level+1);
  dump_stubs_node (onode->todo_stubs, "todo", level+1);
  if (onode->minibsp) dump_stubs ((csBspNode*)(onode->minibsp->root),
  	"root", level+1);
  dump_stubs ((csOctreeNode*)onode->children[0], "0", level+1);
  dump_stubs ((csOctreeNode*)onode->children[1], "1", level+1);
  dump_stubs ((csOctreeNode*)onode->children[2], "2", level+1);
  dump_stubs ((csOctreeNode*)onode->children[3], "3", level+1);
  dump_stubs ((csOctreeNode*)onode->children[4], "4", level+1);
  dump_stubs ((csOctreeNode*)onode->children[5], "5", level+1);
  dump_stubs ((csOctreeNode*)onode->children[6], "6", level+1);
  dump_stubs ((csOctreeNode*)onode->children[7], "7", level+1);
}

void Dumper::dump_stubs (csOctree* octree)
{
  csPolyTreeObject::stub_pool.Dump ();
  CsPrintf (MSG_DEBUG_0, "Dump octree\n");
  dump_stubs ((csOctreeNode*)octree->root, "root", 0);
}

void Dumper::dump_stubs (csPolyTreeObject* ptobj)
{
  CsPrintf (MSG_DEBUG_0, "Dump csPolyTreeObject\n");
  dump_stubs_obj (ptobj->first_stub, "root", 0);
}

