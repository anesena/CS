/*
    Copyright (C) 1998-2001 by Jorrit Tyberghein
    csObject library (C) 1999 by Ivan Avramovic <ivan@avramovic.com>
  
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

#ifndef __CSOBJ_H__
#define __CSOBJ_H__

#include "cstypes.h"
#include "csutil/csbase.h"
#include "csutil/util.h"
#include "iobject/object.h"

class csObjContainer;

/**
 * A generic csObject class. Any csObject can have any number of iObject
 * children attached to it. You can use QUERY_INTERFACE to get interfaces
 * from the child objects.
 */ 
class csObject : public csBase, public iObject
{
protected:
  friend class csObjIterator;
  friend class csObjectIterator;
  friend class csObjectNoDel;
  /// Each object have a unique ID associated with it
  CS_ID csid;

  /// The array of child nodes
  csObjContainer *children;

  /// Object's name or NULL if unnamed.
  char *Name;

  /// Set the parent csObject. Implemented in csPObject class.
  virtual void SetObjectParent (csObject *)
  { }

public:
  /// Initialize the csObject
  csObject ();
  /// Make a copy of another object
  csObject (csObject& iObj);
  /// Destroy this object and the associated children
  virtual ~csObject ();

  /// Set object name
  virtual void SetName (const char *iName)
  { delete [] Name; Name = strnew (iName); }

  /// Query object name
  virtual const char *GetName () const
  { return Name; }

  /// Get the unique ID associated with this object
  CS_ID GetID () const
  { return csid; }

  /// Returns the parent csObject. Implemented in csPObject class.
  virtual csObject* GetObjectParent () const
  { return NULL; }

  /**
   * Returns the parent csObject. Implemented in csPObject class.
   * @@@ Ugly name!
   */
  virtual iObject* GetObjectParentI () const
  { return QUERY_INTERFACE_SAFE (GetObjectParent (), iObject); }

  /// Attach a new csObject to the tree
  void ObjAdd (csObject *obj);

  /// Attach a new iObject to the tree
  virtual void ObjAdd (iObject *obj);

  /// Removes the given object from the tree, without freeing the contents
  void ObjRelease (csObject *obj);

  /// Removes the given object from the tree, without freeing the contents
  virtual void ObjRelease (iObject *obj);

  /// Deletes the given object, removing it from the object tree
  void ObjRemove (csObject *obj);

  /// Deletes the given object, removing it from the object tree
  virtual void ObjRemove (iObject *obj);

  /// Removes all objects from the tree, without freeing the contents
  virtual void ObjReleaseAll ();

  /// Deletes all objects, removing them from the object tree
  virtual void ObjRemoveAll ();

  /**
   * Look for a child object that implements the given interface. You can
   * optionally pass a name to look for. If FirstName is true then the
   * method will stop at the first object with the requested name, even
   * if it did not implement the requested type. Note that the returned
   * object may only be cast to the requested type, no other type, not
   * even iObject! <p>
   * 
   * Note that the returned object will be IncRef'ed.
   */
  virtual void* GetChild (int iInterfaceID, int iVersion,
    const char *Name = NULL, bool FirstName = false) const;

  /// Return the first child object with the given name
  virtual iObject *GetChild (const char *Name) const;

  /**
   * Return an iterator for all child objects. Note that you should not
   * remove child objects while iterating.
   */
  virtual iObjectIterator *GetIterator ();

  DECLARE_IBASE;
};

/**
 * A small modification to csObject that does not delete the
 * contained children objects in the destructor
 */
class csObjectNoDel : public csObject
{
public:
  /// Create the object
  csObjectNoDel () : csObject () {}
  /// Free the memory allocated for children but do not delete them
  virtual ~csObjectNoDel ();
};

#endif // __CSOBJ_H__
