/*
    Copyright (C) 2001 by Jorrit Tyberghein

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
#include "cssys/sysfunc.h"
#include "iutil/vfs.h"
#include "csutil/cscolor.h"
#include "cstool/csview.h"
#include "cstool/initapp.h"
#include "simpcd.h"
#include "iutil/eventq.h"
#include "iutil/event.h"
#include "iutil/objreg.h"
#include "iutil/csinput.h"
#include "iutil/virtclk.h"
#include "iengine/sector.h"
#include "iengine/engine.h"
#include "iengine/camera.h"
#include "iengine/light.h"
#include "iengine/statlght.h"
#include "iengine/texture.h"
#include "iengine/mesh.h"
#include "iengine/movable.h"
#include "iengine/material.h"
#include "imesh/thing/polygon.h"
#include "imesh/thing/thing.h"
#include "imesh/sprite3d.h"
#include "imesh/object.h"
#include "ivideo/graph3d.h"
#include "ivideo/graph2d.h"
#include "ivideo/txtmgr.h"
#include "ivideo/texture.h"
#include "ivideo/material.h"
#include "ivideo/fontserv.h"
#include "igraphic/imageio.h"
#include "imap/parser.h"
#include "ivaria/reporter.h"
#include "ivaria/stdrep.h"
#include "ivaria/collider.h"
#include "ivaria/polymesh.h"
#include "cstool/collider.h"
#include "csutil/cmdhelp.h"

CS_IMPLEMENT_PLATFORM_APPLICATION

//-----------------------------------------------------------------------------

// The global pointer to simple
Simple *simple;

Simple::Simple ()
{
  engine = NULL;
  loader = NULL;
  g3d = NULL;
  kbd = NULL;
  vc = NULL;
  view = NULL;
  cdsys = NULL;
  parent_sprite = NULL;
  sprite1 = NULL;
  sprite2 = NULL;
  rot1_direction = 1;
  rot2_direction = -1;
}

Simple::~Simple ()
{
  if (parent_sprite) parent_sprite->DecRef ();
  if (vc) vc->DecRef ();
  if (engine) engine->DecRef ();
  if (loader) loader->DecRef();
  if (g3d) g3d->DecRef ();
  if (kbd) kbd->DecRef ();
  if (view) view->DecRef ();
  if (cdsys) cdsys->DecRef ();
  csInitializer::DestroyApplication (object_reg);
}

void Simple::SetupFrame ()
{
  // First get elapsed time from the virtual clock.
  csTicks elapsed_time = vc->GetElapsedTicks ();

  // Now rotate the camera according to keyboard state
  float speed = (elapsed_time / 1000.0) * (0.03 * 20);

  //---------
  // First rotate the entire mesh. Rotation of the children
  // is independent from this.
  //---------
  csZRotMatrix3 rotmat (speed / 5);
  parent_sprite->GetMovable ()->Transform (rotmat);
  parent_sprite->GetMovable ()->UpdateMove ();

  //---------
  // Rotate the two sprites depending on elapsed time.
  // Remember the old transforms so that we can restore it later if there
  // was a collision.
  //---------
  csZRotMatrix3 rotmat1 (rot1_direction * speed / 2.5);
  csReversibleTransform old_trans1 = sprite1->GetMovable ()->GetTransform ();
  sprite1->GetMovable ()->Transform (rotmat1);
  sprite1->GetMovable ()->UpdateMove ();
  csZRotMatrix3 rotmat2 (rot2_direction * speed / 1.0);
  csReversibleTransform old_trans2 = sprite2->GetMovable ()->GetTransform ();
  sprite2->GetMovable ()->Transform (rotmat2);
  sprite2->GetMovable ()->UpdateMove ();

  //---------
  // Check for collision between the two children.
  // If there is a collision we undo our transforms of the children and
  // reverse rotation direction for both.
  // Important note! We use GetFullTransform() here because we want
  // to check collision based on the real position of the objects and
  // not the relative positions (which is what GetTransform() would return).
  // But the transform that we remembered for restoration later is the
  // one returned from GetTransform() since there is no equivalent
  // SetFullTransform().
  //---------
  cdsys->ResetCollisionPairs ();
  csReversibleTransform ft1 = sprite1->GetMovable ()->GetFullTransform ();
  csReversibleTransform ft2 = sprite2->GetMovable ()->GetFullTransform ();
  bool cd = cdsys->Collide (sprite1_col, &ft1, sprite2_col, &ft2);
  if (cd)
  {
    // Restore old transforms and reverse turning directions.
    sprite1->GetMovable ()->SetTransform (old_trans1);
    sprite1->GetMovable ()->UpdateMove ();
    sprite2->GetMovable ()->SetTransform (old_trans2);
    sprite2->GetMovable ()->UpdateMove ();
    rot1_direction = -rot1_direction;
    rot2_direction = -rot2_direction;
  }
  parent_sprite->DeferUpdateLighting (CS_NLIGHT_STATIC|CS_NLIGHT_DYNAMIC, 10);

  iCamera* c = view->GetCamera();
  if (kbd->GetKeyState (CSKEY_RIGHT))
    c->GetTransform ().RotateThis (CS_VEC_ROT_RIGHT, speed);
  if (kbd->GetKeyState (CSKEY_LEFT))
    c->GetTransform ().RotateThis (CS_VEC_ROT_LEFT, speed);
  if (kbd->GetKeyState (CSKEY_PGUP))
    c->GetTransform ().RotateThis (CS_VEC_TILT_UP, speed);
  if (kbd->GetKeyState (CSKEY_PGDN))
    c->GetTransform ().RotateThis (CS_VEC_TILT_DOWN, speed);
  if (kbd->GetKeyState (CSKEY_UP))
    c->Move (CS_VEC_FORWARD * 4 * speed);
  if (kbd->GetKeyState (CSKEY_DOWN))
    c->Move (CS_VEC_BACKWARD * 4 * speed);

  // Tell 3D driver we're going to display 3D things.
  if (!g3d->BeginDraw (engine->GetBeginDrawFlags () | CSDRAW_3DGRAPHICS))
    return;

  // Tell the camera to render into the frame buffer.
  view->Draw ();
}

void Simple::FinishFrame ()
{
  g3d->FinishDraw ();
  g3d->Print (NULL);
}

bool Simple::HandleEvent (iEvent& ev)
{
  if (ev.Type == csevBroadcast && ev.Command.Code == cscmdProcess)
  {
    simple->SetupFrame ();
    return true;
  }
  else if (ev.Type == csevBroadcast && ev.Command.Code == cscmdFinalProcess)
  {
    simple->FinishFrame ();
    return true;
  }
  else if (ev.Type == csevKeyDown && ev.Key.Code == CSKEY_ESC)
  {
    iEventQueue* q = CS_QUERY_REGISTRY (object_reg, iEventQueue);
    if (q)
    {
      q->GetEventOutlet()->Broadcast (cscmdQuit);
      q->DecRef ();
    }
    return true;
  }

  return false;
}

bool Simple::SimpleEventHandler (iEvent& ev)
{
  return simple->HandleEvent (ev);
}

iCollider* Simple::InitCollider (iMeshWrapper* mesh)
{
  iPolygonMesh* polmesh = SCF_QUERY_INTERFACE (mesh->GetMeshObject (),
  	iPolygonMesh);
  if (polmesh)
  {
    csColliderWrapper* wrap = new csColliderWrapper
    	(mesh->QueryObject (), cdsys, polmesh);
    polmesh->DecRef ();
    return wrap->GetCollider ();
  }
  else
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
	"Object doesn't support collision detection!");
    return NULL;
  }
}

bool Simple::Initialize (int argc, const char* const argv[])
{
  object_reg = csInitializer::CreateEnvironment (argc, argv);
  if (!object_reg) return false;

  if (!csInitializer::RequestPlugins (object_reg,
  	CS_REQUEST_VFS,
	CS_REQUEST_SOFTWARE3D,
	CS_REQUEST_ENGINE,
	CS_REQUEST_FONTSERVER,
	CS_REQUEST_IMAGELOADER,
	CS_REQUEST_LEVELLOADER,
	CS_REQUEST_REPORTER,
	CS_REQUEST_REPORTERLISTENER,
	CS_REQUEST_PLUGIN("crystalspace.collisiondetection.rapid",
		iCollideSystem),
	CS_REQUEST_END))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
	"Can't initialize plugins!");
    return false;
  }

  if (!csInitializer::SetupEventHandler (object_reg, SimpleEventHandler))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
	"Can't initialize event handler!");
    return false;
  }

  // Check for commandline help.
  if (csCommandLineHelper::CheckHelp (object_reg))
  {
    csCommandLineHelper::Help (object_reg);
    return false;
  }

  // The collision detection system.
  cdsys = CS_QUERY_REGISTRY (object_reg, iCollideSystem);
  if (!cdsys)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
	"Can't find the collision detection system!");
    return false;
  }

  // The virtual clock.
  vc = CS_QUERY_REGISTRY (object_reg, iVirtualClock);
  if (!vc)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
	"Can't find the virtual clock!");
    return false;
  }

  // Find the pointer to engine plugin
  engine = CS_QUERY_REGISTRY (object_reg, iEngine);
  if (!engine)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
	"No iEngine plugin!");
    return false;
  }

  loader = CS_QUERY_REGISTRY (object_reg, iLoader);
  if (!loader)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
    	"No iLoader plugin!");
    return false;
  }

  g3d = CS_QUERY_REGISTRY (object_reg, iGraphics3D);
  if (!g3d)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
    	"No iGraphics3D plugin!");
    return false;
  }

  kbd = CS_QUERY_REGISTRY (object_reg, iKeyboardDriver);
  if (!kbd)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
    	"No iKeyboardDriver plugin!");
    return false;
  }

  // Open the main system. This will open all the previously loaded plug-ins.
  if (!csInitializer::OpenApplication (object_reg))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
    	"Error opening system!");
    return false;
  }

  // First disable the lighting cache. Our app is simple enough
  // not to need this.
  engine->SetLightingCacheMode (0);

  if (!loader->LoadTexture ("stone", "/lib/std/stone4.gif"))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
    	"Error loading 'stone4' texture!");
    return false;
  }
  iMaterialWrapper* tm = engine->GetMaterialList ()->FindByName ("stone");

  room = engine->CreateSector ("room");
  iMeshWrapper* walls = engine->CreateSectorWallsMesh (room, "walls");
  iThingState* walls_state = SCF_QUERY_INTERFACE (walls->GetMeshObject (),
  	iThingState);
  iPolygon3D* p;
  p = walls_state->CreatePolygon ();
  p->SetMaterial (tm);
  p->CreateVertex (csVector3 (-5, 0, 5));
  p->CreateVertex (csVector3 (5, 0, 5));
  p->CreateVertex (csVector3 (5, 0, -5));
  p->CreateVertex (csVector3 (-5, 0, -5));
  p->SetTextureSpace (p->GetVertex (0), p->GetVertex (1), 3);

  p = walls_state->CreatePolygon ();
  p->SetMaterial (tm);
  p->CreateVertex (csVector3 (-5, 20, -5));
  p->CreateVertex (csVector3 (5, 20, -5));
  p->CreateVertex (csVector3 (5, 20, 5));
  p->CreateVertex (csVector3 (-5, 20, 5));
  p->SetTextureSpace (p->GetVertex (0), p->GetVertex (1), 3);

  p = walls_state->CreatePolygon ();
  p->SetMaterial (tm);
  p->CreateVertex (csVector3 (-5, 20, 5));
  p->CreateVertex (csVector3 (5, 20, 5));
  p->CreateVertex (csVector3 (5, 0, 5));
  p->CreateVertex (csVector3 (-5, 0, 5));
  p->SetTextureSpace (p->GetVertex (0), p->GetVertex (1), 3);

  p = walls_state->CreatePolygon ();
  p->SetMaterial (tm);
  p->CreateVertex (csVector3 (5, 20, 5));
  p->CreateVertex (csVector3 (5, 20, -5));
  p->CreateVertex (csVector3 (5, 0, -5));
  p->CreateVertex (csVector3 (5, 0, 5));
  p->SetTextureSpace (p->GetVertex (0), p->GetVertex (1), 3);

  p = walls_state->CreatePolygon ();
  p->SetMaterial (tm);
  p->CreateVertex (csVector3 (-5, 20, -5));
  p->CreateVertex (csVector3 (-5, 20, 5));
  p->CreateVertex (csVector3 (-5, 0, 5));
  p->CreateVertex (csVector3 (-5, 0, -5));
  p->SetTextureSpace (p->GetVertex (0), p->GetVertex (1), 3);

  p = walls_state->CreatePolygon ();
  p->SetMaterial (tm);
  p->CreateVertex (csVector3 (5, 20, -5));
  p->CreateVertex (csVector3 (-5, 20, -5));
  p->CreateVertex (csVector3 (-5, 0, -5));
  p->CreateVertex (csVector3 (5, 0, -5));
  p->SetTextureSpace (p->GetVertex (0), p->GetVertex (1), 3);

  walls_state->DecRef ();
  walls->DecRef ();

  iStatLight* light;
  iLightList* ll = room->GetLights ();

  light = engine->CreateLight (NULL, csVector3 (-3, 5, 0), 10,
  	csColor (1, 0, 0), false);
  ll->Add (light->QueryLight ());
  light->DecRef ();

  light = engine->CreateLight (NULL, csVector3 (3, 5,  0), 10,
  	csColor (0, 0, 1), false);
  ll->Add (light->QueryLight ());
  light->DecRef ();

  light = engine->CreateLight (NULL, csVector3 (0, 5, -3), 10,
  	csColor (0, 1, 0), false);
  ll->Add (light->QueryLight ());
  light->DecRef ();

  engine->Prepare ();

  view = new csView (engine, g3d);
  view->GetCamera ()->SetSector (room);
  view->GetCamera ()->GetTransform ().SetOrigin (csVector3 (0, 5, -6));
  iGraphics2D* g2d = g3d->GetDriver2D ();
  view->SetRectangle (0, 0, g2d->GetWidth (), g2d->GetHeight ());

  iTextureManager* txtmgr = g3d->GetTextureManager ();
  txtmgr->SetPalette ();

  // Load a texture for our sprite.
  iTextureWrapper* txt = loader->LoadTexture ("spark",
  	"/lib/std/spark.png", CS_TEXTURE_3D, txtmgr, true);
  if (txt == NULL)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
    	"Error loading texture!");
    return false;
  }

  //---------
  // Load a sprite template from disk.
  //---------
  iMeshFactoryWrapper* imeshfact = loader->LoadMeshObjectFactory (
  	"/lib/std/sprite2");
  if (imeshfact == NULL)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
    	"crystalspace.application.simpcd",
    	"Error loading mesh object factory!");
    return false;
  }

  //---------
  // Here we create a hierarchical mesh object made from three sprites.
  // There is one 'anchor' (parent_sprite) which has two children
  // ('sprite1' and 'sprite2'). Later on we will rotate the two children
  // and also rotate the entire hierarchical mesh.
  //---------
  iSprite3DState* spstate;

  // First create the parent sprite.
  parent_sprite = engine->CreateMeshWrapper (
  	imeshfact, "Parent", room,
	csVector3 (0, 5, 3.5));
  spstate = SCF_QUERY_INTERFACE (parent_sprite->GetMeshObject (),
  	iSprite3DState);
  spstate->SetAction ("default");
  spstate->DecRef ();
  parent_sprite->DeferUpdateLighting (CS_NLIGHT_STATIC|CS_NLIGHT_DYNAMIC, 10);
  parent_sprite->GetMovable ()->Transform (csZRotMatrix3 (PI/2.));
  parent_sprite->GetMovable ()->UpdateMove ();

  // Now create the first child.
  sprite1 = engine->CreateMeshWrapper (imeshfact, "Rotater1");
  sprite1->GetMovable ()->SetPosition (csVector3 (0, -.5, -.5));
  sprite1->GetMovable ()->Transform (csZRotMatrix3 (PI/2.));
  sprite1->GetMovable ()->UpdateMove ();
  spstate = SCF_QUERY_INTERFACE (sprite1->GetMeshObject (), iSprite3DState);
  spstate->SetAction ("default");
  spstate->DecRef ();
  parent_sprite->GetChildren ()->Add (sprite1);
  sprite1->DecRef ();

  // Now create the second child.
  sprite2 = engine->CreateMeshWrapper (imeshfact, "Rotater2");
  sprite2->GetMovable ()->SetPosition (csVector3 (0, .5, -.5));
  sprite2->GetMovable ()->Transform (csZRotMatrix3 (PI/2.));
  sprite2->GetMovable ()->UpdateMove ();
  spstate = SCF_QUERY_INTERFACE (sprite2->GetMeshObject (), iSprite3DState);
  spstate->SetAction ("default");
  spstate->DecRef ();
  parent_sprite->GetChildren ()->Add (sprite2);
  sprite2->DecRef ();

  imeshfact->DecRef ();

  //---------
  // We only do collision detection for the rotating children
  // so that's the only colliders we have to create.
  //---------
  sprite1_col = InitCollider (sprite1);
  if (!sprite1_col) return false;
  sprite2_col = InitCollider (sprite2);
  if (!sprite2_col) return false;

  return true;
}

void Simple::Start ()
{
  csDefaultRunLoop (object_reg);
}

/*---------------------------------------------------------------------*
 * Main function
 *---------------------------------------------------------------------*/
int main (int argc, char* argv[])
{
  simple = new Simple ();

  if (simple->Initialize (argc, argv))
    simple->Start ();

  delete simple;
  return 0;
}
