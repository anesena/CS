/*
    Copyright (C) 2000-2001 by Jorrit Tyberghein
    Copyright (C) 1998-2000 by Ivan Avramovic <ivan@avramovic.com>

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
#include "csloader.h"
#include "imap/reader.h"

struct csLoaderPluginRec
{
  char* ShortName;
  char* ClassID;
  csRef<iComponent> Component;
  csRef<iLoaderPlugin> Plugin;
  csRef<iBinaryLoaderPlugin> BinPlugin;

  csLoaderPluginRec (const char* shortName,
	const char *classID,
	iComponent* component,
	iLoaderPlugin *plugin,
	iBinaryLoaderPlugin* binPlugin)
  {
    if (shortName) ShortName = csStrNew (shortName);
    else ShortName = NULL;
    ClassID = csStrNew (classID);
    Component = component;
    Plugin = plugin;
    BinPlugin = binPlugin;
  }

  ~csLoaderPluginRec ()
  {
    delete [] ShortName;
    delete [] ClassID;
  }
};

csLoader::csLoadedPluginVector::csLoadedPluginVector ()
{
  plugin_mgr = NULL;
  mutex = csMutex::Create (true);
}

csLoader::csLoadedPluginVector::~csLoadedPluginVector ()
{
  DeleteAll ();
}

void csLoader::csLoadedPluginVector::DeleteAll ()
{
  csScopedMutexLock lock (mutex);
  int i;
  for (i = 0 ; i < vector.Length () ; i++)
  {
    csLoaderPluginRec* rec = vector[i];
    if (rec->Component && plugin_mgr)
    {
      plugin_mgr->UnloadPlugin (rec->Component);
    }
    delete rec;
  }
  vector.DeleteAll ();
}

csLoaderPluginRec* csLoader::csLoadedPluginVector::FindPluginRec (
	const char* name)
{
  csScopedMutexLock lock (mutex);
  int i;
  for (i=0 ; i<vector.Length () ; i++)
  {
    csLoaderPluginRec* pl = vector.Get (i);
    if (pl->ShortName && !strcmp (name, pl->ShortName))
      return pl;
    if (!strcmp (name, pl->ClassID))
      return pl;
  }
  return NULL;
}

bool csLoader::csLoadedPluginVector::GetPluginFromRec (
	csLoaderPluginRec *rec, iLoaderPlugin*& plug,
	iBinaryLoaderPlugin*& binplug)
{
  if (!rec->Component)
  {
    rec->Component = CS_LOAD_PLUGIN (plugin_mgr,
    	rec->ClassID, iComponent);
    if (rec->Component)
    {
      rec->Plugin = SCF_QUERY_INTERFACE (rec->Component, iLoaderPlugin);
      rec->BinPlugin = SCF_QUERY_INTERFACE (rec->Component,
      	iBinaryLoaderPlugin);
    }
  }
  plug = rec->Plugin;
  binplug = rec->BinPlugin;
  return rec->Component != NULL;
}

bool csLoader::csLoadedPluginVector::FindPlugin (
	const char* Name, iLoaderPlugin*& plug,
	iBinaryLoaderPlugin*& binplug)
{
  csScopedMutexLock lock (mutex);
  // look if there is already a loading record for this plugin
  csLoaderPluginRec* pl = FindPluginRec (Name);
  if (pl)
  {
    return GetPluginFromRec (pl, plug, binplug);
  }

  // create a new loading record
  NewPlugin (NULL, Name);
  return GetPluginFromRec (vector.Get(vector.Length()-1),
  	plug, binplug);
}

void csLoader::csLoadedPluginVector::NewPlugin
	(const char *ShortName, const char *ClassID)
{
  csScopedMutexLock lock (mutex);
  vector.Push (new csLoaderPluginRec (ShortName, ClassID, NULL, NULL, NULL));
}

