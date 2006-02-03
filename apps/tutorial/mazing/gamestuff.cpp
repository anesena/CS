/*
    Copyright (C) 2006 by Jorrit Tyberghein

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

#include <cssysdef.h>
#include <crystalspace.h>
#include "appmazing.h"
#include "gamestuff.h"

//-----------------------------------------------------------------------------

Maze::Maze (AppMazing* app)
{
  Maze::app = app;
  int x, y, z;
  for (x = 0 ; x < MAZE_DIMENSION ; x++)
    for (y = 0 ; y < MAZE_DIMENSION ; y++)
      for (z = 0 ; z < MAZE_DIMENSION ; z++)
        occupied[x][y][z] = false;
}

bool Maze::CreateSector (int x, int y, int z)
{
  CS_ASSERT (RoomCoordinate::IsValid (x, y, z));
  char sector_name[100];
  sprintf (sector_name, "room_%d_%d_%d", x, y, z);
  rooms[x][y][z] = app->GetEngine ()->CreateSector (sector_name);
  return rooms[x][y][z] != 0;
}

void Maze::MakeConnection (const RoomCoordinate& from, const RoomCoordinate& to)
{
  CS_ASSERT (from.IsValid ());
  CS_ASSERT (to.IsValid ());
  connections[from.x][from.y][from.z].Push (to);
}

void Maze::FreeSpace (const RoomCoordinate& rc)
{
  CS_ASSERT (rc.IsValid ());
  occupied[rc.x][rc.y][rc.z] = false;
}

void Maze::OccupySpace (const RoomCoordinate& rc)
{
  CS_ASSERT (rc.IsValid ());
  occupied[rc.x][rc.y][rc.z] = true;
}

bool Maze::IsSpaceFree (const RoomCoordinate& rc) const
{
  CS_ASSERT (rc.IsValid ());
  return !occupied[rc.x][rc.y][rc.z];
}

bool Maze::CreateWallOrPortal (iThingFactoryState* factory_state,
  	const csVector3& v1, const csVector3& v2,
  	const csVector3& v3, const csVector3& v4,
	bool do_portal,
	const RoomCoordinate& source,
	const RoomCoordinate& dest)
{
  if (do_portal)
  {
    iPortal* portal;
    csVector3 verts[4];
    verts[0] = v1;
    verts[1] = v2;
    verts[2] = v3;
    verts[3] = v4;
    csRef<iMeshWrapper> portal_mesh = app->GetEngine ()->CreatePortal (0,
    	GetSector (source), csVector3 (0, 0, 0),
	GetSector (dest),
    	verts, 4, portal);
    MakeConnection (source, dest);
    if (!portal_mesh)
      return app->ReportError ("Error creating portal mesh!");
  }
  else
  {
    factory_state->AddQuad (v1, v2, v3, v4);
  }
  return true;
}

bool Maze::CreateRoom (iMaterialWrapper* wall_material,
	int x, int y, int z, char* portals)
{
  iSector* room = GetSector (x, y, z);
  csRef<iMeshWrapper> walls = app->GetEngine ()
  	->CreateSectorWallsMesh (room, "walls");
  if (!walls)
    return app->ReportError ("Couldn't create the walls for the room!");

  csRef<iThingState> object_state = scfQueryInterface<iThingState> (
  	walls->GetMeshObject ());
  csRef<iThingFactoryState> factory_state = scfQueryInterface<
  	iThingFactoryState> (walls->GetMeshObject ()->GetFactory ());
  float sx = float (x) * ROOM_DIMENSION;
  float sy = float (y) * ROOM_DIMENSION;
  float sz = float (z) * ROOM_DIMENSION;
  float rd = ROOM_DIMENSION / 2.0;
  csBox3 box (
  	csVector3 (sx-rd, sy-rd, sz-rd),
  	csVector3 (sx+rd, sy+rd, sz+rd));

  RoomCoordinate start_rc (x, y, z);
  if (!CreateWallOrPortal (factory_state, 
    	box.GetCorner (CS_BOX_CORNER_xYz),
    	box.GetCorner (CS_BOX_CORNER_XYz),
    	box.GetCorner (CS_BOX_CORNER_XYZ),
    	box.GetCorner (CS_BOX_CORNER_xYZ),
	portals[PORTAL_UP] == '#',
	start_rc, RoomCoordinate (x+0, y+1, z+0)))
    return false;
  if (!CreateWallOrPortal (factory_state,
    	box.GetCorner (CS_BOX_CORNER_xyZ),
    	box.GetCorner (CS_BOX_CORNER_XyZ),
    	box.GetCorner (CS_BOX_CORNER_Xyz),
    	box.GetCorner (CS_BOX_CORNER_xyz),
	portals[PORTAL_DOWN] == '#',
	start_rc, RoomCoordinate (x+0, y-1, z+0)))
    return false;
  if (!CreateWallOrPortal (factory_state,
    	box.GetCorner (CS_BOX_CORNER_XyZ),
    	box.GetCorner (CS_BOX_CORNER_xyZ),
    	box.GetCorner (CS_BOX_CORNER_xYZ),
    	box.GetCorner (CS_BOX_CORNER_XYZ),
	portals[PORTAL_FRONT] == '#',
	start_rc, RoomCoordinate (x+0, y+0, z+1)))
    return false;
  if (!CreateWallOrPortal (factory_state,
    	box.GetCorner (CS_BOX_CORNER_xyz),
    	box.GetCorner (CS_BOX_CORNER_Xyz),
    	box.GetCorner (CS_BOX_CORNER_XYz),
    	box.GetCorner (CS_BOX_CORNER_xYz),
	portals[PORTAL_BACK] == '#',
	start_rc, RoomCoordinate (x+0, y+0, z-1)))
    return false;
  if (!CreateWallOrPortal (factory_state,
    	box.GetCorner (CS_BOX_CORNER_xyZ),
    	box.GetCorner (CS_BOX_CORNER_xyz),
    	box.GetCorner (CS_BOX_CORNER_xYz),
    	box.GetCorner (CS_BOX_CORNER_xYZ),
	portals[PORTAL_LEFT] == '#',
	start_rc, RoomCoordinate (x-1, y+0, z+0)))
    return false;
  if (!CreateWallOrPortal (factory_state,
    	box.GetCorner (CS_BOX_CORNER_Xyz),
    	box.GetCorner (CS_BOX_CORNER_XyZ),
    	box.GetCorner (CS_BOX_CORNER_XYZ),
    	box.GetCorner (CS_BOX_CORNER_XYz),
	portals[PORTAL_RIGHT] == '#',
	start_rc, RoomCoordinate (x+1, y+0, z+0)))
    return false;

  factory_state->SetPolygonMaterial (CS_POLYRANGE_ALL, wall_material);
  factory_state->SetPolygonTextureMapping (CS_POLYRANGE_ALL, rd);

  return true;
}

bool Maze::CreateLight (const csColor& color,
  	int x, int y, int z)
{
  float sx = float (x) * ROOM_DIMENSION;
  float sy = float (y) * ROOM_DIMENSION;
  float sz = float (z) * ROOM_DIMENSION;
  csRef<iLight> light = app->GetEngine ()
  	->CreateLight (0, csVector3 (sx, sy, sz), ROOM_DIMENSION * 1.5, color);
  if (!light) return false;
  GetSector (x, y, z)->GetLights ()->Add (light);
  return true;
}

bool Maze::CreateGeometry ()
{
  // We don't need a lighting cache. Disable it.
  app->GetEngine ()->SetLightingCacheMode (0);

  // Load the texture we are going to use for all walls.
  if (!app->GetLoader ()->LoadTexture ("wall_texture", "/lib/std/stone4.gif"))
    return app->ReportError ("Error loading 'stone4' texture!");

  iMaterialWrapper* wall_material = app->GetEngine ()->GetMaterialList ()
  	->FindByName ("wall_texture");

  int x, y, z;
  for (x = 0 ; x < MAZE_DIMENSION ; x++)
    for (y = 0 ; y < MAZE_DIMENSION ; y++)
      for (z = 0 ; z < MAZE_DIMENSION ; z++)
        if (!CreateSector (x, y, z))
	  return false;

  if (!CreateRoom (wall_material, 0, 0, 0, "....#.")) return false;
  if (!CreateRoom (wall_material, 0, 0, 1, "....##")) return false;
  if (!CreateRoom (wall_material, 0, 0, 2, "#..#.#")) return false;
  if (!CreateRoom (wall_material, 1, 0, 0, "...##.")) return false;
  if (!CreateRoom (wall_material, 1, 0, 1, "....##")) return false;
  if (!CreateRoom (wall_material, 1, 0, 2, "..#..#")) return false;
  if (!CreateRoom (wall_material, 2, 0, 0, "#.#.#.")) return false;
  if (!CreateRoom (wall_material, 2, 0, 1, "....##")) return false;
  if (!CreateRoom (wall_material, 2, 0, 2, ".....#")) return false;

  if (!CreateRoom (wall_material, 0, 1, 0, "#..#..")) return false;
  if (!CreateRoom (wall_material, 0, 1, 1, "...##.")) return false;
  if (!CreateRoom (wall_material, 0, 1, 2, ".#.#.#")) return false;
  if (!CreateRoom (wall_material, 1, 1, 0, "..#.#.")) return false;
  if (!CreateRoom (wall_material, 1, 1, 1, "#.#..#")) return false;
  if (!CreateRoom (wall_material, 1, 1, 2, "..##..")) return false;
  if (!CreateRoom (wall_material, 2, 1, 0, ".#..#.")) return false;
  if (!CreateRoom (wall_material, 2, 1, 1, "....##")) return false;
  if (!CreateRoom (wall_material, 2, 1, 2, "#.#..#")) return false;

  if (!CreateRoom (wall_material, 0, 2, 0, ".#.#..")) return false;
  if (!CreateRoom (wall_material, 0, 2, 1, "...#..")) return false;
  if (!CreateRoom (wall_material, 0, 2, 2, "...#..")) return false;
  if (!CreateRoom (wall_material, 1, 2, 0, "..##..")) return false;
  if (!CreateRoom (wall_material, 1, 2, 1, ".###..")) return false;
  if (!CreateRoom (wall_material, 1, 2, 2, "..##..")) return false;
  if (!CreateRoom (wall_material, 2, 2, 0, "..#...")) return false;
  if (!CreateRoom (wall_material, 2, 2, 1, "..#...")) return false;
  if (!CreateRoom (wall_material, 2, 2, 2, ".##...")) return false;

  if (!CreateLight (csColor (1, 0, 0), 0, 0, 0)) return false;
  if (!CreateLight (csColor (0, 0, 1), 0, 0, 2)) return false;
  if (!CreateLight (csColor (0, 1, 0), 1, 0, 1)) return false;
  if (!CreateLight (csColor (1, 1, 0), 1, 1, 1)) return false;
  if (!CreateLight (csColor (0, 1, 1), 0, 1, 1)) return false;
  if (!CreateLight (csColor (1, 1, 1), 2, 1, 2)) return false;
  if (!CreateLight (csColor (1, 0, 0), 1, 2, 1)) return false;
  if (!CreateLight (csColor (0, 0, 1), 0, 2, 0)) return false;
  if (!CreateLight (csColor (0, 1, 0), 2, 2, 0)) return false;

  return true;
}

//-----------------------------------------------------------------------------

Player::Player (AppMazing* app)
{
  Player::app = app;

  desired_location.Set (0, 0, 0);
  desired_lookat.Set (0, 0, 1);
  start_location.Set (0, 0, 0);
  start_lookat.Set (0, 0, 1);
  amount_moved = 0;
  amount_rotated = 0;
}

bool Player::InitCollisionDetection ()
{
  float ps = PLAYER_SIZE / 2.0;
  csPolygonMeshBox* box = new csPolygonMeshBox (csBox3 (
  	csVector3 (-ps, -ps, -ps), csVector3 (ps, ps, ps)));
  player_collider = app->GetCollisionDetectionSystem ()
  	->CreateCollider (box);
  box->DecRef ();
  if (player_collider == 0) return false;
  return true;
}

void Player::MoveAndRotateCamera (float elapsed_seconds)
{
  iCamera* camera = app->GetCamera ();

  // First we move the camera.
  amount_moved += MOVECAMERA_SPEED * elapsed_seconds;
  float move_val = 1 - pow (2.0f, -amount_moved);
  csVector3 current_pos = camera->GetTransform ().GetOrigin ();
  csVector3 new_pos = start_location * (1-move_val)
  	+ desired_location * move_val;
  camera->MoveWorld (new_pos - current_pos, false);

  // Now we rotate the camera.
  amount_rotated += ROTATECAMERA_SPEED * elapsed_seconds;
  float rotate_val = 1 - pow (2.0f, -amount_rotated);
  csVector3 new_lookat = start_lookat * (1-rotate_val)
  	+ desired_lookat * rotate_val;
  camera->GetTransform ().LookAt (new_lookat, csVector3 (0, 1, 0));
}

void Player::StartMovement (const csVector3& dir)
{
  iCamera* camera = app->GetCamera ();

  amount_moved = 0;
  csReversibleTransform camera_trans = camera->GetTransform ();
  start_location = camera_trans.GetOrigin ();

  // Calculate the spot where we want to move too depending on elapsed
  // time and current direction the camera is facing.
  csVector3 world_dir = camera_trans.This2OtherRelative (dir);
  desired_location = start_location + MOVE_DISTANCE * world_dir;

  // First we find all meshes that are near the path that we want to move.
  // To do that we take the center point of our movement vector and
  // calculate all objects that touch the circle around that point with
  // radius (MOVE_DISTANCE+PLAYER_SIZE) / 2.0.
  csRef<iMeshWrapperIterator> it = app->GetEngine ()->GetNearbyMeshes (
  	camera->GetSector (), (start_location + desired_location) / 2.0,
	(MOVE_DISTANCE + PLAYER_SIZE) / 2.0);

  if (!it->HasNext ())
  {
    // We have no objects in the iterator so there can be no collision.
    // We can move freely.
    return;
  }

  // If we have meshes then we will calculate collision detection for our
  // object along the path we want to move.
  desired_location = start_location;
  csVector3 test_location = start_location + STEP_DISTANCE * world_dir;
  iCollideSystem* cdsys = app->GetCollisionDetectionSystem ();
  float current_move = STEP_DISTANCE;
  while (current_move <= MOVE_DISTANCE)
  {
    camera_trans.SetOrigin (test_location);

    // Test collision with our iterator.
    it->Reset ();
    while (it->HasNext ())
    {
      iMeshWrapper* mesh = it->Next ();
      csColliderWrapper* collide_wrap = csColliderWrapper::GetColliderWrapper (
    	  mesh->QueryObject ());
      if (collide_wrap)
      {
        csReversibleTransform mesh_trans = mesh->GetMovable ()
      	  ->GetFullTransform ();
	cdsys->ResetCollisionPairs ();
        if (cdsys->Collide (
      	      collide_wrap->GetCollider (), &mesh_trans,
	      player_collider, &camera_trans))
        {
	  // Collision, so we can stop. 'desired_location' will contain
	  // the last valid location that we can move too.
	  return;
	}
      }
    }
    desired_location = test_location;
    test_location += STEP_DISTANCE * world_dir;
    current_move += STEP_DISTANCE;
  }
}

void Player::StartRotation (const csVector3& rot)
{
  float angle = ROTATE_ANGLE;
  csOrthoTransform trans = app->GetCamera ()->GetTransform ();
  start_lookat = trans.This2OtherRelative (csVector3 (0, 0, 1));
  trans.RotateThis (rot, angle);
  desired_lookat = trans.This2OtherRelative (csVector3 (0, 0, 1));
  amount_rotated = 0;
}

//-----------------------------------------------------------------------------

Explosion::Explosion (iMeshWrapper* mesh, int timeleft)
{
  Explosion::mesh = mesh;
  Explosion::timeleft = timeleft;
}

bool Explosion::Handle (csTicks elapsed_ticks)
{
  timeleft -= elapsed_ticks;
  if (timeleft <= 0)
  {
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------

Adversary::Adversary (AppMazing* app,
	iMeshWrapper* mesh, const RoomCoordinate& rc) :
	scfImplementationType (this)
{
  Adversary::app = app;
  Adversary::mesh = mesh;
  current_location = rc;
  moving = false;
}

void Adversary::ThinkAndMove (float elapsed_seconds)
{
  if (!moving)
  {
    const csArray<RoomCoordinate>& connections = app->GetMaze ()
    	->GetConnections (current_location);
    int moveto = (rand () >> 3) % connections.Length ();
    if (app->GetMaze ()->IsSpaceFree (connections[moveto]))
    {
      start.x = float (current_location.x) * ROOM_DIMENSION;
      start.y = float (current_location.y) * ROOM_DIMENSION;
      start.z = float (current_location.z) * ROOM_DIMENSION;
      app->GetMaze ()->FreeSpace (current_location);
      current_location = connections[moveto];
      app->GetMaze ()->OccupySpace (current_location);
      end.x = float (current_location.x) * ROOM_DIMENSION;
      end.y = float (current_location.y) * ROOM_DIMENSION;
      end.z = float (current_location.z) * ROOM_DIMENSION;
      remaining_seconds = ADVERSARY_MOVETIME;
      moving = true;
    }
  }
  else
  {
    remaining_seconds -= elapsed_seconds;
    csVector3 new_pos;
    if (remaining_seconds <= 0)
    {
      moving = false;
      new_pos = end;
    }
    else
    {
      csMath3::Between (end, start, new_pos,
      	100.0 * remaining_seconds / ADVERSARY_MOVETIME, 0);
    }
    iMovable* movable = mesh->GetMovable ();
    iSector* old_sector = movable->GetSectors ()->Get (0);
    bool mirror;
    iSector* new_sector = old_sector->FollowSegment (
    	movable->GetTransform (), new_pos, mirror, true);
    movable->SetSector (new_sector);
    movable->GetTransform ().SetOrigin (new_pos);
    movable->UpdateMove ();
  }
}

//-----------------------------------------------------------------------------

Laser::Laser (AppMazing* app)
{
  Laser::app = app;
  lasertime = 0;
}

void Laser::Start ()
{
  if (lasertime > 0) return;	// Laser already in progress.
  lasertime = LASER_LIFETIME;
  const csOrthoTransform& tr = app->GetCamera ()->GetTransform ();
  laserstart = tr.This2Other (LASER_OFFSET);
  laserend = tr.This2Other (csVector3 (0, 0, 20.0));
  lasersector = app->GetCamera ()->GetSector ();

  // Fire up our beam.
  laserbeam->GetMovable ()->SetPosition (lasersector, laserstart);
  laserbeam->GetMovable ()->GetTransform ().LookAt (
  	laserend-tr.GetOrigin (), csVector3 (0, 1, 0));
  laserbeam->GetMovable ()->UpdateMove ();
}

void Laser::Handle (csTicks ticks)
{
  if (lasertime <= 0) return;
  lasertime -= ticks;
  if (lasertime <= 0)
  {
    // Time to stop the laser.
    lasertime = 0;
    laserbeam->GetMovable ()->ClearSectors ();
    laserbeam->GetMovable ()->UpdateMove ();
  }
  else
  {
    int flick = (lasertime / LASER_FLICKTIME) & 1;
    Check ();
    csRef<iGeneralMeshState> state = scfQueryInterface<iGeneralMeshState> (
    	laserbeam->GetMeshObject ());
    state->SetColor (flick ? csColor (2.0, 2.0, 2.0) : csColor (.5, .5, .5));
  }
}

void Laser::Check ()
{
  // Do a beam to check if we hit an adversary.
  csVector3 isect;
  iMeshWrapper* mesh = lasersector->HitBeamPortals (laserstart,
  	laserend, isect, 0);
  if (mesh)
  {
    csRef<Adversary> adv = CS_GET_CHILD_OBJECT (
    	mesh->QueryObject (), Adversary);
    if (adv)
    {
      // Hit!
      app->ExplodeAdversary (adv);
    }
  }
}

//-----------------------------------------------------------------------------
