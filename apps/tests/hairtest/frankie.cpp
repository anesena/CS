/*
  Copyright (C) 2010 Christian Van Brussel, Communications and Remote
      Sensing Laboratory of the School of Engineering at the 
      Universite catholique de Louvain, Belgium
      http://www.tele.ucl.ac.be

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cssysdef.h"
#include "frankie.h"

FrankieScene::FrankieScene (HairTest* hairTest)
  : hairTest (hairTest)
{
  // Define the available keys
  hairTest->hudHelper.keyDescriptions.DeleteAll ();
  hairTest->hudHelper.keyDescriptions.Push ("arrow keys: move camera");
  hairTest->hudHelper.keyDescriptions.Push ("SHIFT-up/down keys: camera closer/farther");
 
  if (hairTest->physicsEnabled)
    hairTest->hudHelper.keyDescriptions.Push ("d: display active colliders");
}

FrankieScene::~FrankieScene ()
{
  if (!animesh)
    return;

  // Remove the mesh from the scene
  csRef<iMeshObject> animeshObject = scfQueryInterface<iMeshObject> (animesh);
  hairTest->engine->RemoveObject (animeshObject->GetMeshWrapper ());

  if (furMesh)
  {
    // Remove the fur mesh from the scene
    csRef<iMeshObject> furMeshObject = scfQueryInterface<iMeshObject> (furMesh);
    hairTest->engine->RemoveObject (furMeshObject->GetMeshWrapper ());
  }
}

csVector3 FrankieScene::GetCameraStart ()
{
  return csVector3 (0.0f, 0.0f, -1.25f);
}

float FrankieScene::GetCameraMinimumDistance ()
{
  return 0.75f;
}

csVector3 FrankieScene::GetCameraTarget ()
{
  csRef<iMeshObject> animeshObject = scfQueryInterface<iMeshObject> (animesh);
  csVector3 avatarPosition = animeshObject->GetMeshWrapper ()->QuerySceneNode ()
    ->GetMovable ()->GetTransform ().GetOrigin ();
  avatarPosition.y = 0.35f;
  return avatarPosition;
}

float FrankieScene::GetSimulationSpeed ()
{
  return 0.25f;
}

bool FrankieScene::HasPhysicalObjects ()
{
  return true;
}

bool FrankieScene::CreateAvatar ()
{
  printf ("Loading Frankie...\n");

  // Load animesh factory
  csLoadResult rc = hairTest->loader->Load ("/lib/frankie/frankie.xml");
  if (!rc.success)
    return hairTest->ReportError ("Can't load Frankie library file!");

  csRef<iMeshFactoryWrapper> meshfact =
    hairTest->engine->FindMeshFactory ("franky_frankie");
  if (!meshfact)
    return hairTest->ReportError ("Can't find Frankie's mesh factory!");

  animeshFactory = scfQueryInterface<CS::Mesh::iAnimatedMeshFactory>
    (meshfact->GetMeshObjectFactory ());
  if (!animeshFactory)
    return hairTest->ReportError ("Can't find Frankie's animesh factory!");

  // Load bodymesh (animesh's physical properties)
  rc = hairTest->loader->Load ("/lib/frankie/skelfrankie_body");
  if (!rc.success)
    return hairTest->ReportError ("Can't load Frankie's body mesh file!");

  csRef<CS::Animation::iBodyManager> bodyManager =
    csQueryRegistry<CS::Animation::iBodyManager> (hairTest->GetObjectRegistry ());
  csRef<CS::Animation::iBodySkeleton> bodySkeleton = bodyManager->FindBodySkeleton ("frankie_body");
  if (!bodySkeleton)
    return hairTest->ReportError ("Can't find Frankie's body mesh description!");

  // Create a new animation tree. The structure of the tree is:
  //   + ragdoll controller node (root node - only if physics are enabled)
  //     + 'LookAt' controller node
  //       + 'speed' controller node
  //         + animation nodes for all speeds
  csRef<CS::Animation::iSkeletonAnimPacketFactory2> animPacketFactory =
    animeshFactory->GetSkeletonFactory ()->GetAnimationPacket ();

  // Create the 'random' node
  csRef<CS::Animation::iSkeletonRandomNodeFactory2> randomNodeFactory =
    animPacketFactory->CreateRandomNode ("random");
  randomNodeFactory->SetAutomaticSwitch (true);

  // Create the 'idle' animation node
  csRef<CS::Animation::iSkeletonAnimationNodeFactory2> idleNodeFactory =
    animPacketFactory->CreateAnimationNode ("idle");
  idleNodeFactory->SetAnimation
    (animPacketFactory->FindAnimation ("Frankie_Idle1"));

  // Create the 'walk_slow' animation node
  csRef<CS::Animation::iSkeletonAnimationNodeFactory2> walkSlowNodeFactory =
    animPacketFactory->CreateAnimationNode ("walk_slow");
  walkSlowNodeFactory->SetAnimation
    (animPacketFactory->FindAnimation ("Frankie_WalkSlow"));

  // Create the 'walk' animation node
  csRef<CS::Animation::iSkeletonAnimationNodeFactory2> walkNodeFactory =
    animPacketFactory->CreateAnimationNode ("walk");
  walkNodeFactory->SetAnimation
    (animPacketFactory->FindAnimation ("Frankie_Walk"));

  // Create the 'walk_fast' animation node
  csRef<CS::Animation::iSkeletonAnimationNodeFactory2> walkFastNodeFactory =
    animPacketFactory->CreateAnimationNode ("walk_fast");
  walkFastNodeFactory->SetAnimation
    (animPacketFactory->FindAnimation ("Frankie_WalkFast"));

  // Create the 'footing' animation node
  csRef<CS::Animation::iSkeletonAnimationNodeFactory2> footingNodeFactory =
    animPacketFactory->CreateAnimationNode ("footing");
  footingNodeFactory->SetAnimation
    (animPacketFactory->FindAnimation ("Frankie_Runs"));

  // Create the 'run_slow' animation node
  csRef<CS::Animation::iSkeletonAnimationNodeFactory2> runSlowNodeFactory =
    animPacketFactory->CreateAnimationNode ("run_slow");
  runSlowNodeFactory->SetAnimation
    (animPacketFactory->FindAnimation ("Frankie_RunSlow"));

  // Create the 'run' animation node
  csRef<CS::Animation::iSkeletonAnimationNodeFactory2> runNodeFactory =
    animPacketFactory->CreateAnimationNode ("run");
  runNodeFactory->SetAnimation
    (animPacketFactory->FindAnimation ("Frankie_Run"));

  // Create the 'run_fast' animation node
  csRef<CS::Animation::iSkeletonAnimationNodeFactory2> runFastNodeFactory =
    animPacketFactory->CreateAnimationNode ("run_fast");
  runFastNodeFactory->SetAnimation
    (animPacketFactory->FindAnimation ("Frankie_RunFaster"));

  // Create the 'run_jump' animation node
  csRef<CS::Animation::iSkeletonAnimationNodeFactory2> runJumpNodeFactory =
    animPacketFactory->CreateAnimationNode ("run_jump");
  runJumpNodeFactory->SetAnimation
    (animPacketFactory->FindAnimation ("Frankie_RunFast2Jump"));

  idleNodeFactory->SetAutomaticReset (true);
  walkSlowNodeFactory->SetAutomaticReset (true);
  walkNodeFactory->SetAutomaticReset (true);
  walkFastNodeFactory->SetAutomaticReset (true);
  footingNodeFactory->SetAutomaticReset (true);
  runSlowNodeFactory->SetAutomaticReset (true);
  runNodeFactory->SetAutomaticReset (true);
  runFastNodeFactory->SetAutomaticReset (true);
  runJumpNodeFactory->SetAutomaticReset (true);

  idleNodeFactory->SetAutomaticStop (false);
  walkSlowNodeFactory->SetAutomaticStop (false);
  walkNodeFactory->SetAutomaticStop (false);
  walkFastNodeFactory->SetAutomaticStop (false);
  footingNodeFactory->SetAutomaticStop (false);
  runSlowNodeFactory->SetAutomaticStop (false);
  runNodeFactory->SetAutomaticStop (false);
  runFastNodeFactory->SetAutomaticStop (false);
  runJumpNodeFactory->SetAutomaticStop (false);

  randomNodeFactory->AddNode (idleNodeFactory, 1.0f);
  randomNodeFactory->AddNode (walkSlowNodeFactory, 1.0f);
  randomNodeFactory->AddNode (walkNodeFactory, 1.0f);
  randomNodeFactory->AddNode (walkFastNodeFactory, 1.0f);
  randomNodeFactory->AddNode (footingNodeFactory, 1.0f);
  randomNodeFactory->AddNode (runSlowNodeFactory, 1.0f);
  randomNodeFactory->AddNode (runNodeFactory, 1.0f);
  randomNodeFactory->AddNode (runFastNodeFactory, 1.0f);
  randomNodeFactory->AddNode (runJumpNodeFactory, 1.0f);

  if (hairTest->physicsEnabled)
  {
    // Create the ragdoll controller
    csRef<CS::Animation::iSkeletonRagdollNodeFactory2> ragdollNodeFactory =
      hairTest->ragdollManager->CreateAnimNodeFactory ("ragdoll",
					     bodySkeleton, hairTest->dynamicSystem);
    animPacketFactory->SetAnimationRoot (ragdollNodeFactory);
    ragdollNodeFactory->SetChildNode (randomNodeFactory);

    // Create a bone chain for the whole body and add it to the ragdoll controller.
    // The chain will be in kinematic mode when Frankie is alive, and in dynamic state
    // when Frankie has been killed.
    bodyChain = bodySkeleton->CreateBodyChain
      ("body_chain", animeshFactory->GetSkeletonFactory ()->FindBone ("Frankie_Main"),
       animeshFactory->GetSkeletonFactory ()->FindBone ("CTRL_Pelvis"),
       animeshFactory->GetSkeletonFactory ()->FindBone ("CTRL_Head"), 0);
    ragdollNodeFactory->AddBodyChain (bodyChain, CS::Animation::STATE_KINEMATIC);

    // Create a bone chain for the tail of Frankie and add it to the ragdoll controller.
    // The chain will be in kinematic mode most of the time, and in dynamic mode when the
    // user ask for it with the 'f' key or when Frankie has been killed.
    tailChain = bodySkeleton->CreateBodyChain
      ("tail_chain", animeshFactory->GetSkeletonFactory ()->FindBone ("Tail_1"),
       animeshFactory->GetSkeletonFactory ()->FindBone ("Tail_8"), 0);
    ragdollNodeFactory->AddBodyChain (tailChain, CS::Animation::STATE_KINEMATIC);
  }

  else
    animPacketFactory->SetAnimationRoot (randomNodeFactory);

  // Create the animated mesh
  csRef<iMeshWrapper> avatarMesh =
    hairTest->engine->CreateMeshWrapper (meshfact, "Frankie",
					   hairTest->room, csVector3 (0.0f));
  animesh = scfQueryInterface<CS::Mesh::iAnimatedMesh> (avatarMesh->GetMeshObject ());

  // When the animated mesh is created, the animation nodes are created too.
  // We can therefore set them up now.
  CS::Animation::iSkeletonAnimNode2* rootNode =
    animesh->GetSkeleton ()->GetAnimationPacket ()->GetAnimationRoot ();

  // Setup of the ragdoll controller
  if (hairTest->physicsEnabled)
  {
    ragdollNode =
      scfQueryInterface<CS::Animation::iSkeletonRagdollNode2> (rootNode->FindNode ("ragdoll"));

    // Start the ragdoll animation node in order to have the rigid bodies created
    ragdollNode->Play ();
  }

  // Load fur material
  rc = hairTest-> loader ->Load ("/hairtest/fur_material_frankie.xml");
  if (!rc.success)
    hairTest->ReportError("Can't load Fur library file!");

  // Load Marschner shader
  csRef<iMaterialWrapper> materialWrapper = 
    hairTest->engine->FindMaterial("marschner_material_frankie");
  if (!materialWrapper)
    hairTest->ReportError("Can't find marschner material!");

  // Get plugin manager
  csRef<iPluginManager> plugmgr = 
    csQueryRegistry<iPluginManager> (hairTest->object_reg);
  if (!plugmgr)
    return hairTest->ReportError("Failed to locate Plugin Manager!");

  // Load animationPhysicsControl
  csRef<CS::Mesh::iFurPhysicsControl> animationPhysicsControl = 
    csLoadPlugin<CS::Mesh::iFurPhysicsControl>
    (plugmgr, "crystalspace.physics.fur.animation");
  if (!animationPhysicsControl)
    return hairTest->ReportError("Failed to locate CS::Mesh::iFurPhysicsControl plugin!");

  // Load hairMeshProperties
  csRef<CS::Mesh::iFurMeshProperties> hairMeshProperties = 
    csQueryRegistry<CS::Mesh::iFurMeshProperties> (hairTest->object_reg);
  if (!hairMeshProperties)
    return hairTest->ReportError("Failed to locate CS::Mesh::iFurMeshProperties plugin!");

  // Load base material
  csRef<iMaterialWrapper> baseMaterial = 
    hairTest->engine->FindMaterial("base_material");
  if (!baseMaterial)	
    return hairTest->ReportError ("Can't find Frankie's base material!");

  // Load furMesh
  csRef<CS::Mesh::iFurMeshType> furMeshType = 
    csQueryRegistry<CS::Mesh::iFurMeshType> (hairTest->object_reg);
  if (!furMeshType)
    return hairTest->ReportError("Failed to locate CS::Mesh::iFurMeshType plugin!");

  csRef<iRigidBody> mainBody = ragdollNode->GetBoneRigidBody
    (animeshFactory->GetSkeletonFactory ()->FindBone ("Frankie_Main"));

  animationPhysicsControl->SetAnimesh(animesh);

  hairMeshProperties->SetMaterial(materialWrapper->GetMaterial());

  csRef<iMeshObjectFactory> imof = furMeshType->NewFactory();

  csRef<iMeshFactoryWrapper> imfw = 
    hairTest->engine->CreateMeshFactory(imof, "frankie_fur_factory");

  csRef<iMeshWrapper> hairMesh = hairTest->engine->
    CreateMeshWrapper (imfw, "fur_mesh_wrapper", hairTest->room, csVector3 (0));

  csRef<iMeshObject> imo = hairMesh->GetMeshObject();

  // Get reference to the iFurMesh interface
  furMesh = scfQueryInterface<CS::Mesh::iFurMesh>(imo);

  furMesh->SetPhysicsControl(animationPhysicsControl);
  furMesh->SetFurStrandGenerator(hairMeshProperties);

  furMesh->SetMeshFactory(animeshFactory);
  furMesh->SetMeshFactorySubMesh(animesh -> GetSubMesh(0)->GetFactorySubMesh());
  furMesh->SetBaseMaterial(baseMaterial->GetMaterial());
  furMesh->GenerateGeometry(hairTest->view, hairTest->room);

  furMesh->StartPhysicsControl();

  furMesh->SetGuideLOD(0);
  furMesh->SetStrandLOD(1);

  // Reset the scene so as to put the parameters of the animation nodes in a default state
  ResetScene ();

  // Start animation
  rootNode->Play ();

  return true;
}

void FrankieScene::ResetScene ()
{
  // Reset the position of the animesh
  csRef<iMeshObject> animeshObject = scfQueryInterface<iMeshObject> (animesh);
  animeshObject->GetMeshWrapper ()->QuerySceneNode ()->GetMovable ()->SetTransform
    (csOrthoTransform (csMatrix3 (), csVector3 (0.0f)));
  animeshObject->GetMeshWrapper ()->QuerySceneNode ()->GetMovable ()->UpdateMove ();

  if (hairTest->physicsEnabled)
  {
    // Set the ragdoll state of the 'body' and 'tail' chains as kinematic
    ragdollNode->SetBodyChainState (bodyChain, CS::Animation::STATE_KINEMATIC);
    ragdollNode->SetBodyChainState (tailChain, CS::Animation::STATE_KINEMATIC);

    // Update the display of the dynamics debugger
    if (hairTest->dynamicsDebugMode == DYNDEBUG_COLLIDER
	|| hairTest->dynamicsDebugMode == DYNDEBUG_MIXED)
      hairTest->dynamicsDebugger->UpdateDisplay ();
  }

  // Reset morphing
  animesh->SetMorphTargetWeight (animeshFactory->FindMorphTarget ("smile.B"), 1.0f);
  animesh->SetMorphTargetWeight (animeshFactory->FindMorphTarget ("eyebrows_down.B"), 1.0f);
  animesh->SetMorphTargetWeight (animeshFactory->FindMorphTarget ("wings_in"), 1.0f);
  animesh->SetMorphTargetWeight (animeshFactory->FindMorphTarget ("eyelids_closed"), 0.0f);
}

void FrankieScene::UpdateStateDescription ()
{
  hairTest->hudHelper.stateDescriptions.DeleteAll ();
}


void FrankieScene::SwitchFurPhysics()
{
}
