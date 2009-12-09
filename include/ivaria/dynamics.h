/*
    Copyright (C) 2002 Anders Stenberg

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


#ifndef __CS_IVARIA_DYNAMICS_H__
#define __CS_IVARIA_DYNAMICS_H__

/**\file
 * Physics interfaces
 */

#include "csutil/scf.h"


struct iBodyGroup;
struct iJoint;
struct iMeshWrapper;
struct iLight;
struct iCamera;
struct iObject;
struct iRigidBody;

class csMatrix3;
class csOrthoTransform;
class csPlane3;
class csVector3;

struct iDynamicsCollisionCallback;
struct iDynamicsMoveCallback;
struct iDynamicSystem;


/**
 * This is the interface for a dynamics step callback.
 */
struct iDynamicsStepCallback : public virtual iBase
{
  SCF_INTERFACE (iDynamicsStepCallback, 0, 0, 1);

  virtual void Step (float stepsize) = 0;
};

/**
 * This is the interface for the actual plugin.
 * It is responsible for creating iDynamicSystem.
 *
 * Main creators of instances implementing this interface:
 * - ODE Physics plugin (crystalspace.dynamics.ode)
 * 
 * Main ways to get pointers to this interface:
 * - csQueryRegistry()
 * 
 * Main users of this interface:
 * - Dynamics loader plugin (crystalspace.dynamics.loader)
 */
struct iDynamics : public virtual iBase
{
  SCF_INTERFACE(iDynamics,0,0,2);
  /// Create a rigid body and add it to the simulation
  virtual csPtr<iDynamicSystem> CreateSystem () = 0;

  /// Remove dynamic system from the simulation
  virtual void RemoveSystem (iDynamicSystem* system) = 0;

  /// Remove all dynamic systems from the simulation
  virtual void RemoveSystems () = 0;

  /// Finds a system by name
  virtual iDynamicSystem* FindSystem (const char *name) = 0;

  /// Step the simulation forward by stepsize.
  virtual void Step (float stepsize) = 0;

  /**
   * Add a callback to be executed dynamics is being stepped.
   */
  virtual void AddStepCallback (iDynamicsStepCallback *callback) = 0;

  /**
   * Remove dynamics step callback.
   */
  virtual void RemoveStepCallback (iDynamicsStepCallback *callback) = 0;
};

struct iDynamicsSystemCollider;

/**
 * This is the interface for the dynamics core.
 * It handles all bookkeeping for rigid bodies and joints.
 * It also handles collision response.
 * Collision detection is done in another plugin.
 *
 * Main creators of instances implementing this interface:
 * - iDynamics::CreateSystem()
 * 
 * Main ways to get pointers to this interface:
 * - iDynamics::FindSystem()
 */
struct iDynamicSystem : public virtual iBase
{
  SCF_INTERFACE (iDynamicSystem, 0, 0, 3);

  /// returns the underlying object
  virtual iObject *QueryObject (void) = 0;
  /// Set the global gravity.
  virtual void SetGravity (const csVector3& v) = 0;
  /// Get the global gravity.
  virtual const csVector3 GetGravity () const = 0;
  /// Set the global linear dampener.
  virtual void SetLinearDampener (float d) = 0;
  /// Get the global linear dampener setting.
  virtual float GetLinearDampener () const = 0;
  /// Set the global rolling dampener.
  virtual void SetRollingDampener (float d) = 0;
  /// Get the global rolling dampener setting.
  virtual float GetRollingDampener () const = 0;

  /**
   * Turn on/off AutoDisable functionality.
   * AutoDisable will stop moving objects if they are stable in order
   * to save processing time. By default this is enabled.
   */
  virtual void EnableAutoDisable (bool enable) = 0;
  virtual bool AutoDisableEnabled () =0;
  /**
   * Set the parameters for AutoDisable.
   * \param linear Maximum linear movement to disable a body
   * \param angular Maximum angular movement to disable a body
   * \param steps Minimum number of steps the body meets linear and angular
   * requirements before it is disabled.
   * \param time Minimum time the body needs to meet linear and angular
   * movement requirements before it is disabled.
   */
  virtual void SetAutoDisableParams (float linear, float angular, int steps,
  	float time)=0;

  /// Step the simulation forward by stepsize.
  virtual void Step (float stepsize) = 0;

  /// Create a rigid body and add it to the simulation
  virtual csPtr<iRigidBody> CreateBody () = 0;

  /// Create a rigid body and add it to the simulation
  virtual void RemoveBody (iRigidBody* body) = 0;

  /// Finds a body within a system
  virtual iRigidBody *FindBody (const char *name) = 0;

  /// Get Rigid Body by its index
  virtual iRigidBody *GetBody (unsigned int index) = 0;

  /// Get rigid bodys count
  virtual int GetBodysCount () = 0;

  /// Create a body group.  Bodies in a group don't collide with each other
  virtual csPtr<iBodyGroup> CreateGroup () = 0;

  /// Remove a group from a simulation.  Those bodies now collide
  virtual void RemoveGroup (iBodyGroup* group) = 0;

  /// Create a joint and add it to the simulation
  virtual csPtr<iJoint> CreateJoint () = 0;

  /// Remove a joint from the simulation
  virtual void RemoveJoint (iJoint* joint) = 0;

  /// Get the default move callback.
  virtual iDynamicsMoveCallback* GetDefaultMoveCallback () = 0;

  /**
   * Attaches a convex static collider mesh to world
   * \param mesh the mesh to use for collision detection. This
   * mesh must be convex.
   * \param trans a hard transform to apply to the mesh
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderConvexMesh (iMeshWrapper* mesh,
    const csOrthoTransform& trans, float friction,
    float elasticity, float softness = 0.01f) = 0;

  /**
   * Attaches a static collider mesh to world
   * \param mesh the mesh to use for collision detection
   * \param trans a hard transform to apply to the mesh
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderMesh (iMeshWrapper* mesh,
    const csOrthoTransform& trans, float friction,
    float elasticity, float softness = 0.01f) = 0;

  /**
   * Attaches a static collider cylinder to world (oriented along it's Z axis)
   * \param length the cylinder length along the axis
   * \param radius the cylinder radius
   * \param trans a hard transform to apply to the mesh
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderCylinder (float length, float radius,
    const csOrthoTransform& trans, float friction,
    float elasticity, float softness = 0.01f) = 0;

  /**
   * Attaches a static collider box to world
   * \param size the box size along each axis
   * \param trans a hard transform to apply to the mesh
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderBox (const csVector3 &size,
    const csOrthoTransform& trans, float friction,
    float elasticity, float softness = 0.01f) = 0;

  /**
   * Attaches a static collider sphere to world
   * \param radius the radius of the sphere
   * \param offset a translation of the sphere's center
   * from the default (0,0,0)
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderSphere (float radius, const csVector3 &offset,
    float friction, float elasticity, float softness = 0.01f) = 0;

  /**
   * Attaches a static collider plane to world
   * \param plane describes the plane to added
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderPlane (const csPlane3 &plane, float friction,
    float elasticity, float softness = 0.01f) = 0;

  /// Destroy all static colliders
  virtual void DestroyColliders () = 0;

  /// Destroy static collider
  virtual void DestroyCollider (iDynamicsSystemCollider* collider) = 0;

  /// Attach collider to dynamic system 
  virtual void AttachCollider (iDynamicsSystemCollider* collider) = 0;

  /**
   * Create static collider and put it into simulation. After collision it
   * will remain in the same place, but it will affect collided dynamic
   * colliders (to make it dynamic, just attach it to the rigid body).
   */
  virtual csRef<iDynamicsSystemCollider> CreateCollider () = 0;

  /// Get static collider.
  virtual csRef<iDynamicsSystemCollider> GetCollider (unsigned int index) = 0;

  /// Get static colliders count.
  virtual int GetColliderCount () = 0;

  /**
   * Attaches a static collider capsule to world (oriented along it's Z axis).
   * A capsule is a cylinder with an halph-sphere at each end. It is less costly
   * to compute collisions with a capsule than with a cylinder.
   * \param length the capsule length along the axis (i.e. the distance between the 
   * two halph-sphere's centers)
   * \param radius the capsule radius
   * \param trans a hard transform to apply to the mesh
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderCapsule (float length, float radius,
    const csOrthoTransform& trans, float friction,
    float elasticity, float softness = 0.01f) = 0;
};

/**
 * This is the interface for a dynamics move callback.
 * Set on iRigidBody, it can update attachments after each step.
 *
 * Main ways to get pointers to this interface:
 * - application specific
 * - iDynamicSystem::GetDefaultMoveCallback()
 * 
 * Main users of this interface:
 * - iDynamicSystem
 */
struct iDynamicsMoveCallback : public virtual iBase
{
  SCF_INTERFACE (iDynamicsMoveCallback, 0, 0, 1);

  /// Update the position of the mesh with the specified transform.
  virtual void Execute (iMeshWrapper* mesh, csOrthoTransform& t) = 0;

  /// Update the position of the light with the specified transform.
  virtual void Execute (iLight* light, csOrthoTransform& t) = 0;

  /// Update the position of the camera with the specified transform.
  virtual void Execute (iCamera* camera, csOrthoTransform& t) = 0;

  /**
   * Update the position of the rigid body with the specified transform. If 
   * you want to attach to the rigid body an object different than a mesh, a 
   * camera or a light, then you should reimplement this method and update 
   * here the position of your object.
   */
  virtual void Execute (csOrthoTransform& t) = 0;
};

/**
 * This is the interface for attaching a collider callback to the body
 * 
 * Main ways to get pointers to this interface:
 * - application specific
 *   
 * Main users of this interface:
 * - iDynamicSystem
 *   
 */
struct iDynamicsCollisionCallback : public virtual iBase
{
  SCF_INTERFACE (iDynamicsCollisionCallback, 0, 0, 2);

  /**
   * A collision occured.
   * \param thisbody The body that received a collision.
   * \param otherbody The body that collided with \a thisBody.
   * \param pos is the position on which the collision occured.
   * \param normal is the collision normal.
   * \param depth is the penetration depth.
   */
  virtual void Execute (iRigidBody *thisbody, iRigidBody *otherbody,
      const csVector3& pos, const csVector3& normal, float depth) = 0;
};

/**
 * Body Group is a collection of bodies which don't collide with
 * each other.  This can speed up processing by manually avoiding
 * certain collisions.  For instance if you have a car built of
 * many different bodies.  The bodies can be collected into a group
 * and the car will be treated as a single object.
 *
 * Main creators of instances implementing this interface:
 * - iDynamicSystem::CreateGroup()
 * 
 * Main ways to get pointers to this interface:
 * - iRigidBody::GetGroup()
 * 
 * Main users of this interface:
 * - iDynamicSystem
 */
struct iBodyGroup : public virtual iBase
{
  SCF_INTERFACE (iBodyGroup, 0, 1, 0);

  /// Adds a body to this group
  virtual void AddBody (iRigidBody *body) = 0;
  /// Removes a body from this group
  virtual void RemoveBody (iRigidBody *body) = 0;
  /// Tells whether the body is in this group or not
  virtual bool BodyInGroup (iRigidBody *body) = 0;
};

/**
 * This is the interface for a rigid body.
 * It keeps all properties for the body.
 * It can also be attached to a movable or a bone,
 * to automatically update it.
 *
 * Main creators of instances implementing this interface:
 * - iDynamicSystem::CreateBody()
 * 
 * Main ways to get pointers to this interface:
 * - iDynamicSystem::FindBody()
 * 
 * Main users of this interface:
 * - iDynamicSystem
 */
struct iRigidBody : public virtual iBase
{
  SCF_INTERFACE (iRigidBody, 0, 0, 3);

  /// returns the underlying object
  virtual iObject *QueryObject (void) = 0;
  /**
   * Makes a body stop reacting dynamically.  This is especially useful
   * for environmental objects.  It will also increase speed in some cases
   * by ignoring all physics for that body
   */
  virtual bool MakeStatic (void) = 0;
  /// Returns a static body to a dynamic state
  virtual bool MakeDynamic (void) = 0;
  /// Tells whether a body has been made static or not
  virtual bool IsStatic (void) = 0;
  /**
    * Temporarily ignores the body until something collides with it.
  */
  virtual bool Disable (void) = 0;
  /// Re-enables a body after calling Disable, or by being auto disabled
  virtual bool Enable (void) = 0;
  /// Returns true if a body is enabled.
  virtual bool IsEnabled (void) = 0; 

  /// Returns which group a body belongs to
  virtual csRef<iBodyGroup> GetGroup (void) = 0;

  /**
   * Add a collider with a associated friction coefficient
   * \param mesh the mesh object which will act as collider. This
   * must be a convex mesh.
   * \param trans a hard transform to apply to the mesh
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param density the density of this rigid body (used to calculate
   * mass based on collider volume)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderConvexMesh (iMeshWrapper* mesh,
    const csOrthoTransform& trans, float friction, float density,
    float elasticity, float softness = 0.01f) = 0;

  /**
   * Add a collider with a associated friction coefficient
   * \param mesh the mesh object which will act as collider
   * \param trans a hard transform to apply to the mesh
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param density the density of this rigid body (used to calculate
   * mass based on collider volume)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderMesh (iMeshWrapper* mesh,
    const csOrthoTransform& trans, float friction, float density,
    float elasticity, float softness = 0.01f) = 0;

  /**
   * Cylinder orientated along its local z axis
   * \param length length of the cylinder
   * \param radius radius of the cylinder
   * \param trans a hard transform to apply to the mesh
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param density the density of this rigid body (used to calculate
   * mass based on collider volume)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderCylinder (float length, float radius,
    const csOrthoTransform& trans, float friction, float density,
    float elasticity, float softness = 0.01f) = 0;

  /**
   * Add a collider box with given properties
   * \param size the box's dimensions
   * \param trans a hard transform to apply to the mesh
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param density the density of this rigid body (used to calculate
   * mass based on collider volume)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderBox (const csVector3 &size,
    const csOrthoTransform& trans, float friction, float density,
    float elasticity, float softness = 0.01f) = 0;

  /**
   * Add a collider sphere with given properties
   * \param radius radius of sphere
   * \param offset position of sphere
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param density the density of this rigid body (used to calculate
   * mass based on collider volume)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderSphere (float radius, const csVector3 &offset,
    float friction, float density, float elasticity,
    float softness = 0.01f) = 0;

  /**
   * Add a collider plane with given properties
   * \param plane the plane which will act as collider
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param density the density of this rigid body (used to calculate
   * mass based on collider volume)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderPlane (const csPlane3 &plane, float friction,
    float density, float elasticity, float softness = 0.01f) = 0;

  /** 
   * Attach collider to rigid body. If you have set colliders transform before
   * then it will be considered as relative to attached body (but still if you 
   * will use colliders "GetTransform ()" it will be in the world coordinates.
   * Colider become dynamic (which means that it will follow rigid body).
   */
  virtual void AttachCollider (iDynamicsSystemCollider* collider) = 0;

  /// Destroy body colliders.
  virtual void DestroyColliders () = 0;

  /// Destroy body collider.
  virtual void DestroyCollider (iDynamicsSystemCollider* collider) = 0;

  /// Set the position
  virtual void SetPosition (const csVector3& trans) = 0;
  /// Get the position
  virtual const csVector3 GetPosition () const = 0;
  /// Set the orientation
  virtual void SetOrientation (const csMatrix3& trans) = 0;
  /// Get the orientation
  virtual const csMatrix3 GetOrientation () const = 0;
  /// Set the transform
  virtual void SetTransform (const csOrthoTransform& trans) = 0;
  /// Get the transform
  virtual const csOrthoTransform GetTransform () const = 0;
  /// Set the linear velocity (movement)
  virtual void SetLinearVelocity (const csVector3& vel) = 0;
  /// Get the linear velocity (movement)
  virtual const csVector3 GetLinearVelocity () const = 0;
  /// Set the angular velocity (rotation)
  virtual void SetAngularVelocity (const csVector3& vel) = 0;
  /// Get the angular velocity (rotation)
  virtual const csVector3 GetAngularVelocity () const = 0;

  /// Set the physic properties
  virtual void SetProperties (float mass, const csVector3& center,
    const csMatrix3& inertia) = 0;
  /// Get the physic properties. 0 parameters are ignored
  virtual void GetProperties (float* mass, csVector3* center,
    csMatrix3* inertia) = 0;
  /// Get the total mass of this body
  virtual float GetMass () = 0;
  /// Get the center of mass of this body
  virtual csVector3 GetCenter () = 0;
  /// Get the matrix of inertia of this body
  virtual csMatrix3 GetInertia () = 0;

  /**
   * Set the total mass to targetmass, and adjust the properties 
   * (center of mass and matrix of inertia)
   */
  virtual void AdjustTotalMass (float targetmass) = 0;

  /// Add a force (world space) (active for one timestep)
  virtual void AddForce (const csVector3& force) = 0;
  /// Add a torque (world space) (active for one timestep)
  virtual void AddTorque (const csVector3& force) = 0;
  /// Add a force (local space) (active for one timestep)
  virtual void AddRelForce (const csVector3& force) = 0;
  /// Add a torque (local space) (active for one timestep)
  virtual void AddRelTorque (const csVector3& force) = 0 ;
  /**
   * Add a force (world space) at a specific position (world space)
   * (active for one timestep)
   */
  virtual void AddForceAtPos (const csVector3& force, const csVector3& pos) = 0;
  /**
   * Add a force (world space) at a specific position (local space)
   * (active for one timestep)
   */
  virtual void AddForceAtRelPos (const csVector3& force,
    const csVector3& pos) = 0;
  /**
   * Add a force (local space) at a specific position (world space)
   * (active for one timestep)
   */
  virtual void AddRelForceAtPos (const csVector3& force,
    const csVector3& pos) = 0;
  /**
   * Add a force (local space) at a specific position (local space)
   * (active for one timestep)
   */
  virtual void AddRelForceAtRelPos (const csVector3& force,
    const csVector3& pos) = 0;

  /// Get total force (world space)
  virtual const csVector3 GetForce () const = 0;
  /// Get total torque (world space)
  virtual const csVector3 GetTorque () const = 0;

  /*
  /// Get total force (local space)
  virtual const csVector3& GetRelForce () const = 0;
  /// Get total force (local space)
  virtual const csVector3& GetRelTorque () const = 0;
  */

  /*
  /// Get the number of joints attached to this body
  virtual int GetJointCount () const = 0;
  */

  /// Attach an iMeshWrapper to this body
  virtual void AttachMesh (iMeshWrapper* mesh) = 0;
  /// Returns the attached MeshWrapper
  virtual iMeshWrapper* GetAttachedMesh () = 0;
  /// Attach an iLight to this body
  virtual void AttachLight (iLight* light) = 0;
  /// Returns the attached light
  virtual iLight* GetAttachedLight () = 0;
  /// Attach an iCamera to this body
  virtual void AttachCamera (iCamera* camera) = 0;
  /// Returns the attached camera
  virtual iCamera* GetAttachedCamera () = 0;

  /**
   * Set a callback to be executed when this body moves.
   * If 0, no callback is executed.
   */
  virtual void SetMoveCallback (iDynamicsMoveCallback* cb) = 0;
  /**
   * Set a callback to be executed when this body collides with another.
   * If 0, no callback is executed.
   */
  virtual void SetCollisionCallback (iDynamicsCollisionCallback* cb) = 0;

  /**
   * If there's a collision callback with this body, execute it
   * \param other The body that collided.
   * \param pos is the position on which the collision occured.
   * \param normal is the collision normal.
   * \param depth is the penetration depth.
   */
  virtual void Collision (iRigidBody *other, const csVector3& pos,
      const csVector3& normal, float depth) = 0;

  /// Update transforms for mesh and/or bone
  virtual void Update () = 0;

  /// Get body collider by its index
  virtual csRef<iDynamicsSystemCollider> GetCollider (unsigned int index) = 0;

  /// Get body colliders count 
  virtual int GetColliderCount () = 0;

  /**
   * Attaches a collider capsule to the body (oriented along it's Z axis).
   * A capsule is a cylinder with an halph-sphere at each end. It is less costly
   * to compute collisions with a capsule than with a cylinder.
   * \param length the capsule length along the axis (i.e. the distance between the 
   * two halph-sphere's centers)
   * \param radius the capsule radius
   * \param trans a hard transform to apply to the mesh
   * \param friction how much friction this body has,
   * ranges from 0 (no friction) to infinity (perfect friction)
   * \param elasticity the "bouncyness" of this object, from 0
   * (no bounce) to 1 (maximum bouncyness)
   * \param softness how "squishy" this object is, in the range
   * 0...1; small values (range of 0.00001 to 0.01) give
   * reasonably stiff collision contacts, larger values
   * are more "mushy"
   */
  virtual bool AttachColliderCapsule (float length, float radius,
    const csOrthoTransform& trans, float friction, float density,
    float elasticity, float softness = 0.01f) = 0;
};

enum csColliderGeometryType
{
  NO_GEOMETRY,
  BOX_COLLIDER_GEOMETRY,
  PLANE_COLLIDER_GEOMETRY,
  TRIMESH_COLLIDER_GEOMETRY,
  CONVEXMESH_COLLIDER_GEOMETRY,
  CYLINDER_COLLIDER_GEOMETRY,
  CAPSULE_COLLIDER_GEOMETRY,
  SPHERE_COLLIDER_GEOMETRY
};


/**
 * This is the interface for attaching a collider callback to the body
 *
 * Main ways to get pointers to this interface:
 * - application specific
 * 
 * Main users of this interface:
 * - iDynamicSystem
 */
struct iDynamicsColliderCollisionCallback : public virtual iBase
{
  SCF_INTERFACE (iDynamicsColliderCollisionCallback, 0, 0, 1);

  virtual void Execute (iDynamicsSystemCollider *thiscollider, 
    iDynamicsSystemCollider *othercollider) = 0;
  virtual void Execute (iDynamicsSystemCollider *thiscollider, 
    iRigidBody *otherbody) = 0;
};


struct iGeneralFactoryState;
class csBox3;
class csSphere;
class csReversibleTransform;

/**
 * This is the interface for a dynamics system collider.
 * It keeps all properties that system uses for collision 
 * detection and after collision behaviour (like surface 
 * properties, collider geometry). It can be placed into 
 * dynamic system (then this will be "static" collider by default) 
 * or attached to body (then it will be "dynamic"). 
 *
 * Main creators of instances implementing this interface:
 * - iDynamicSystem::CreateCollider()
 * 
 * Main ways to get pointers to this interface:
 * - iDynamicSystem::GetCollider()
 * - iRigidBody::GetCollider()
 * 
 * Main users of this interface:
 * - iDynamicSystem
 * - iRigidBody
 */
struct iDynamicsSystemCollider : public virtual iBase
{
  SCF_INTERFACE (iDynamicsSystemCollider, 0, 0, 3);

  /// Create Collider Geometry with given sphere.
  virtual bool CreateSphereGeometry (const csSphere& sphere) = 0;

  /// Create Collider Geometry with given plane.
  virtual bool CreatePlaneGeometry (const csPlane3& plane) = 0;

  /// Create Collider Geometry with given convex mesh geometry.
  virtual bool CreateConvexMeshGeometry (iMeshWrapper *mesh) = 0;

  /// Create Collider Geometry with given mesh geometry.
  virtual bool CreateMeshGeometry (iMeshWrapper *mesh) = 0;

  /// Create Collider Geometry with given box (given by its size).
  virtual bool CreateBoxGeometry (const csVector3& box_size) = 0;

  /// Create Capsule Collider Geometry.
  virtual bool CreateCapsuleGeometry (float length, float radius) = 0;

  /// Create Cylinder Geometry.
  virtual bool CreateCylinderGeometry (float length, float radius) = 0;

  //FIXME: This should be implememented, but it is not so obvious - it
  //should be valid also for static colliders.
  virtual void SetCollisionCallback (
  	iDynamicsColliderCollisionCallback* cb) = 0;

  /// Set the friction of the collider surface.
  virtual void SetFriction (float friction) = 0;

  /// Set the softness of the collider surface.
  virtual void SetSoftness (float softness) = 0;

  /**
   * Set the density of the body. This could look strange that collider needs
   * to know this parameter, but it is used for reseting mass of connected body  
   * (it should depend on collider geometry)
   */
  virtual void SetDensity (float density) = 0;

  /// Set the elasticity of the collider surface.
  virtual void SetElasticity (float elasticity) = 0;
  
  /// Get the friction of the collider surface.
  virtual float GetFriction () = 0;

  /// Get the softness of the collider surface.
  virtual float GetSoftness () = 0;

  /// Get the density of the body.
  virtual float GetDensity () = 0;

  /// Get the elasticity of the collider surface.
  virtual float GetElasticity () = 0;

  /// Fill given General Mesh factory with collider geometry 
  virtual void FillWithColliderGeometry (
  	csRef<iGeneralFactoryState> genmesh_fact) = 0;

  /// Get the type of the geometry.
  virtual csColliderGeometryType GetGeometryType () = 0;

  /// Get collider transform (it will always be in world coordinates)
  virtual csOrthoTransform GetTransform () = 0;

  /**
   * Get collider transform. If the collider is attached to a body, then the
   *  transform will be in body space, otherwise it will be in world coordinates.
   */
  virtual csOrthoTransform GetLocalTransform () = 0;

  /**
   * Set Collider transform. If this is a "static" collider then the given transform
   * will be in world space, otherwise it will be in attached rigid body space.
   */ 
  virtual void SetTransform (const csOrthoTransform& trans) = 0;

  /**
   * If this collider has a box geometry then the method will return true and the 
   * size of the box, otherwise it will return false.
   */
  virtual bool GetBoxGeometry (csVector3& size) = 0;

  /**
   * If this collider has a sphere geometry then the method will return true and
   * the sphere, otherwise it will return false.
   */
  virtual bool GetSphereGeometry (csSphere& sphere) = 0;

  /**
   * If this collider has a plane geometry then the method will return true and 
   * the plane, otherwise it will return false.
   */
  virtual bool GetPlaneGeometry (csPlane3& plane) = 0;

  /**
   * If this collider has a cylinder geometry then the method will return true and
   * the cylinder's length and radius, otherwise it will return false.
   */
  virtual bool GetCylinderGeometry (float& length, float& radius) = 0;

  /**
   * Make collider static. Static collider acts on dynamic colliders and bodies,
   * but ignores other static colliders (it won't do precise collision detection 
   * in that case).
   */
  virtual void MakeStatic () = 0;

  /**
   * Make collider dynamic. Dynamic colliders acts (it collision is checked, and 
   * collision callbacks called) on every other collider and body.
   */
  virtual void MakeDynamic () = 0;

  /// Check if collider is static. 
  virtual bool IsStatic () = 0;

  /**
   * If this collider has a capsule geometry then the method will return true and
   * the capsule's length and radius, otherwise it will return false.
   */
  virtual bool GetCapsuleGeometry (float& length, float& radius) = 0;
};

/**
 * This is the interface for a joint.  It works by constraining
 * the relative motion between the two bodies it attaches.  For
 * instance if all motion in along the local X axis is constrained
 * then the bodies will stay motionless relative to each other
 * along an x axis rotated and positioned by the Joint's transform.
 *
 * Main creators of instances implementing this interface:
 * - iDynamicSystem::CreateJoint()
 * 
 * Main users of this interface:
 * - iDynamicSystem
 */
struct iJoint : public virtual iBase
{
  SCF_INTERFACE (iJoint, 0, 0, 1);

  /**
   * Set which two bodies to be affected by this joint. Set force_update to true if 
   * you want to apply changes right away.
   */
  virtual void Attach (iRigidBody* body1, iRigidBody* body2, bool force_update = true) = 0;
  /// Get an attached body (valid values for body are 0 and 1).
  virtual csRef<iRigidBody> GetAttachedBody (int body) = 0;
  /**
   * Set the local transformation of the joint.  This transform
   * sets the position of the constraining axes in the world
   * not relative to the attached bodies. Set force_update to true if 
   * you want to apply changes right away.
   */
  virtual void SetTransform (const csOrthoTransform &trans, bool force_update = true) = 0;
  /// Get the local transformation of the joint.
  virtual csOrthoTransform GetTransform () = 0;
  /**
   * Set the translation constraints on the 3 axes.  If true is
   * passed for an axis then the Joint will constrain all motion along
   * that axis.  If false is passed in then all motion along that
   * axis is free, but bounded by the minimum and maximum distance
   * if set. Set force_update to true if you want to apply changes 
   * right away.
   */
  virtual void SetTransConstraints (bool X, bool Y, bool Z, bool force_update = true) = 0;
  /// True if this axis' translation is constrained.
  virtual bool IsXTransConstrained () = 0;
  /// True if this axis' translation is constrained.
  virtual bool IsYTransConstrained () = 0;
  /// True if this axis' translation is constrained.
  virtual bool IsZTransConstrained () = 0;
  /**
   * Set the minimum constrained distance between bodies. Set force_update to true if 
   * you want to apply changes right away.
   */
  virtual void SetMinimumDistance (const csVector3 &min, bool force_update = true) = 0;
  /// Get the minimum constrained distance between bodies.
  virtual csVector3 GetMinimumDistance () = 0;
  /**
   * Set the maximum constrained distance between bodies. Set force_update to true if 
   * you want to apply changes right away.
   */
  virtual void SetMaximumDistance (const csVector3 &max, bool force_update = true) = 0;
  /// Get the maximum constrained distance between bodies.
  virtual csVector3 GetMaximumDistance () = 0;
  /**
   * Set the rotational constraints on the 3 axes.  If true is
   * passed for an axis then the Joint will constrain all rotation around
   * that axis.  If false is passed in then all rotation around that
   * axis is free, but bounded by the minimum and maximum distance
   * if set. Set force_update to true if you want to apply changes 
   * right away.
   */
  virtual void SetRotConstraints (bool X, bool Y, bool Z, bool force_update = true) = 0;
  /// True if this axis' rotation is constrained.
  virtual bool IsXRotConstrained () = 0;
  /// True if this axis' rotation is constrained.
  virtual bool IsYRotConstrained () = 0;
  /// True if this axis' rotation is constrained
  virtual bool IsZRotConstrained () = 0;
  /**
   * Set the minimum constrained angle between bodies (in radian). Set force_update to true if 
   * you want to apply changes right away.
   */
  virtual void SetMinimumAngle (const csVector3 &min, bool force_update = true) = 0;
  /// Get the minimum constrained angle between bodies (in radian).
  virtual csVector3 GetMinimumAngle () = 0;
  /**
   * Set the maximum constrained angle between bodies (in radian). Set force_update to true if 
   * you want to apply changes right away.
   */
  virtual void SetMaximumAngle (const csVector3 &max, bool force_update = true) = 0;
  /// Get the maximum constrained angle between bodies (in radian).
  virtual csVector3 GetMaximumAngle () = 0;

  //Motor parameters

  /** 
   * Set the restitution of the joint's stop point (this is the 
   * elasticity of the joint when say throwing open a door how 
   * much it will bounce the door back closed when it hits).
   */
  virtual void SetBounce (const csVector3 & bounce, bool force_update = true) = 0;
  /// Get the joint restitution.
  virtual csVector3 GetBounce () = 0;
  /**
   * Apply a motor velocity to joint (for instance on wheels). Set force_update to true if 
   * you want to apply changes right away.
   */
  virtual void SetDesiredVelocity (const csVector3 &velocity, bool force_update = true) = 0;
  virtual csVector3 GetDesiredVelocity () = 0;
  /**
   * Set the force at which the desired velocity will be achieved. Set force_update to true if 
   * you want to apply changes right away.
   */
  virtual void SetMaxForce (const csVector3 & maxForce, bool force_update = true) = 0;
  virtual csVector3 GetMaxForce () = 0;
  /**
   * Set custom angular constraint axis (have sense only with rotation free minimum along 2 axis).
   * Set force_update to true if you want to apply changes right away.
   */
  virtual void SetAngularConstraintAxis (const csVector3 &axis, int body, bool force_update = true) = 0;
  /// Get custom angular constraint axis.
  virtual csVector3 GetAngularConstraintAxis (int body) = 0;
  /**
   * Rebuild joint using current setup. Returns true if rebuilding operation is sucesfull
   * (otherwise joint won't be active).
   */
  virtual bool RebuildJoint () = 0;

};

#endif // __CS_IVARIA_DYNAMICS_H__

