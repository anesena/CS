/*
    Copyright (C) 2001 by Jorrit Tyberghein
    Copyright (C) 2000 by W.C.A. Wijngaards
  
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

#ifndef __CS_MATERIAL_H__
#define __CS_MATERIAL_H__

#include "csgfx/rgbpixel.h"
#include "csobject/csobject.h"
#include "csobject/nobjvec.h"
#include "ivideo/material.h"
#include "iengine/material.h"

struct iTextureWrapper;
struct iTextureManager;


SCF_VERSION (csMaterialWrapper, 0, 0, 1);

/**
 * A material class.
 */
class csMaterial : public iMaterial
{
private:
  /// flat shading color
  csRGBcolor flat_color;
  /// the texture of the material (can be NULL)
  iTextureWrapper *texture;
  /// Number of texture layers (currently maximum 4).
  int num_texture_layers;
  /// Optional texture layer.
  csTextureLayer texture_layers[4];
  /// Texture wrappers for texture layers.
  iTextureWrapper* texture_layer_wrappers[4];

  /// The diffuse reflection value of the material
  float diffuse;
  /// The ambient lighting of the material
  float ambient;
  /// The reflectiveness of the material
  float reflection;

public:
  /**
   * create an empty material
   */
  csMaterial ();
  /**
   * create a material with only the texture given.
   */
  csMaterial (iTextureWrapper *txt);

  /**
   * destroy material
   */
  virtual ~csMaterial ();

  /// Get the flat shading color
  csRGBcolor& GetFlatColor () { return flat_color; }

  /// Get diffuse reflection constant for the material
  float GetDiffuse () const { return diffuse; }
  /// Set diffuse reflection constant for the material
  void SetDiffuse (float val) { diffuse = val; }

  /// Get ambient lighting for the material
  float GetAmbient () const { return ambient; }
  /// Set ambient lighting for the material
  void SetAmbient (float val) { ambient = val; }

  /// Get reflection of the material
  float GetReflection () const { return reflection; }
  /// Set reflection of the material
  void SetReflection (float val) { reflection = val; }

  /// Get the texture (if none NULL is returned)
  iTextureWrapper *GetTextureWrapper () const { return texture; }
  /// Set the texture (pass NULL to set no texture)
  void SetTextureWrapper (iTextureWrapper *tex);

  /// Add a texture layer (currently only one supported).
  void AddTextureLayer (iTextureWrapper* txtwrap, UInt mode,
      	int uscale, int vscale, int ushift, int vshift);

  //--------------------- iMaterial implementation ---------------------

  /// Get texture.
  virtual iTextureHandle* GetTexture ();
  /// Get num texture layers.
  virtual int GetNumTextureLayers ();
  /// Get a texture layer.
  virtual csTextureLayer* GetTextureLayer (int idx);
  /// Get flat color.
  virtual void GetFlatColor (csRGBpixel &oColor);
  /// Set the flat shading color
  virtual void SetFlatColor (const csRGBcolor& col) { flat_color = col; }
  /// Get reflection values (diffuse, ambient, reflection).
  virtual void GetReflection (float &oDiffuse, float &oAmbient,
    float &oReflection);
  /// Set reflection values (diffuse, ambient, reflection).
  virtual void SetReflection (float oDiffuse, float oAmbient,
    float oReflection)
  {
    diffuse = oDiffuse;
    ambient = oAmbient;
    reflection = oReflection;
  }

  DECLARE_IBASE;
};

/**
 * csMaterialWrapper represents a texture and its link
 * to the iMaterialHandle as returned by iTextureManager.
 */
class csMaterialWrapper : public csObject
{
private:
  /// The corresponding iMaterial.
  iMaterial* material;
  /// The handle as returned by iTextureManager.
  iMaterialHandle* handle;

public:
  /// Construct a material handle given a material.
  csMaterialWrapper (iMaterial* Image);
  /// Construct a csMaterialWrapper from a pre-registered material handle.
  csMaterialWrapper (iMaterialHandle *ith);
  /// Release material handle
  virtual ~csMaterialWrapper ();

  /**
   * Change the material handle. Note: This will also change the base
   * material to NULL.
   */
  void SetMaterialHandle (iMaterialHandle *mat);
  /// Get the material handle.
  iMaterialHandle* GetMaterialHandle () { return handle; }

  /**
   * Change the base material. Note: The changes will not be visible until
   * you re-register the material.
   */
  void SetMaterial (iMaterial* material);
  /// Get the original material.
  iMaterial* GetMaterial () { return material; }

  /// Register the material with the texture manager
  void Register (iTextureManager *txtmng);

  /**
   * Visit this material. This should be called by the engine right
   * before using the material. It will call Visit() on all textures
   * that are used.
   */
  void Visit ();

  DECLARE_IBASE_EXT (csObject);

  //------------------- iMaterialWrapper implementation -----------------------
  struct MaterialWrapper : public iMaterialWrapper
  {
    DECLARE_EMBEDDED_IBASE (csMaterialWrapper);
    virtual csMaterialWrapper* GetPrivateObject ()
    { return (csMaterialWrapper*)scfParent; }
    virtual iObject *QueryObject()
    { return scfParent; }
    virtual void SetMaterialHandle (iMaterialHandle *mat)
    { scfParent->SetMaterialHandle (mat); }
    virtual iMaterialHandle* GetMaterialHandle ()
    { return scfParent->GetMaterialHandle (); }
    virtual void SetMaterial (iMaterial* material)
    { scfParent->SetMaterial (material); }
    virtual iMaterial* GetMaterial ()
    { return scfParent->GetMaterial (); }
    virtual void Register (iTextureManager *txtmng)
    { scfParent->Register (txtmng); }
    virtual void Visit ()
    { scfParent->Visit (); }
  } scfiMaterialWrapper;
};

/**
 * This class is used to hold a list of materials.
 */
class csMaterialList : public csNamedObjVector
{
public:
  /// Initialize the array
  csMaterialList ();
  /// Destroy every material in the list
  virtual ~csMaterialList ();

  /// Create a new material.
  csMaterialWrapper* NewMaterial (iMaterial* material);

  /**
   * Create a engine wrapper for a pre-prepared iTextureHandle
   * The handle will be IncRefed.
   */
  csMaterialWrapper* NewMaterial (iMaterialHandle *ith);

  /// Return material by index
  csMaterialWrapper *Get (int idx)
  { return (csMaterialWrapper *)csNamedObjVector::Get (idx); }

  /// Find a material by name
  csMaterialWrapper *FindByName (const char* iName)
  { return (csMaterialWrapper *)csNamedObjVector::FindByName (iName); }

  DECLARE_IBASE;

  //------------------- iMaterialList implementation -----------------------
  struct MaterialList : public iMaterialList
  {
    DECLARE_EMBEDDED_IBASE (csMaterialList);
    virtual iMaterialWrapper* NewMaterial (iMaterial* material)
    {
      csMaterialWrapper* mw = scfParent->NewMaterial (material);
      if (mw) return &(mw->scfiMaterialWrapper);
      else return NULL;
    }
    virtual iMaterialWrapper* NewMaterial (iMaterialHandle *ith)
    {
      csMaterialWrapper* mw = scfParent->NewMaterial (ith);
      if (mw) return &(mw->scfiMaterialWrapper);
      else return NULL;
    }
    virtual int GetNumMaterials ()
    {
      return scfParent->Length ();
    }
    virtual iMaterialWrapper* Get (int idx)
    {
      return &(scfParent->Get (idx)->scfiMaterialWrapper);
    }
    virtual iMaterialWrapper* FindByName (const char* iName)
    {
      csMaterialWrapper* mw = scfParent->FindByName (iName);
      if (mw) return &(mw->scfiMaterialWrapper);
      else return NULL;
    }
  } scfiMaterialList;
};

#endif // __CS_MATERIAL_H__
