/*
    Copyright (C) 2007 by Marten Svanfeldt

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
#include "cstool/meshfilter.h"

using namespace CS::Utility;

MeshFilter::MeshFilter ()
: filterMode (MESH_FILTER_EXCLUDE)
{

}

MeshFilter::~MeshFilter ()
{
}

void MeshFilter::AddFilterMesh (iMeshWrapper* mesh)
{
  filteredMeshes.Add (mesh);
}

void MeshFilter::RemoveFilterMesh (iMeshWrapper* mesh)
{
  filteredMeshes.Delete (mesh);
}

bool MeshFilter::IsMeshFiltered (iMeshWrapper* mesh) const
{
  if (filterMode == MESH_FILTER_EXCLUDE)
  {
    return filteredMeshes.Contains (mesh);
  }
  else
  {
    return !filteredMeshes.Contains (mesh);
  }
}
