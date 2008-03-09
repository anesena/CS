/*
    Copyright (C) 1998-2001 by Jorrit Tyberghein

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

#ifndef __CS_CAMERA_H__
#define __CS_CAMERA_H__

#include "csgeom/transfrm.h"
#include "csutil/refarr.h"
#include "csutil/scf.h"
#include "iengine/camera.h"
#include "iengine/scenenode.h"

#include "sector.h"
#include "movable.h"

class csEngine;

CS_PLUGIN_NAMESPACE_BEGIN(Engine)
{

#include "csutil/deprecated_warn_off.h"

/**
 * A camera positioned in the 3D world.
 */
class csCameraBase : public csOrthoTransform, 
                     public scfImplementation2<csCameraBase, 
                                               iCamera,
                                               iSceneNode>
{
private:
  /// @@@@@@@@@@@@@@  NOT USED YET!!!
  csMovable movable;

  /// The sector the camera is in.
  iSector* sector;
  /// If true we are in a mirrored world.
  bool mirror;

  /// Callbacks.
  csRefArray<iCameraSectorListener> listeners;

  /**
   * If true then we only check collision with portals and not
   * with other polygons.
   */
  bool only_portals;

  /// a farplane to cut everything thats behind it
  csPlane3 *fp;

  /**
   * Camera number. This number is changed for every new camera
   * instance and it is also updated whenever the camera transformation
   * changes. This number can be used to cache camera vertex arrays, for
   * example.
   */
  long cameranr;
  /**
   * The last used camera number.
   */
  static long cur_cameranr;

  ///
  void Correct (int n, float* vals[]);

protected:
  void BumpCamera() { cameranr = cur_cameranr++; }
public:
  ///
  csCameraBase ();
  csCameraBase (const csCameraBase& other);
  ///
  virtual ~csCameraBase ();

  /**
   * Get the camera number. This number is changed for every new camera
   * instance and it is also updated whenever the camera transformation
   * changes. This number can be used to cache camera vertex arrays, for
   * example.
   */
  virtual long GetCameraNumber () const
  {
    return cameranr;
  }

  void SetFOV (int a, int width) { }
  virtual int GetFOV () const { return -1; }
  virtual float GetInvFOV () const { return 0; }

  virtual void SetFOVAngle (float a, int width) {}
  virtual float GetFOVAngle () const { return 0; }

  virtual float GetShiftX () const { return 0; }
  virtual float GetShiftY () const { return 0; }

  /// Set farplane, everything behind this will be cut
  virtual void SetFarPlane (csPlane3* farplane);
  /// Get the Farplane
  virtual csPlane3* GetFarPlane () const { return fp; }

  /**
   * Set the sector that the camera resides in.
   * Note that this function does not check if the current
   * camera location is really in that sector. In fact
   * it is legal to have a camera which is viewing the
   * current sector from outside.
   */
  void SetSector (iSector *s)
  {
    if (sector == s) return;
    sector = s;
    cameranr = cur_cameranr++;
    FireCameraSectorListeners (sector);
  }

  /**
   * Get the current sector of the camera.
   */
  iSector* GetSector () const { return sector; }

  /**
   * Returns true if we are in a mirrored world.
   * Basically this means that back-face culling will
   * be reversed.
   */
  bool IsMirrored () const { return mirror; }

  /**
   * Set the mirrored state of this camera.
   * The effect of this is mainly that back-face culling will
   * be reversed. This is useful if you are stepping into a
   * mirrored sector.
   */
  void SetMirrored (bool m)
  {
    if (mirror != m) cameranr = cur_cameranr++;
    mirror = m;
  }

  /**
   * Set 'other' to 'this' transformation matrix.
   * csCamera overrides this in order to be able to update the
   * camera number.
   */
  virtual void SetO2T (const csMatrix3& m)
  {
    csOrthoTransform::SetO2T (m);
    cameranr = cur_cameranr++;
  }

  /**
   * Set 'this' to 'other' transformation matrix.
   * csCamera overrides this in order to be able to update the
   * camera number.
   */
  virtual void SetT2O (const csMatrix3& m)
  {
    csOrthoTransform::SetT2O (m);
    cameranr = cur_cameranr++;
  }

  /**
   * Set 'world' to 'this' translation.
   * csCamera overrides this in order to be able to update the
   * camera number.
   */
  virtual void SetO2TTranslation (const csVector3& v)
  {
    csOrthoTransform::SetO2TTranslation (v);
    cameranr = cur_cameranr++;
  }

  /**
   * Sets the absolute position of the camera inside the sector.
   * Vector 'v' is in world space coordinates. This function does
   * not check if the vector is really in the current sector. In
   * fact it is legal to set the position outside the sector
   * boundaries.
   */
  virtual void SetPosition (const csVector3& v) { SetOrigin (v); }

  /**
   * Set the world to camera transformation matrix.
   * This basically defines the direction that the camera looks.
   */
  inline void SetW2C (const csMatrix3& m) { SetO2T (m); }

  /**
   * Set the camera to world transformation matrix.
   * This basically defines the direction that the camera looks.
   */
  inline void SetC2W (const csMatrix3& m) { SetT2O (m); }

  /**
   * Return the world to camera transformation matrix.
   */
  inline csMatrix3 GetW2C () const { return GetO2T (); }

  /**
   * Return the camera to world transformation matrix.
   */
  inline csMatrix3 GetC2W () const { return GetT2O (); }

  /**
   * Return the world to camera translation.
   */
  inline csVector3 GetW2CTranslation () const { return GetO2TTranslation (); }

  /**
   * Transform a worldspace point to camera space.
   */
  inline csVector3 World2Camera (const csVector3& v) const
  { return Other2This (v); }

  /**
   * Transform a camera space point to world space.
   */
  inline csVector3 Camera2World (const csVector3& v) const
  { return This2Other (v); }

  /**
   * Transform a camera space point to worldspace, relative to camera position.
   */
  inline csVector3 Camera2WorldRelative (const csVector3& v) const
  { return This2OtherRelative (v); }

  /**
   * Moves the camera a relative amount in world coordinates.
   * If 'cd' is true then collision detection with objects and things
   * inside the sector is active. Otherwise you can walk through objects
   * (but portals will still be correctly checked).
   */
  virtual void MoveWorld (const csVector3& v, bool cd = true);

  /**
   * Moves the camera a relative amount in camera coordinates.
   */
  virtual void Move (const csVector3& v, bool cd = true)
  { MoveWorld (m_t2o * v, cd); }

  /**
   * Moves the camera a relative amount in world coordinates,
   * ignoring portals and walls. This is used by the wireframe
   * class. In general this is useful by any camera model that
   * doesn't want to restrict its movement by portals and
   * sector boundaries.
   */
  virtual void MoveWorldUnrestricted (const csVector3& v) { Translate (v); }

  /**
   * Moves the camera a relative amount in camera coordinates,
   * ignoring portals and walls. This is used by the wireframe
   * class. In general this is useful by any camera model that
   * doesn't want to restrict its movement by portals and
   * sector boundaries.
   */
  virtual void MoveUnrestricted (const csVector3& v) { Translate (m_t2o * v); }

  /**
   * Eliminate roundoff error by snapping the camera orientation to a
   * grid of density n
   */
  void Correct (int n);

  void SetPerspectiveCenter (float, float) { } 
  virtual csVector2 Perspective (const csVector3& v) const
  {
    csVector2 p (0);
    return p;
  }
  virtual csVector3 InvPerspective (const csVector2& p, float z) const
  {
    csVector3 v (0);
    return v;
  }

  virtual void AddCameraSectorListener (iCameraSectorListener* listener)
  {
    listeners.Push (listener);
  }

  virtual void RemoveCameraSectorListener (iCameraSectorListener* listener)
  {
    listeners.Delete (listener);
  }

  void FireCameraSectorListeners (iSector* sector);

  virtual void OnlyPortals (bool hop)
  {
    only_portals = hop;
  }
  virtual bool GetOnlyPortals ()
  {
    return only_portals;
  }
  virtual csOrthoTransform& GetTransform ()
  { return *(csOrthoTransform*)this; }
  virtual const csOrthoTransform& GetTransform () const
  { return *(csOrthoTransform*)this; }
  virtual void SetTransform (const csOrthoTransform& tr)
  {
    *(csOrthoTransform*)this = tr;
    cameranr = cur_cameranr++;
  }
  virtual iSceneNode* QuerySceneNode () { return this; }

  /**\name iSceneNode implementation
   * @{ */
  virtual iMovable* GetMovable () const { return 0; }
  virtual void SetParent (iSceneNode* /*parent*/) { }
  virtual iSceneNode* GetParent () const { return 0; }
  virtual const csRefArray<iSceneNode>& GetChildren () const
  {
    return movable.GetChildren ();
  }
  virtual iMeshWrapper* QueryMesh () { return 0; }
  virtual iLight* QueryLight () { return 0; }
  virtual iCamera* QueryCamera () { return this; }
  virtual csPtr<iSceneNodeArray> GetChildrenArray () const
  {
    return csPtr<iSceneNodeArray> (
      new scfArrayWrapConst<iSceneNodeArray, csRefArray<iSceneNode> > (
      movable.GetChildren ()));
  }
  /** @} */
};

class PerspectiveImpl : public iPerspectiveCamera
{
  protected:
  ///
  int aspect;
  static int default_aspect;
  ///
  float inv_aspect;
  static float default_inv_aspect;
  ///
  float shift_x;
  float shift_y;

  /// FOV in angles (degrees).
  float fov_angle;
  static float default_fov_angle;

  /// Compute above angle.
  void ComputeAngle (int width);
  static void ComputeDefaultAngle (int width);

  uint projNumber;
  int matrix_vw, matrix_vh;
  uint matrixProjNumber;
  CS::Math::Matrix4 matrix;
  
  void UpdateMatrix (int viewWidth,
    int viewHeight);
public:
  PerspectiveImpl (int frameWidth, int frameHeight);
  
  /// Set the default FOV for new cameras.
  static void SetDefaultFOV (int fov, int width)
  {
    default_aspect = fov;
    default_inv_aspect = 1.0f / default_aspect;
    ComputeDefaultAngle (width);
  }

  /// Get the default FOV for new cameras.
  static int GetDefaultFOV () { return default_aspect; }
  /// Get the default inverse FOV for new cameras.
  static float GetDefaultInvFOV () { return default_inv_aspect; }
  /// Get the default FOV in angles (degrees).
  static float GetDefaultFOVAngle () { return default_fov_angle; }

  /// Set the FOV for this camera.
  void SetFOV (int a, int width)
  {
    aspect = a;
    inv_aspect = 1.0f / a;
    ComputeAngle (width);
  }
  /// Get the FOV for this camera
  virtual int GetFOV () const { return aspect; }
  /// Get the inverse FOV for this camera.
  virtual float GetInvFOV () const { return inv_aspect; }

  /// Set the FOV in angles (degrees).
  virtual void SetFOVAngle (float a, int width);
  /// Get the FOV in angles (degrees).
  virtual float GetFOVAngle () const
  {
    return fov_angle;
  }

  /// Get the X shift value.
  virtual float GetShiftX () const { return shift_x; }
  /// Get the Y shift value.
  virtual float GetShiftY () const { return shift_y; }

  /// Change the shift for perspective correction.
  void SetPerspectiveCenter (float x, float y)
  {
    shift_x = x; shift_y = y; 
    projNumber++;
  } 

  /// Calculate perspective corrected point for this camera.
  virtual csVector2 Perspective (const csVector3& v) const
  {
    csVector2 p;
    float iz = aspect / v.z;
    p.x = v.x * iz + shift_x;
    p.y = v.y * iz + shift_y;
    return p;
  }

  /// Calculate inverse perspective corrected point for this camera.
  virtual csVector3 InvPerspective (const csVector2& p, float z) const
  {
    csVector3 v;
    v.z = z;
    v.x = (p.x - shift_x) * z * inv_aspect;
    v.y = (p.y - shift_y) * z * inv_aspect;
    return v;
  }
  
  const CS::Math::Matrix4& GetProjectionMatrix (int viewWidth,
    int viewHeight)
  {
    UpdateMatrix (viewWidth, viewHeight);
    return matrix;
  }
};

// Helper to forward iCamera perspective methods to a PerspectiveImpl instance
template<typename ThisClass>
class CameraPerspectiveProxy : public csCameraBase
{
  PerspectiveImpl& Persp ()
  { return *(static_cast<PerspectiveImpl*> (static_cast <ThisClass*> (this))); }
  const PerspectiveImpl& Persp () const
  { return *(static_cast<const PerspectiveImpl*> (
      static_cast <const ThisClass*> (this))); }
public:
  void SetFOV (int a, int width)
  {
    Persp().PerspectiveImpl::SetFOV (a, width);
    BumpCamera();
  }
  int GetFOV () const { return Persp().PerspectiveImpl::GetFOV(); }
  float GetInvFOV () const { return Persp().PerspectiveImpl::GetInvFOV(); }

  void SetFOVAngle (float a, int width)
  {
    Persp().PerspectiveImpl::SetFOVAngle (a, width);
    BumpCamera();
  }
  float GetFOVAngle () const { return Persp().PerspectiveImpl::GetFOVAngle(); }

  float GetShiftX () const { return Persp().PerspectiveImpl::GetShiftX(); }
  float GetShiftY () const { return Persp().PerspectiveImpl::GetShiftY(); }

  void SetPerspectiveCenter (float x, float y)
  {
    Persp().PerspectiveImpl::SetPerspectiveCenter (x, y);
    BumpCamera();
  }
  csVector2 Perspective (const csVector3& v) const
  { return Persp().PerspectiveImpl::Perspective (v); }
  csVector3 InvPerspective (const csVector2& p, float z) const
  { return Persp().PerspectiveImpl::InvPerspective (p, z); }
};

class csCameraPerspective : 
  public PerspectiveImpl,
  public scfImplementationExt1<csCameraPerspective, 
                               CameraPerspectiveProxy<csCameraPerspective>,
                               scfFakeInterface<iPerspectiveCamera> >
{
  uint clipPlanesProjNr;
  csPlane3* clipPlanes;
  
  void UpdateClipPlanes();
public:
  csCameraPerspective (int frameWidth, int frameHeight)
    : PerspectiveImpl (frameWidth, frameHeight), scfImplementationType (this),
      clipPlanesProjNr (~0), clipPlanes (0) {}

  csCameraPerspective (const csCameraPerspective& other)
   : PerspectiveImpl (other), scfImplementationType (this, other),
     clipPlanesProjNr (~0), clipPlanes (0) {}

  ~csCameraPerspective()
  {
    delete[] clipPlanes;
  }
    
  csPtr<iCamera> Clone () const
  {
    return new csCameraPerspective (*this);
  }
  
  const CS::Math::Matrix4& GetProjectionMatrix (int w, int h)
  {
    return PerspectiveImpl::GetProjectionMatrix (w, h);
  }
  
  const csPlane3* GetVisibleVolume (size_t& num)
  {
    UpdateClipPlanes();
    num = 4; /* 4 sides */
    return clipPlanes;
  }
};

class csCameraCustomMatrix :
  public scfImplementationExt1<csCameraCustomMatrix,
                               csCameraBase,
			       iCustomMatrixCamera>

{
  CS::Math::Matrix4 matrix;
public:
  csCameraCustomMatrix () :scfImplementationType (this) {}
    
  csPtr<iCamera> Clone () const
  {
    return new csCameraCustomMatrix (*this);
  }
  
  const CS::Math::Matrix4& GetProjectionMatrix (int, int)
  { return matrix; }
  void SetProjectionMatrix (const CS::Math::Matrix4& m)
  { matrix = m; }
  
  const csPlane3* GetVisibleVolume (size_t& num)
  {
    CS_ASSERT_MSG("Not implemented yet!", false);
    num = 0;
    return 0;
  }
};

#include "csutil/deprecated_warn_on.h"

}
CS_PLUGIN_NAMESPACE_END(Engine)

#endif // __CS_CAMERA_H__
