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

#define SYSDEF_ACCESS
#include "sysdef.h"
#include "version.h"
#include "qint.h"
#include "cssys/common/system.h"
#include "support/console.h"
#include "support/command.h"
#include "walktest/walktest.h"
#include "walktest/infmaze.h"
#include "walktest/scon.h"
#include "csparser/csloader.h"
#include "csgeom/csrect.h"
#include "csgeom/frustrum.h"
#include "csengine/dumper.h"
#include "csengine/csview.h"
#include "csengine/stats.h"
#include "csengine/light/light.h"
#include "csengine/light/dynlight.h"
#include "csengine/texture.h"
#include "csengine/objects/thing.h"
#include "csengine/wirefrm.h"
#include "csengine/library.h"
#include "csengine/polygon/polytext.h"
#include "csengine/polygon/pol2d.h"
#include "csscript/csscript.h"
#include "csscript/intscri.h"

// Being is no more
//#include "csengine/colldet/being.h"
#include "csengine/colldet/cdobj.h"
#include "csengine/colldet/collider.h"

#include "csengine/2d/csspr2d.h"
#include "csutil/sparse3d.h"
#include "csutil/inifile.h"
#include "csutil/impexp/impexp.h"
#include "csobject/nameobj.h"
#include "csobject/dataobj.h"
#include "csgfxldr/csimage.h"
#include "cssndldr/common/sndbuf.h"
#include "csparser/sndbufo.h"
#include "igraph3d.h"
#include "itxtmgr.h"
#include "isndrdr.h"

#if defined(OS_DOS) || defined(OS_WIN32) || defined (OS_OS2)
#  include <io.h>
#elif defined(OS_UNIX)
#  include <unistd.h>
#endif

#include "debug/fpu80x86.h"	// for debugging numerical instabilities

WalkTest *Sys;
converter *ImportExport;

#define Gfx3D System->piG3D
#define Gfx2D System->piG2D

//-----------------------------------------------------------------------------

void DrawZbuffer ()
{
  for (int y = 0; y < FRAME_HEIGHT; y++)
  {
    int gi_pixelbytes;
    System->piGI->GetPixelBytes (gi_pixelbytes);

    if (gi_pixelbytes == 4)
    {
      //@@@
    }
    if (gi_pixelbytes == 2)
    {
      UShort *dest;
      Gfx2D->GetPixelAt(0, y, (unsigned char**)&dest);

      ULong *zbuf;
      Gfx3D->GetZBufPoint (0, y, &zbuf);

      for (int x = 0; x < FRAME_WIDTH; x++)
        *dest++ = (unsigned short)(*zbuf++ >> 13);
    }
    else
    {
      unsigned char *dest;
      Gfx2D->GetPixelAt(0, y, &dest);

      ULong *zbuf;
      Gfx3D->GetZBufPoint(0, y, &zbuf);

      for (int x = 0; x < FRAME_WIDTH; x++)
        *dest++ = (unsigned char)(*zbuf++ >> 16);
    }
  }
}

int collcount = 0;

//------------------------------------------------------------------------
// The following series of functions are all special callback functions
// which are called by csWorld::DrawFunc() or csLight::LightingFunc().
//------------------------------------------------------------------------

// Callback for LightingFunc() to show the lighting frustrum for the
// selected light.
void show_frustrum (csLightView* lview, int type, void* /*entity*/)
{
  ITextureManager* txtmgr;
  Gfx3D->GetTextureManager (&txtmgr);
  int red, white;
  txtmgr->FindRGB (255, 255, 255, white);
  txtmgr->FindRGB (255, 0, 0, red);

  if (type == CALLBACK_POLYGON)
  {
    csCamera* cam = Sys->view->GetCamera ();
    csFrustrum* fr = lview->light_frustrum;
    csVector3 v0, v1, v2;
    csVector3 light_cam = cam->Other2This (fr->GetOrigin ());
    int j;

    for (j = 0 ; j < fr->GetNumVertices () ; j++)
    {
      v0 = fr->GetVertices ()[j] + fr->GetOrigin ();
      v1 = cam->Other2This (v0);
      v0 = fr->GetVertices ()[(j+1)%fr->GetNumVertices ()] + fr->GetOrigin ();
      v2 = cam->Other2This (v0);
      Gfx3D->DrawLine (light_cam, v1, csCamera::aspect, red);
      Gfx3D->DrawLine (light_cam, v2, csCamera::aspect, red);
      Gfx3D->DrawLine (v1, v2, csCamera::aspect, white);
    }
  }
}

// Callback for DrawFunc() to select an object with the mouse. The coordinate
// to check for is in 'coord_check_vector'.
bool check_poly;
bool check_light;
csVector2 coord_check_vector;

void select_object (csRenderView* rview, int type, void* entity)
{
  static csPolygon3D* last_poly = NULL;

  if (type == CALLBACK_POLYGON)
  {
    // Here we depend on CALLBACK_POLYGON being called right before CALLBACK_POLYGON2D.
    last_poly = (csPolygon3D*)entity;
  }
  else if (type == CALLBACK_POLYGON2D)
  {
    int i;
    csPolygon2D* polygon = (csPolygon2D*)entity;
    int num = polygon->GetNumVertices ();
    CHK (csPolygon2D* pp = new csPolygon2D (num));
    if (rview->IsMirrored ())
      for (i = 0 ; i < num ; i++)
        pp->AddVertex  (polygon->GetVertices ()[num-i-1]);
    else
      for (i = 0 ; i < num ; i++)
        pp->AddVertex  (polygon->GetVertices ()[i]);
    if (csMath2::InPoly2D (coord_check_vector, pp->GetVertices (),
        pp->GetNumVertices (), &pp->GetBoundingBox ()) != CS_POLY_OUT)
    {
      csPolygonSet* ps = (csPolygonSet*)(last_poly->GetParent ());
      Sys->Printf (MSG_DEBUG_0, "Hit polygon '%s/%s'\n",
        csNameObject::GetName(*ps), csNameObject::GetName(*last_poly));
      Dumper::dump (polygon, "csPolygon2D");
      Dumper::dump (last_poly);
      if (check_poly && !last_poly->GetPortal ())
      {
        if (Sys->selected_polygon == last_poly) Sys->selected_polygon = NULL;
        else Sys->selected_polygon = last_poly;
	//check_poly = false;
      }
    }

    CHK (delete pp);
  }
  else if (type == CALLBACK_SECTOR)
  {
    csSector* sector = (csSector*)entity;
    int i;
    csVector3 v;
    float iz;
    int px, py, r;
    for (i = 0 ; i < sector->lights.Length () ; i++)
    {
      v = rview->Other2This (((csStatLight*)sector->lights[i])->GetCenter ());
      if (v.z > SMALL_Z)
      {
        iz = rview->aspect/v.z;
        px = QInt (v.x * iz + csWorld::shift_x);
        py = csWorld::frame_height - 1 - QInt (v.y * iz + csWorld::shift_y);
        r = QInt (.3 * iz);
        if (ABS (coord_check_vector.x - px) < 5 && ABS (coord_check_vector.y - (csWorld::frame_height-1-py)) < 5)
        {
	  csLight* light = (csLight*)sector->lights[i];
	  if (check_light)
	  {
            if (Sys->selected_light == light) Sys->selected_light = NULL;
	    else Sys->selected_light = light;
	    //check_light = false;
	  }
          Sys->Printf (MSG_CONSOLE, "Selected light %s/(%f,%f,%f).\n",
                    csNameObject::GetName(*sector), light->GetCenter ().x,
                    light->GetCenter ().y, light->GetCenter ().z);
          Sys->Printf (MSG_DEBUG_0, "Selected light %s/(%f,%f,%f).\n",
                    csNameObject::GetName(*sector), light->GetCenter ().x,
                    light->GetCenter ().y, light->GetCenter ().z);
        }
      }
    }
  }
}

// Callback for DrawFunc() to show an outline for all polygons and lights.
// If callback_data in 'rview' is not NULL then we only show outline for
// selected light and/or polygon.
void draw_edges (csRenderView* rview, int type, void* entity)
{
  ITextureManager* txtmgr;
  Gfx3D->GetTextureManager (&txtmgr);
  int red, white, blue, yellow, selcol;
  txtmgr->FindRGB (255, 255, 255, white);
  txtmgr->FindRGB (255, 0, 0, red);
  txtmgr->FindRGB (0, 0, 255, blue);
  txtmgr->FindRGB (255, 255, 0, yellow);

  bool hilighted_only = !!rview->callback_data;
  if (hilighted_only) selcol = yellow;
  else selcol = white;
  static csPolygon3D* last_poly = NULL;

  if (type == CALLBACK_POLYGON)
  {
    // Here we depend on CALLBACK_POLYGON being called right before CALLBACK_POLYGON2D.
    last_poly = (csPolygon3D*)entity;
  }
  else if (type == CALLBACK_POLYGON2D)
  {
    csPolygon2D* polygon = (csPolygon2D*)entity;
    if (!hilighted_only || Sys->selected_polygon == last_poly)
      polygon->Draw (rview->g2d, selcol);
  }
  else if (type == CALLBACK_POLYGONQ)
  {
    if (!hilighted_only)
    {
      G3DPolygonDPQ* dpq = (G3DPolygonDPQ*)entity;
      int i1 = dpq->num-1;
      int i;
      for (i = 0 ; i < dpq->num ; i++)
      {
        rview->g2d->DrawLine (dpq->vertices[i].sx, csWorld::frame_height - 1 - dpq->vertices[i].sy,
      	  dpq->vertices[i1].sx, csWorld::frame_height - 1 - dpq->vertices[i1].sy, blue);
        i1 = i;
      }
    }
  }
  else if (type == CALLBACK_SECTOR)
  {
    csSector* sector = (csSector*)entity;
    int i;
    csVector3 v;
    float iz;
    int px, py, r;
    for (i = 0 ; i < sector->lights.Length () ; i++)
    {
      csStatLight* light = (csStatLight*)(sector->lights[i]);
      if (!hilighted_only || Sys->selected_light == light)
      {
        v = rview->Other2This (light->GetCenter ());
        if (v.z > SMALL_Z)
        {
          iz = rview->aspect/v.z;
          px = QInt (v.x * iz + csWorld::shift_x);
          py = csWorld::frame_height - 1 - QInt (v.y * iz + csWorld::shift_y);
          r = QInt (.3 * iz);
          rview->g2d->DrawLine (px-r, py-r, px+r, py+r, selcol);
          rview->g2d->DrawLine (px+r, py-r, px-r, py+r, selcol);
          rview->g2d->DrawLine (px, py-2, px, py+2, red);
          rview->g2d->DrawLine (px+2, py, px-2, py, red);
        }
      }
    }
  }
}

// Callback for DrawFunc() to show a 3D map of everything that is visible.
void draw_map (csRenderView* /*rview*/, int type, void* entity)
{
  csWireFrame* wf = Sys->wf->GetWireframe ();
  if (type == CALLBACK_POLYGON)
  {
    csPolygon3D* poly = (csPolygon3D*)entity;
    int j;
    csWfPolygon* po = wf->AddPolygon ();
    po->SetVisColor (wf->GetYellow ());
    po->SetNumVertices (poly->GetNumVertices ());
    for (j = 0 ; j < poly->GetNumVertices () ; j++) po->SetVertex (j, poly->Vwor (j));
    po->Prepare ();
  }
  else if (type == CALLBACK_SECTOR)
  {
    csSector* sector = (csSector*)entity;
    int i;
    for (i = 0 ; i < sector->lights.Length () ; i++)
    {
      csWfVertex* vt = wf->AddVertex (((csStatLight*)sector->lights[i])->GetCenter ());
      vt->SetColor (wf->GetRed ());
    }
  }
}

// Callback for DrawFunc() to dump debug information about everything
// that is currently visible. This is useful to debug clipping errors
// and other visual errors.
int dump_visible_indent = 0;
void dump_visible (csRenderView* rview, int type, void* entity)
{
  int i;
  char indent_spaces[255];
  int ind = dump_visible_indent;
  if (ind > 254) ind = 254;
  for (i = 0 ; i < ind ; i++) indent_spaces[i] = ' ';
  indent_spaces[ind] = 0;

  if (type == CALLBACK_POLYGON)
  {
    csPolygon3D* poly = (csPolygon3D*)entity;
    const char* name = csNameObject::GetName (*poly);
    if (!name) name = "(NULL)";
    const char* pname = csNameObject::GetName (*(csPolygonSet*)poly->GetParent ());
    if (!pname) pname = "(NULL)";
    Sys->Printf (MSG_DEBUG_0, "%03d%sPolygon '%s/%s' ------\n",
    	dump_visible_indent, indent_spaces, pname, name);
    if (poly->GetPortal ())
      Sys->Printf (MSG_DEBUG_0, "%03d%s   | Polygon has a portal.\n",
        dump_visible_indent, indent_spaces);
    for (i = 0 ; i < poly->GetNumVertices () ; i++)
    {
      csVector3& vw = poly->Vwor (i);
      csVector3& vc = poly->Vcam (i);
      Sys->Printf (MSG_DEBUG_0, "%03d%s   | %d: wor=(%f,%f,%f) cam=(%f,%f,%f)\n",
      	dump_visible_indent, indent_spaces, i, vw.x, vw.y, vw.z, vc.x, vc.y, vc.z);
    }
  }
  else if (type == CALLBACK_POLYGON2D)
  {
    csPolygon2D* poly = (csPolygon2D*)entity;
    Sys->Printf (MSG_DEBUG_0, "%03d%s2D Polygon ------\n", dump_visible_indent, indent_spaces);
    for (i = 0 ; i < poly->GetNumVertices () ; i++)
    {
      csVector2 v = *poly->GetVertex (i);
      Sys->Printf (MSG_DEBUG_0, "%03d%s   | %d: persp=(%f,%f)\n",
      	dump_visible_indent, indent_spaces, i, v.x, v.y);
    }
  }
  else if (type == CALLBACK_POLYGONQ)
  {
    G3DPolygonDPQ* dpq = (G3DPolygonDPQ*)entity;
  }
  else if (type == CALLBACK_SECTOR)
  {
    csSector* sector = (csSector*)entity;
    const char* name = csNameObject::GetName (*sector);
    if (!name) name = "(NULL)";
    Sys->Printf (MSG_DEBUG_0, "%03d%s BEGIN Sector '%s' ------------\n",
    	dump_visible_indent+1, indent_spaces, name);
    for (i = 0 ; i < sector->GetNumVertices () ; i++)
    {
      csVector3& vw = sector->Vwor (i);
      csVector3& vc = sector->Vcam (i);
      Sys->Printf (MSG_DEBUG_0, "%03d%s   | %d: wor=(%f,%f,%f) cam=(%f,%f,%f)\n",
      	dump_visible_indent+1, indent_spaces, i, vw.x, vw.y, vw.z, vc.x, vc.y, vc.z);
    }
    dump_visible_indent++;
  }
  else if (type == CALLBACK_SECTOREXIT)
  {
    csSector* sector = (csSector*)entity;
    const char* name = csNameObject::GetName (*sector);
    if (!name) name = "(NULL)";
    Sys->Printf (MSG_DEBUG_0, "%03d%sEXIT Sector '%s' ------------\n",
    	dump_visible_indent, indent_spaces, name);
    dump_visible_indent--;
  }
  else if (type == CALLBACK_THING)
  {
    csThing* thing = (csThing*)entity;
    const char* name = csNameObject::GetName (*thing);
    if (!name) name = "(NULL)";
    Sys->Printf (MSG_DEBUG_0, "%03d%s BEGIN Thing '%s' ------------\n",
    	dump_visible_indent+1, indent_spaces, name);
    for (i = 0 ; i < thing->GetNumVertices () ; i++)
    {
      csVector3& vw = thing->Vwor (i);
      csVector3& vc = thing->Vcam (i);
      Sys->Printf (MSG_DEBUG_0, "%03d%s   | %d: wor=(%f,%f,%f) cam=(%f,%f,%f)\n",
      	dump_visible_indent+1, indent_spaces, i, vw.x, vw.y, vw.z, vc.x, vc.y, vc.z);
    }
    dump_visible_indent++;
  }
  else if (type == CALLBACK_THINGEXIT)
  {
    csThing* thing = (csThing*)entity;
    const char* name = csNameObject::GetName (*thing);
    if (!name) name = "(NULL)";
    Sys->Printf (MSG_DEBUG_0, "%03d%sEXIT Thing '%s' ------------\n",
    	dump_visible_indent, indent_spaces, name);
    dump_visible_indent--;
  }
}

//------------------------------------------------------------------------

void WalkTest::DrawFrame (long elapsed_time, long current_time)
{
  (void)elapsed_time; (void)current_time;

  //not used since we need WHITE background not black
  int drawflags = 0; /* do_clear ? CSDRAW_CLEARSCREEN : 0; */
  if (do_clear || map_mode == MAP_ON)
  {
    if (Gfx3D->BeginDraw (CSDRAW_2DGRAPHICS) != S_OK)
      return;
    Gfx2D->Clear (map_mode == MAP_ON ? 0 : 255);
  }

  if (!System->Console->IsActive ()
   || ((SimpleConsole*)(System->Console))->IsTransparent ())
  {
    // Tell Gfx3D we're going to display 3D things
    if (Gfx3D->BeginDraw (drawflags | CSDRAW_3DGRAPHICS) != S_OK)
      return;
    if (map_mode != MAP_ON) view->Draw ();
    // no need to clear screen anymore
    drawflags = 0;

    Sys->world->AdvanceSpriteFrames (current_time);

    csDynLight* dyn = Sys->world->GetFirstDynLight ();
    while (dyn)
    {
      extern void HandleDynLight (csDynLight*);
      csDynLight* dn = dyn->GetNext ();
      if (dyn->GetObj(csDataObject::Type())) HandleDynLight (dyn);
      dyn = dn;
    }
    int i;
    for (i = 0 ; i < Sys->world->sprites.Length () ; i++)
    {
      extern void HandleSprite (csSprite3D*);
      csSprite3D* spr = (csSprite3D*)Sys->world->sprites[i];
      HandleSprite (spr);
    }
  }

  // Start drawing 2D graphics
  if (Gfx3D->BeginDraw (drawflags | CSDRAW_2DGRAPHICS) != S_OK)
    return;

  if (map_mode != MAP_OFF)
  {
    wf->GetWireframe ()->Clear ();
    view->GetWorld ()->DrawFunc (Gfx3D, view->GetCamera (), view->GetClipper (), draw_map);
    wf->GetWireframe ()->Draw (Gfx3D, wf->GetCamera ());
  }
  else
  {
    if (do_show_z) DrawZbuffer ();
    if (do_edges) view->GetWorld ()->DrawFunc (Gfx3D, view->GetCamera (), view->GetClipper (), draw_edges);
    if (selected_polygon || selected_light) view->GetWorld ()->DrawFunc (Gfx3D, view->GetCamera (), view->GetClipper (), draw_edges, (void*)1);
    if (do_light_frust && selected_light) ((csStatLight*)selected_light)->LightingFunc (show_frustrum);
  }

  SimpleConsole* scon = (SimpleConsole*)System->Console;
  scon->Print (NULL);

  if (!System->Console->IsActive ())
  {
    if (do_fps)
    {
      GfxWrite(11, FRAME_HEIGHT-11, 0, -1, "FPS=%f", timeFPS);
      GfxWrite(10, FRAME_HEIGHT-10, scon->get_fg (), -1, "FPS=%f", timeFPS);
    } /* endif */
    if (do_stats)
    {
      char buffer[30];
      sprintf (buffer, "pc=%d pd=%d po=%d", Stats::polygons_considered,
        Stats::polygons_drawn, Stats::portals_drawn);
      GfxWrite(FRAME_WIDTH-24*8-1, FRAME_HEIGHT-11, 0, -1, "%s", buffer);
      GfxWrite(FRAME_WIDTH-24*8, FRAME_HEIGHT-10, scon->get_fg (), -1, "%s", buffer);
    }
    else if (do_show_coord)
    {
      char buffer[100];
      sprintf (buffer, "%2.2f,%2.2f,%2.2f: %s",
        view->GetCamera ()->GetW2CTranslation ().x, view->GetCamera ()->GetW2CTranslation ().y,
        view->GetCamera ()->GetW2CTranslation ().z, csNameObject::GetName(*(view->GetCamera ()->GetSector())));
      Gfx2D->Write(FRAME_WIDTH-24*8-1, FRAME_HEIGHT-11, 0, -1, buffer);
      Gfx2D->Write(FRAME_WIDTH-24*8, FRAME_HEIGHT-10, scon->get_fg (), -1, buffer);
    }
    else if (do_cd)
    {/*
      char buffer[200], names[30];
      sprintf (buffer,"CD %d cam pos %2.2f,%2.2f,%2.2f %c %c",
         collcount,
         view->GetCamera ()->GetW2CTranslation ().x, view->GetCamera ()->GetW2CTranslation ().y,
         view->GetCamera ()->GetW2CTranslation ().z,
         csBeing::player->falling?'F':' ', csBeing::player->climbing?'C':' ');
      for (int i = 0; i < collcount; i++)
      {
        csCollider *id1, *id2;
        if (csCollider::Report(&id1, &id2))
	{
          sprintf(names,"[%s,%s] ",id1->GetName (),id2->GetName ());
          strcat(buffer,names);
	}
      }
      Gfx2D->Write(8-1, FRAME_HEIGHT-21, 0, -1, buffer);
      Gfx2D->Write(8, FRAME_HEIGHT-20, scon->get_fg (), -1, buffer);*/
    } /* endif */

    if (cslogo)
    {
      int w = cslogo->Width()  * FRAME_WIDTH  / 640;
      int h = cslogo->Height() * FRAME_HEIGHT / 480;
      int x = FRAME_WIDTH - 2 - w*152/256;
      cslogo->Draw(Gfx2D, x,2,w,h);
    }
  } /* endif */

  // Drawing code ends here
  Gfx3D->FinishDraw ();
  // Print the output.
  Gfx3D->Print (NULL);
}

int cnt = 1;
time_t time0 = (time_t)-1;

#define EPS 0.00001

int FindIntersection(csVector3 *tri1,csVector3 *tri2,csVector3 line[2])
{
  int i,j;
  csVector3 v1[3],v2[3];

  for(i=0;i<3;i++)
  {
    j=(i+1)%3;
    v1[i]=tri1[j]-tri1[i];
    v2[i]=tri2[j]-tri2[i];
  }

  csVector3 n1=v1[0]%v1[1];
  csVector3 n2=v2[0]%v2[1];

  float d1=-n1*tri1[0],d2=-n2*tri2[0];

  csVector3 d=n1%n2;

  int index=0;
  float max=fabs(d.x);
  if(fabs(d.y)>max)
    max=fabs(d.y), index=1;
  if(fabs(d.z)>max)
    max=fabs(d.z), index=2;

  int m1=0,m2=0,n=0;
  float t1[3],t2[3];
  csVector3 p1[2],p2[2];
  p1[0].Set (0, 0, 0);
  p1[1].Set (0, 0, 0);
  p2[0].Set (0, 0, 0);
  p2[1].Set (0, 0, 0);
  float isect1[2],isect2[2],isect[4];
  csVector3 *idx[4];

  for(i=0;i<3;i++)
  {
    float div1=n2*v1[i],div2=n1*v2[i];
    float pr1=-(n2*tri1[i]+d2),pr2=-(n1*tri2[i]+d1);

    if(fabs(div1)<EPS)
    {
      if(fabs(pr1)<EPS)
      {
	// line is in the plane of intersection
	t1[i]=0;
      }
      else
      {
	// line is parallel to the plane of
	// intersection, so we don't need it ;)
	t1[i]=15.0;
      }
    }
    else
      t1[i]=pr1/div1;

    if(fabs(div2)<EPS)
    {
      if(fabs(pr2)<EPS)
      {
	// line is in the plane of intersection
	t2[i]=0;
      }
      else
      {
	// line is parallel to the plane of
	// intersection, so we don't need it ;)
	t2[i]=15.0;
      }
    }
    else
      t2[i]=pr2/div2;

    if(t1[i]>=0.0&&t1[i]<=1.0&&m1!=2)
    {
      p1[m1]=tri1[i]+v1[i]*t1[i];
      isect1[m1]=p1[m1][index];
      idx[n]=p1+m1;
      isect[n++]=isect1[m1++];
    }
    if(t2[i]>=0.0&&t2[i]<=1.0&&m2!=2)
    {
      p2[m2]=tri2[i]+v2[i]*t2[i];
      isect2[m2]=p2[m2][index];
      idx[n]=p2+m2;
      isect[n++]=isect2[m2++];
    }
  }

  if(n<4)
  {
    // triangles are not intersecting
    return 0;
  }

  for(i=0;i<4;i++)
  {
    for(j=i+1;j<4;j++)
    {
      if(isect[i]>isect[j])
      {
	csVector3 *p=idx[j];
	idx[j]=idx[i];
	idx[i]=p;

	float _=isect[i];
	isect[i]=isect[j];
	isect[j]=_;
      }
    }
  }

  line[0]=*idx[1];
  line[1]=*idx[2];

  return 1;
}

int FindIntersection(CDTriangle *t1,CDTriangle *t2,csVector3 line[2])
{
  csVector3 tri1[3]; tri1[0]=t1->p1; tri1[1]=t1->p2; tri1[2]=t1->p3;
  csVector3 tri2[3]; tri2[0]=t2->p1; tri2[1]=t2->p2; tri2[2]=t2->p3;

  return FindIntersection(tri1,tri2,line);
}

// Define the player bounding box.
// The camera's lens or person's eye is assumed to be
// at 0,0,0.  The height (DY), width (DX) and depth (DZ).
// Is the size of the camera/person and the origin
// coordinates (OX,OY,OZ) locate the bbox with respect to the eye.
// This player is 1.8 metres tall (assuming 1cs unit = 1m) (6feet)
#define DX    0.75
#define DY    1.4
#define DZ    0.75
#define OY   (-0.7)

#define DX_L  0.4
#define DZ_L  0.4

#define DX_2  (DX/2)
#define DZ_2  (DZ/2)

#define DX_2L (DX_L/2)
#define DZ_2L (DZ_L/2)

#define OYL  (-1.1)
#define DYL  (OY-OYL)

void WalkTest::CreateColliders(void)
{
  csPolygon3D *p;
  CHK (csPolygonSet *pb = new csPolygonSet());
  csNameObject::AddName(*pb, "Player's Body");

  pb->AddVertex(-DX_2, OY,    -DZ_2);
  pb->AddVertex(-DX_2, OY,    DZ_2);
  pb->AddVertex(-DX_2, OY+DY, DZ_2);
  pb->AddVertex(-DX_2, OY+DY, -DZ_2);
  pb->AddVertex(DX_2,  OY,    -DZ_2);
  pb->AddVertex(DX_2,  OY,    DZ_2);
  pb->AddVertex(DX_2,  OY+DY, DZ_2);
  pb->AddVertex(DX_2,  OY+DY, -DZ_2);

  // Left
  p = pb->NewPolygon (0);

  p->AddVertex (0); p->AddVertex (1);
  p->AddVertex (2); p->AddVertex (3);

  // Right
  p = pb->NewPolygon (0);
  p->AddVertex (4); p->AddVertex (5);
  p->AddVertex (6); p->AddVertex (7);

  // Bottom
  p = pb->NewPolygon (0);
  p->AddVertex (0); p->AddVertex (1);
  p->AddVertex (5); p->AddVertex (4);

  // Top
  p = pb->NewPolygon (0);
  p->AddVertex (3); p->AddVertex (2);
  p->AddVertex (6); p->AddVertex (7);

  // Front
  p = pb->NewPolygon (0);
  p->AddVertex (1); p->AddVertex (5);
  p->AddVertex (6); p->AddVertex (2);

  // Back
  p = pb->NewPolygon (0);
  p->AddVertex (0); p->AddVertex (4);
  p->AddVertex (7); p->AddVertex (3);

  this->body=new csCollider(pb);

  CHK (csPolygonSet *pl = new csPolygonSet());

  pl->AddVertex(-DX_2L, OYL,     -DZ_2L);
  pl->AddVertex(-DX_2L, OYL,     DZ_2L);
  pl->AddVertex(-DX_2L, OYL+DYL, DZ_2L);
  pl->AddVertex(-DX_2L, OYL+DYL, -DZ_2L);
  pl->AddVertex(DX_2L,  OYL,     -DZ_2L);
  pl->AddVertex(DX_2L,  OYL,     DZ_2L);
  pl->AddVertex(DX_2L,  OYL+DYL, DZ_2L);
  pl->AddVertex(DX_2L,  OYL+DYL, -DZ_2L);

  // Left
  p = pl->NewPolygon (0);

  p->AddVertex (0); p->AddVertex (1);
  p->AddVertex (2); p->AddVertex (3);

  // Right
  p = pl->NewPolygon (0);
  p->AddVertex (4); p->AddVertex (5);
  p->AddVertex (6); p->AddVertex (7);

  // Bottom
  p = pl->NewPolygon (0);
  p->AddVertex (0); p->AddVertex (1);
  p->AddVertex (5); p->AddVertex (4);

  // Top
  p = pl->NewPolygon (0);
  p->AddVertex (3); p->AddVertex (2);
  p->AddVertex (6); p->AddVertex (7);

  // Front
  p = pl->NewPolygon (0);
  p->AddVertex (1); p->AddVertex (5);
  p->AddVertex (6); p->AddVertex (2);

  // Back
  p = pl->NewPolygon (0);
  p->AddVertex (0); p->AddVertex (4);
  p->AddVertex (7); p->AddVertex (3);

  this->legs=new csCollider(pl);

  if(!this->body||!this->legs)
    do_cd=false;
}

#define MAXSECTORSOCCUPIED  20

// No more than 1000 collisions ;)
extern collision_pair *CD_contact;
//collision_pair *our_cd_contact=0;
static collision_pair our_cd_contact[1000];//=0;
static int num_our_cd;

int FindSectors(csVector3 v, csVector3 d, csSector *s, csSector **sa)
{
  sa[0] = s;
  int i, c = 1;
  float size = d.x * d.x + d.y * d.y + d.z * d.z;
  for(i = 0;i < s->GetNumPolygons() && c < MAXSECTORSOCCUPIED; i++)
  {
    // Get the polygon of for this sector.
    csPolygon3D* p = (csPolygon3D*) s->GetPolygon (i);
    csPortal* portal = p->GetPortal ();
    // Handle only portals.
    if(portal != NULL && portal->PortalType () == PORTAL_CS)
    {
      csPortalCS *pcs = (csPortalCS*)portal;
      if (p->GetPlane ()->SquaredDistance (v) < size)
      {
        sa[c] = pcs->GetSector ();
        c++;
      }
//    sa[c] = pcs->GetSector();
//    c++;
    }
  }
  return c;
}

int CollisionDetect (csCollider *c, csSector* sp, csTransform *cdt)
{
  int hit = 0;

  // Check collision with this sector.
  csCollider::numHits = 0;
  csCollider::CollidePair (c, csColliderPointerObject::GetCollider(*sp), cdt);
  hit += csCollider::numHits;
  for (int i=0 ; i<csCollider::numHits ; i++)
    our_cd_contact[num_our_cd++] = CD_contact[i];

  if (csCollider::firstHit && hit)
    return 1;

  // Check collision of with the things in this sector.
  csThing *tp = sp->GetFirstThing ();
  while (tp)
  {
    // TODO, if and when Things can move, their transform must be passed in.
    csCollider::numHits=0;
    csCollider::CollidePair(c,csColliderPointerObject::GetCollider(*tp),cdt);
    hit += csCollider::numHits;

    for (int i=0 ; i<csCollider::numHits ; i++)
      our_cd_contact[num_our_cd++] = CD_contact[i];

    if (csCollider::firstHit && hit)
      return 1;

    tp = (csThing*)(tp->GetNext ());
    // TODO, should test which one is the closest.
  }

  return hit;
}

void DoGravity (csVector3& pos, csVector3& vel)
{
  pos=Sys->view->GetCamera ()->GetOrigin ();

  csVector3 new_pos = pos+vel;
  csMatrix3 m;
  csOrthoTransform test (m, new_pos);

  csSector *n[MAXSECTORSOCCUPIED];
  int num_sectors = FindSectors (new_pos, 4*Sys->body->GetBbox()->d,
    Sys->view->GetCamera()->GetSector(), n);

  num_our_cd = 0;
  csCollider::firstHit = false;
  int hits = 0;

  csCollider::CollideReset ();

  for ( ; num_sectors-- ; )
    hits += CollisionDetect (Sys->body, n[num_sectors], &test);

  for (int j=0 ; j<hits ; j++)
  {
    CDTriangle *wall = our_cd_contact[j].tr2;
    csVector3 n = ((wall->p3-wall->p2)%(wall->p2-wall->p1)).Unit();
    if (n*vel<0)
      continue;
    vel = -(vel%n)%n;
  }

  // We now know our (possible) velocity. Let's try to move up or down, if possible
  new_pos = pos+vel;
  test = csOrthoTransform (csMatrix3(), new_pos);

  num_sectors = FindSectors (new_pos, 4*Sys->legs->GetBbox()->d, Sys->view->GetCamera()->GetSector(), n);

  num_our_cd = 0;
  csCollider::firstHit = false;
  csCollider::numHits = 0;
  int hit = 0;

  csCollider::CollideReset ();

  for ( ; num_sectors-- ; )
    hit += CollisionDetect (Sys->legs, n[num_sectors], &test);

  if (!hit)
  {
    Sys->on_ground = false;
    if (Sys->do_gravity && !Sys->move_3d)
      vel.y -= 0.004;
  }
  else
  {
    float max_y=-1e10;

    for (int j=0 ; j<hit ; j++)
    {
      // � -- ������!.. � ��������� ��� ����� ��� ���. � ���� ���� ������
      //  ������������ �� ���������, � ��������. � �������... ��� �� ������ ;) -- D.D.

      CDTriangle first = *our_cd_contact[j].tr1;
      CDTriangle second = *our_cd_contact[j].tr2;

      csVector3 n=((second.p3-second.p2)%(second.p2-second.p1)).Unit ();

      if (n*csVector3(0,-1,0)<0.7)
        continue;

      csVector3 line[2];

      first.p1 += new_pos;
      first.p2 += new_pos;
      first.p3 += new_pos;

      if (FindIntersection (&first,&second,line))
      {
        if (line[0].y>max_y)
          max_y=line[0].y;
        if (line[1].y>max_y)
          max_y=line[1].y;
      }
    }

    float p = new_pos.y-max_y+OYL+0.01;
    if (fabs(p)<DYL-0.01)
    {
      if (max_y != -1e10)
        new_pos.y = max_y-OYL-0.01;

      if (vel.y<0)
        vel.y = 0;
    }
    Sys->on_ground = true;
  }

  new_pos -= Sys->view->GetCamera ()->GetOrigin ();
  Sys->view->GetCamera ()->MoveWorld (new_pos);

  Sys->velocity = Sys->view->GetCamera ()->GetO2T ()*vel;

  if(!Sys->do_gravity)
    Sys->velocity.y -= SIGN (Sys->velocity.y) * MIN (0.017, fabs (Sys->velocity.y));
}

void WalkTest::PrepareFrame (long elapsed_time, long current_time)
{
  (void)elapsed_time; (void)current_time;

  CLights::LightIdle (); // SJI

  if (do_cd)
  {
    if (!player_spawned)
    {
      CreateColliders ();
      player_spawned=true;
    }

    for (int repeats=0 ; repeats<((elapsed_time)/25.0+0.5) ; repeats++)
    {
      if (move_3d)
      {
        // If we are moving in 3d then don't do any camera correction.
      }
      else
      {
        view->GetCamera ()->SetT2O (csMatrix3 ());
        view->GetCamera ()->RotateWorld (csVector3 (0,1,0), angle.y);
        if (!do_gravity)
          view->GetCamera ()->Rotate (csVector3 (1,0,0), angle.x);
      }

      csVector3 vel = view->GetCamera ()->GetT2O ()*velocity;

      static bool check_once = false;
      if (ABS (vel.x) < SMALL_EPSILON && ABS (vel.y) < SMALL_EPSILON && ABS (vel.z) < SMALL_EPSILON)
      {
        // If we don't move we don't have to do the collision detection tests.
	// However, we need to do it once to make sure that we are standing
	// on solid ground. So we first set 'check_once' to true to enable
	// one more test.
	if (check_once == false) { check_once = true; DoGravity (pos, vel); }
      }
      else { check_once = false; DoGravity (pos, vel); }

      if (do_gravity && !move_3d)
        view->GetCamera ()->Rotate (csVector3 (1,0,0), angle.x);

      // Apply angle velocity to camera angle
      angle += angle_velocity;
    }
  }

#if 0
  if (do_cd && csBeing::init)
  {
    // TODO ALEX: In future this should depend on whether the whole world
    // or 'active' sectors need to be set up as monsters hunt for player
    // outside of current sector, but this will do for now.

    // Test camera collision.
    // Load camera location into player.
    csBeing::player->sector = view->GetCamera ()->GetSector ();
    csBeing::player->transform = view->GetCamera ();
    collcount = csBeing::player->CollisionDetect ();
    // Load player transformation back into camera.
    view->GetCamera ()->SetW2C (csBeing::player->transform->GetO2T ());
    view->GetCamera ()->SetPosition (csBeing::player->transform->GetO2TTranslation ());
    view->GetCamera ()->SetSector (csBeing::player->sector);

  }
#endif

  if (cnt <= 0)
  {
    time_t time1 = SysGetTime ();
    if (time0 != (time_t)-1)
    {
      if (time1 != time0)
        timeFPS=10000.0f/(float)(time1-time0);
    }
    cnt = 10;
    time0 = SysGetTime ();
  }
  cnt--;

  layer->step_run ();
}

void perf_test ()
{
  Sys->busy_perf_test = true;
  time_t t1, t2, t;
  Sys->Printf (MSG_CONSOLE, "Performance test busy...\n");
  t = t1 = SysGetTime ();
  int i;
  for (i = 0 ; i < 100 ; i++)
  {
    Sys->layer->step_run ();
    Sys->DrawFrame (SysGetTime ()-t, SysGetTime ());
    t = SysGetTime ();
  }
  t2 = SysGetTime ();
  Sys->Printf (MSG_CONSOLE, "%f secs to render 100 frames: %f fps\n",
        (float)(t2-t1)/1000., 100000./(float)(t2-t1));
  Sys->Printf (MSG_DEBUG_0, "%f secs to render 100 frames: %f fps\n",
        (float)(t2-t1)/1000., 100000./(float)(t2-t1));
  cnt = 1;
  time0 = (time_t)-1;
  Sys->busy_perf_test = false;
}

void CaptureScreen (void)
{
#if !defined(OS_MACOS)  // SJI - NON mac stuff - the mac has its own way of doing screen captures
  int i = 0;
  char name[255];
  UByte pall[768];
  UByte *pal = &pall[0];

  do
  {
    sprintf (name, "cryst%02d.pcx", i++);
  } while (i < 100 && (access (name, 0) == 0));

  if (i >= 100) return;

  RGBpaletteEntry* pPalette;
  System->piGI->GetPalette (&pPalette);

  if (pPalette)
  {
    for (i=0; i<256; i++)
    {
      *pal++ = pPalette[i].red;
      *pal++ = pPalette[i].green;
      *pal++ = pPalette[i].blue;
    }
  }

  extern void WritePCX (char *name, unsigned char *data, UByte *pal,
			int width,int height);
  Gfx3D->BeginDraw(CSDRAW_2DGRAPHICS);

  unsigned char* pFirstPixel;
  Gfx2D->GetPixelAt(0,0, &pFirstPixel);

  WritePCX (name, pFirstPixel, pall, FRAME_WIDTH, FRAME_HEIGHT);
  Gfx3D->FinishDraw();
  Sys->Printf (MSG_CONSOLE, "Screenshot: %s", name);

#endif // !OS_MACOS
}

/*---------------------------------------------
 * Our main event loop.
 *---------------------------------------------*/

/*
 * Do a large debug dump just before the program
 * exits. This function can be installed as a last signal
 * handler if the program crashes (for systems that support
 * this).
 */
void debug_dump ()
{
  Sys->view->GetCamera ()->SaveFile ("coord.bug");
  Sys->Printf (MSG_DEBUG_0, "Camera saved in coord.bug\n");
  Dumper::dump (Sys->view->GetCamera ());
  Sys->Printf (MSG_DEBUG_0, "Camera dumped in debug.txt\n");
  Dumper::dump (Sys->world);
  Sys->Printf (MSG_DEBUG_0, "World dumped in debug.txt\n");
}

/*
 * A sample script which just prints a message on screen.
 */
bool do_message_script (IntRunScript* sc, char* data)
{
  sc->get_layer ()->message (data);
  return true;
}

//---------------------------------------------------------------------------


void cleanup ()
{
  pprintf ("Cleaning up...\n");
  free_keymap ();
  /*csBeing::*/Sys->EndWorld ();
  CHK (delete Sys); Sys = NULL;
  pprintf_close();
}

/*---------------------------------------------------------------------*
 * Demo stuff
 *---------------------------------------------------------------------*/

struct DemoInfo
{
  csWorld* world;
};

DemoInfo* demo_info = NULL;

/*
 * The routine to update the screen in demo mode.
 */
void WalkTest::DemoWrite (const char* buf)
{
  if (Console)
  {
    if (Gfx2D)
    {
      VERIFY_SUCCESS (Gfx2D->BeginDraw ());
      csRect area;
      bool dblbuff;
      Gfx2D->GetDoubleBufferState (dblbuff);
      Gfx2D->Clear (0);
      Console->PutText ("%s", buf);
      Console->Print (&area);
      if (dblbuff)
        area.Union (0, 0, FRAME_WIDTH - 1, FRAME_HEIGHT - 1);
      Gfx2D->FinishDraw ();
      Gfx2D->Print (&area);
    }
  }
}

/*
 * Start the demo in the already open screen.
 */
void start_demo ()
{
  CHK (demo_info = new DemoInfo);
  CHK (Sys->world = demo_info->world = new csWorld ());

  ITextureManager* txtmgr;
  Gfx3D->GetTextureManager (&txtmgr);
//Gfx2D->DoubleBuffer (false);
  demo_info->world->Initialize (GetISystemFromSystem (System), System->piGI, config);

  // Initialize the texture manager
  txtmgr->Initialize ();

  // Allocate a uniformly distributed in R,G,B space palette for console
  int r,g,b;
  for (r = 0; r < 8; r++)
    for (g = 0; g < 8; g++)
      for (b = 0; b < 4; b++)
        txtmgr->ReserveColor (r * 32, g * 32, b * 64);

  txtmgr->Prepare ();
  txtmgr->AllocPalette ();

  ((SimpleConsole *)System->Console)->SetupColors (txtmgr);
  ((SimpleConsole *)System->Console)->SetMaxLines (1000);       // Some arbitrary high value.
  ((SimpleConsole *)System->Console)->SetTransparent (0);

  System->DemoReady = true;
}

/*
 * Stop the demo.
 */
void stop_demo ()
{
  if (demo_info)
  {
    CHK (delete demo_info->world);
    CHK (delete demo_info);
  }
}

void WalkTest::EndWorld() {}

void WalkTest::InitWorld (csWorld* world, csCamera* /*camera*/)
{
//  Sys->Printf (MSG_INITIALIZATION, "Computing OBBs ...\n");

  int sn = world->sectors.Length ();
  while (sn > 0)
  {
    sn--;
    csSector* sp = (csSector*)world->sectors[sn];
    // Initialize the sector itself.
    CHK(csCollider* pCollider = new csCollider(sp));
    csColliderPointerObject::SetCollider(*sp, pCollider, true);
    // Initialize the things in this sector.
    csThing* tp = sp->GetFirstThing ();
    while (tp)
    {
      CHK(csCollider* pCollider = new csCollider(tp));
      csColliderPointerObject::SetCollider(*tp, pCollider, true);
      tp = (csThing*)(tp->GetNext ());
    }
  }
  // Initialize all sprites for collision detection.
  csSprite3D* spp;
  int i;
  for (i = 0 ; i < world->sprites.Length () ; i++)
  {
    spp = (csSprite3D*)world->sprites[i];

    // TODO: Should create beings for these.
    CHK(csCollider* pCollider = new csCollider(spp));
    csColliderPointerObject::SetCollider(*spp, pCollider, true);
  }

  // Create a player object that follows the camera around.
//  player = csBeing::PlayerSpawn("Player");

//  init = true;
//  Sys->Printf (MSG_INITIALIZATION, "DONE\n");
}

/*---------------------------------------------------------------------*
 * Main function
 *---------------------------------------------------------------------*/
int main (int argc, char* argv[])
{
  srand (time (NULL));

  // Create our main class which is the driver for WalkTest.
  CHK (Sys = new WalkTest ());

  // Open our configuration file and read the configuration values
  // specific for WalkTest.
  //@@@Config should belong to WalkTest.
  CHK (config = new csIniFile ("cryst.cfg"));

  // create the converter class for testing
  CHK (ImportExport = new converter());

  // process import/export files from config and print log for testing

  ImportExport->ProcessConfig (config);

  // free memory - delete this if you want to use the data in the buffer

  CHK (delete ImportExport);
  // end converter test

  Sys->do_fps = config->GetYesNo ("WalkTest", "FPS", true);
  Sys->do_stats = config->GetYesNo ("WalkTest", "STATS", false);
  Sys->do_cd = config->GetYesNo ("WalkTest", "COLLDET", true);
  strcpy (WalkTest::world_file, config->GetStr ("World", "WORLDFILE", "world"));

  // Create our world. The world is the representation of
  // the 3D engine.
  CHK (csWorld* world = new csWorld ());

  // Initialize the main system. This will load all needed
  // COM drivers (3D, 2D, network, sound, ...) and initialize them.
  if (!Sys->Initialize (argc, argv, world->GetEngineConfigCOM ()))
  {
    Sys->Printf (MSG_FATAL_ERROR, "Error initializing system!\n");
    cleanup ();
    fatal_exit (0, false);
  }

#ifdef DEBUG
  // enable all kinds of useful exceptions on a x86
  // note that we can't do it above since at least on OS/2 each dynamic
  // library on loading/initialization resets the control word to default
  _control87 (0x32, 0x3f);
#else
  // this will disable exceptions on DJGPP (for the "industrial" version)
  _control87 (0x3f, 0x3f);
#endif

  // Open the main system. This will open all the previously loaded
  // COM drivers.
  if (!Sys->Open ("Crystal Space"))
  {
    Sys->Printf (MSG_FATAL_ERROR, "Error opening system!\n");
    cleanup ();
    fatal_exit (0, false);
  }

  // For debugging purposes.
  Gfx3D->SetRenderState (G3DRENDERSTATE_DEBUGENABLE,
    config->GetYesNo ("WalkTest", "DEBUG", false));

  // Create console object for text and commands.
  CHK (System->Console = new SimpleConsole ());

  // Start the 'demo'. This is currently nothing more than
  // the display of all startup messages on the console.
  start_demo ();

  // Some commercials...
  Sys->Printf (MSG_INITIALIZATION, "Crystal Space version %s (%s).\n", VERSION, RELEASE_DATE);
  Sys->Printf (MSG_INITIALIZATION, "Created by Jorrit Tyberghein and others...\n\n");
  ITextureManager* txtmgr;
  Gfx3D->GetTextureManager (&txtmgr);
  txtmgr->SetVerbose (true);

  // Initialize our world now that the system is ready.
  Sys->world = world;
  world->Initialize (GetISystemFromSystem (System), System->piGI, config);

  // csView is a view encapsulating both a camera and a clipper.
  // You don't have to use csView as you can do the same by
  // manually creating a camera and a clipper but it makes things a little
  // easier.
  CHK (Sys->view = new csView (world, Gfx3D));

  // Initialize the command processor with the world and camera.
  Command::Initialize (world, Sys->view->GetCamera (), Gfx3D, System->Console, GetISystemFromSystem (System));

  // Create the language layer needed for scripting.
  // Also register a small C script so that it can be used
  // by levels. large.zip uses this script.
  CHK (Sys->layer = new LanguageLayer (world, Sys->view->GetCamera ()));
  int_script_reg.reg ("message", &do_message_script);

  // Now we have two choices. Either we create an infinite
  // maze (random). This happens when the '-infinite' commandline
  // option is given. Otherwise we load the given world.
  csSector* room;

  if (Sys->do_infinite)
  {
    // The infinite maze.

    Sys->Printf (MSG_INITIALIZATION, "Creating initial room!...\n");
    //@@@ Think
    csPolygon3D::do_force_recalc = true;

    // Disable collision detection for the infinite maze because
    // it does not work here yet.
    Sys->do_cd = false;

    // Load the standard library.
    CSLoader::LoadLibrary (world, "standard", "standard.zip");

    // Load two textures that are used in the maze.
    CSLoader::LoadTexture (world, "txt", "stone4.gif");
    CSLoader::LoadTexture (world, "txt2", "mystone2.gif");

    // After loading the textures but BEFORE using them the
    // texture manager needs to be prepared.
    txtmgr->Prepare ();

    // Create the initial (non-random) part of the maze.
    CHK (Sys->infinite_maze = new InfiniteMaze ());
    room = Sys->infinite_maze->create_six_room (world, 0, 0, 0)->sector;
    Sys->infinite_maze->create_six_room (world, 0, 0, 1);
    Sys->infinite_maze->create_six_room (world, 0, 0, 2);
    Sys->infinite_maze->create_six_room (world, 1, 0, 2);
    Sys->infinite_maze->create_six_room (world, 0, 1, 2);
    Sys->infinite_maze->create_six_room (world, 1, 1, 2);
    Sys->infinite_maze->create_six_room (world, 0, 0, 3);
    Sys->infinite_maze->create_six_room (world, 0, 0, 4);
    Sys->infinite_maze->create_six_room (world, -1, 0, 4);
    Sys->infinite_maze->create_six_room (world, -2, 0, 4);
    Sys->infinite_maze->create_six_room (world, 0, -1, 3);
    Sys->infinite_maze->create_six_room (world, 0, -2, 3);
    Sys->infinite_maze->create_six_room (world, 0, 1, 3);
    Sys->infinite_maze->create_six_room (world, 0, 2, 3);
    Sys->infinite_maze->connect_infinite (0, 0, 0, 0, 0, 1);
    Sys->infinite_maze->connect_infinite (0, 0, 1, 0, 0, 2);
    Sys->infinite_maze->connect_infinite (0, 0, 2, 0, 0, 3);
    Sys->infinite_maze->connect_infinite (0, 0, 2, 1, 0, 2);
    Sys->infinite_maze->connect_infinite (0, 0, 2, 0, 1, 2);
    Sys->infinite_maze->connect_infinite (1, 1, 2, 0, 1, 2);
    Sys->infinite_maze->connect_infinite (1, 1, 2, 1, 0, 2);
    Sys->infinite_maze->connect_infinite (0, 0, 3, 0, 0, 4);
    Sys->infinite_maze->connect_infinite (-1, 0, 4, 0, 0, 4);
    Sys->infinite_maze->connect_infinite (-2, 0, 4, -1, 0, 4);
    Sys->infinite_maze->connect_infinite (0, 0, 3, 0, -1, 3);
    Sys->infinite_maze->connect_infinite (0, -1, 3, 0, -2, 3);
    Sys->infinite_maze->connect_infinite (0, 0, 3, 0, 1, 3);
    Sys->infinite_maze->connect_infinite (0, 1, 3, 0, 2, 3);
    Sys->infinite_maze->create_loose_portal (-2, 0, 4, -2, 1, 4);

    // Prepare the world. This will calculate all lighting and
    // prepare the lightmaps for the 3D rasterizer.
    world->Prepare (Gfx3D);
  }
  else
  {
    // Load from a world file.

    Sys->Printf (MSG_INITIALIZATION, "Loading world '%s'...\n", WalkTest::world_file);

    // Load the world from the file.
    if (!CSLoader::LoadWorldFile (world, Sys->layer, WalkTest::world_file))
    {
      Sys->Printf (MSG_FATAL_ERROR, "Loading of world failed!\n");
      cleanup ();
      fatal_exit (0, false);
    }

    // Load the standard library.
    if (!CSLoader::LoadLibrary (world, "standard", "standard.zip"))
    {
      //Error message was already printed be CSLoader...
      cleanup ();
      fatal_exit (0, false);
    }

    //Find the Crystal Space logo and set the renderer Flag to for_2d, to allow
    //the use in the 2D part.
    csTextureList *texlist = world->GetTextures ();
    ASSERT(texlist);
    csTextureHandle *texh = texlist->GetTextureMM ("cslogo.gif");
    if (texh)
    {
      texh->for_2d = true;
    }

    // Prepare the world. This will calculate all lighting and
    // prepare the lightmaps for the 3D rasterizer.
    world->Prepare (Gfx3D);

    //Create a 2D sprite for the Logo
    if (texh)
    {
      int w, h;
      ITextureHandle* phTex = texh->GetTextureHandle();
      phTex->GetBitmapDimensions(w,h);
      CHK (Sys->cslogo = new csSprite2D (texh, 0, 0, w, h));
    }

    // Look for the start sector in this world.
    char* strt = (char*)(world->start_sector ? world->start_sector : "room");
    room = (csSector*)world->sectors.FindByName (strt);
    if (!room)
    {
      Sys->Printf (MSG_FATAL_ERROR,
          "World file does not contain a room called '%s' which is used\nas a starting point!\n",
	  strt);
      cleanup ();
      fatal_exit (0, false);
    }
  }

  // Initialize collision detection system (even if disabled so that we can enable it later).
  Sys->InitWorld(Sys->world,Sys->view->GetCamera());
//  csBeing::InitWorld (Sys->world, Sys->view->GetCamera ());

  Sys->Printf (MSG_INITIALIZATION, "--------------------------------------\n");

  // Wait one second before starting.
  long t = Sys->Time ()+1000;
  while (Sys->Time () < t) ;

  // Reinit console object for 3D engine use.
  ((SimpleConsole *)System->Console)->SetupColors (txtmgr);
  ((SimpleConsole *)System->Console)->SetMaxLines ();
  ((SimpleConsole *)System->Console)->SetTransparent ();
  System->Console->Clear ();

  // Initialize our 3D view.
  Sys->view->SetSector (room);
  Sys->view->GetCamera ()->SetPosition (world->start_vec);
  Sys->view->SetRectangle (2, 2, FRAME_WIDTH - 4, FRAME_HEIGHT - 4);

  // Stop the demo.
  stop_demo ();

  // Allocate the palette as calculated by the texture manager.
  txtmgr->AllocPalette ();

  // Create a wireframe object which will be used for debugging.
  CHK (Sys->wf = new csWireFrameCam (txtmgr));

  // Load a few sounds.
#ifdef DO_SOUND
  csSoundBuffer* w = csSoundBufferObject::GetSound(*world, "tada.wav");
  if (w) Sys->piSound->PlayEphemeral (w);

  Sys->wMissile_boom = csSoundBufferObject::GetSound(*world, "boom.wav");
  Sys->wMissile_whoosh = csSoundBufferObject::GetSound(*world, "whoosh.wav");
#endif

  // Start the 'autoexec.cfg' script and fully execute it.
  Command::start_script ("autoexec.cfg");
  char cmd_buf[512];
  while (Command::get_script_line (cmd_buf, 511))
    Command::perform_line (cmd_buf);

  // If there is another script given on the commandline
  // start it but do not execute it yet. This will be done
  // frame by frame.
  if (Sys->auto_script)
    Command::start_script (Sys->auto_script);

  //TestFrustrum ();
  // The main loop.
  Sys->Loop ();

  cleanup ();

  return 1;
}
