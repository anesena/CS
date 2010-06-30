/*
  Copyright (C) 2010 Alexandru - Teodor Voicu

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
#include <cssysdef.h>
#include <iutil/objreg.h>
#include <iutil/plugin.h>

#include "furmaterial.h"

CS_PLUGIN_NAMESPACE_BEGIN(FurMaterial)
{
  /********************
  *  FurMaterialType
  ********************/

  SCF_IMPLEMENT_FACTORY (FurMaterialType)

    CS_LEAKGUARD_IMPLEMENT(FurMaterialType);

  FurMaterialType::FurMaterialType (iBase* parent) :
  scfImplementationType (this, parent),
    object_reg(0)
  {
  }

  FurMaterialType::~FurMaterialType ()
  {
    furMaterialHash.DeleteAll();
  }

  // From iComponent
  bool FurMaterialType::Initialize (iObjectRegistry* r)
  {
    object_reg = r;
    return true;
  }

  // From iFurMaterialType
  void FurMaterialType::ClearFurMaterials ()
  {
    furMaterialHash.DeleteAll ();
  }

  void FurMaterialType::RemoveFurMaterial (const char *name, iFurMaterial* furMaterial)
  {
    furMaterialHash.Delete (name, furMaterial);
  }

  iFurMaterial* FurMaterialType::CreateFurMaterial (const char *name)
  {
    csRef<iFurMaterial> newFur;
    newFur.AttachNew(new FurMaterial (this, name, object_reg));
    return furMaterialHash.PutUnique (name, newFur);
  }

  iFurMaterial* FurMaterialType::FindFurMaterial (const char *name) const
  {
    return furMaterialHash.Get (name, 0);
  }

  /********************
  *  FurMaterial
  ********************/

  CS_LEAKGUARD_IMPLEMENT(FurMaterial);

  FurMaterial::FurMaterial (FurMaterialType* manager, const char *name, 
    iObjectRegistry* object_reg) :
  scfImplementationType (this), manager (manager), name (name), 
    object_reg(object_reg), physicsControl(0), furMaterialWrapper(0)
  {
    svStrings = csQueryRegistryTagInterface<iShaderVarStringSet> (
      object_reg, "crystalspace.shader.variablenameset");

    if (!svStrings) 
      printf ("No SV names string set!");

    engine = csQueryRegistry<iEngine> (object_reg);
    if (!engine) printf ("Failed to locate 3D engine!");

    loader = csQueryRegistry<iLoader> (object_reg);
    if (!loader) printf ("Failed to locate Loader!");
  }

  FurMaterial::~FurMaterial ()
  {
  }

  void FurMaterial::GenerateGeometry (iView* view, iSector *room)
  {
    iRenderBuffer* vertexes = meshFactory->GetVertices();
    iRenderBuffer* normals = meshFactory->GetNormals();
    //iRenderBuffer* texCoord = meshFactory->GetTexCoords();
    iRenderBuffer* indices = meshFactorySubMesh->GetIndices(0);

    GenerateGuidHairs(indices, vertexes, normals);
    SynchronizeGuideHairs();
    GenerateHairStrands(indices, vertexes);

    this->view = view;
    int controlPoints = hairStrands.Get(0).controlPointsCount;
    int numberOfStrains = hairStrands.GetSize();

    csRef<iEngine> engine = csQueryRegistry<iEngine> (object_reg);
    if (!engine) csApplicationFramework::ReportError("Failed to locate iEngine plugin!");

    // First create the factory:
    csRef<iMeshFactoryWrapper> factory = engine->CreateMeshFactory (
      "crystalspace.mesh.object.genmesh", "hairFactory");

    factoryState = scfQueryInterface<iGeneralFactoryState> (
      factory->GetMeshObjectFactory ());

    factoryState -> SetVertexCount ( numberOfStrains * 2 * controlPoints );
    factoryState -> SetTriangleCount ( numberOfStrains * 2 * (controlPoints - 1));

    csVector3 *vbuf = factoryState->GetVertices (); 
    csTriangle *ibuf = factoryState->GetTriangles ();

    for ( int x = 0 ; x < numberOfStrains ; x ++ )
    {
      for ( int y = 0 ; y < controlPoints ; y ++ )
      {
        vbuf[ x * 2 * controlPoints + 2 * y].Set
          ( hairStrands.Get(x).controlPoints[y] );
        vbuf[ x * 2 * controlPoints + 2 * y + 1].Set
          ( hairStrands.Get(x).controlPoints[y] + csVector3(-0.01f,0,0) );
      }

      for ( int y = 0 ; y < 2 * (controlPoints - 1) ; y ++ )
      {
        if (y % 2 == 0)
        {
          ibuf[ x * 2 * (controlPoints - 1) + y ].Set
            ( 2 * x * controlPoints + y , 
            2 * x * controlPoints + y + 1 , 
            2 * x * controlPoints + y + 2 );
          //printf("%d %d %d\n", 2 * x + y , 2 * x + y + 3 , 2 * x + y + 1);
        }
        else
        {
          ibuf[ x * 2 * (controlPoints - 1) + y ].Set
            ( 2 * x * controlPoints + y , 
            2 * x * controlPoints + y + 2 , 
            2 * x * controlPoints + y + 1 );
          //printf("%d %d %d\n", 2 * x + y + 1 , 2 * x + y + 2 , 2 * x + y - 1);
        }
      }
    }

    factoryState -> CalculateNormals();

    // Make a ball using the genmesh plug-in.
    csRef<iMeshWrapper> meshWrapper =
      engine->CreateMeshWrapper (factory, "hair", room, csVector3 (0, 0, 0));

    csRef<iMaterialWrapper> materialWrapper = 
      CS::Material::MaterialBuilder::CreateColorMaterial
      (object_reg,"hairDummyMaterial",csColor(1,0,0));

    if (furMaterialWrapper && furMaterialWrapper->GetMaterial())
      materialWrapper->SetMaterial(furMaterialWrapper->GetMaterial());

    meshWrapper -> GetMeshObject() -> SetMaterialWrapper(materialWrapper);

    csRef<iGeneralMeshState> meshState =
      scfQueryInterface<iGeneralMeshState> (meshWrapper->GetMeshObject ());

    csRef<FurAnimationControl> animationControl;
    animationControl.AttachNew(new FurAnimationControl(this));
    meshState -> SetAnimationControl(animationControl);
  }

  void FurMaterial::GenerateGuidHairs(iRenderBuffer* indices, 
    iRenderBuffer* vertexes, iRenderBuffer* normals)
  {
    csRenderBufferLock<csVector3> positions (vertexes, CS_BUF_LOCK_READ);
    csRenderBufferLock<csVector3> norms (normals, CS_BUF_LOCK_READ);
    CS::TriangleIndicesStream<size_t> tris (indices, CS_MESHTYPE_TRIANGLES);    
    csArray<int> uniqueIndices;

    csVector3 *normsArray = new csVector3[positions.GetSize()];

    // chose unique indices
    while (tris.HasNext())
    {
      CS::TriangleT<size_t> tri (tris.Next ());

      if(uniqueIndices.Contains(tri.a) == csArrayItemNotFound)
        uniqueIndices.Push(tri.a);
      if(uniqueIndices.Contains(tri.b) == csArrayItemNotFound)
        uniqueIndices.Push(tri.b);
      if(uniqueIndices.Contains(tri.c) == csArrayItemNotFound)
        uniqueIndices.Push(tri.c);

      csTriangle triangleNew = csTriangle(uniqueIndices.Contains(tri.a),
        uniqueIndices.Contains(tri.b), uniqueIndices.Contains(tri.c));
      guideHairsTriangles.Push(triangleNew);

      normsArray[tri.a] = csVector3( norms.Get(tri.a).x, norms.Get(tri.a).z, norms.Get(tri.a).y );
      normsArray[tri.b] = csVector3( norms.Get(tri.b).x, norms.Get(tri.b).z, norms.Get(tri.b).y );
      normsArray[tri.c] = csVector3( norms.Get(tri.c).x, norms.Get(tri.c).z, norms.Get(tri.c).y );
    }

    // generate the guide hairs - this should be done based on heightmap
    for (size_t i = 0; i < uniqueIndices.GetSize(); i ++)
    {
      csVector3 pos = positions.Get(uniqueIndices.Get(i)) + 
        displaceEps * normsArray[uniqueIndices.Get(i)];

      csGuideHair guideHair;
      guideHair.controlPointsCount = 5;
      guideHair.controlPoints = new csVector3[ guideHair.controlPointsCount ];

      for ( size_t j = 0 ; j < guideHair.controlPointsCount ; j ++ )
        guideHair.controlPoints[j] = pos + j * 0.05f * normsArray[uniqueIndices.Get(i)];
      //guideHair.controlPoints[j] = pos + j * 0.05f * csVector3(0,1,0);

      guideHairs.Push(guideHair);
    }
  }

  void FurMaterial::SynchronizeGuideHairs ()
  {
    if (!physicsControl) // no physics support
      return;

    for (size_t i = 0 ; i < guideHairs.GetSize(); i ++)
      physicsControl->InitializeStrand(i,guideHairs.Get(i).controlPoints, 
      guideHairs.Get(i).controlPointsCount);
  }

  void FurMaterial::GenerateHairStrands (iRenderBuffer* indices, iRenderBuffer* 
    vertexes)
  {
    csRandomGen rng (csGetTicks ());
    float bA, bB, bC; // barycentric coefficients

    float density = 5;

    CS::ShaderVarName densityName (svStrings, "density");	
    csRef<csShaderVariable> shaderVariable = material->GetVariable(densityName);

    if (shaderVariable)
      shaderVariable->GetValue(density);

    // for every triangle
    for (size_t iter = 0 ; iter < guideHairsTriangles.GetSize(); iter ++)
    {
      for ( int den = 0 ; den < density ; den ++ )
      {
        csHairStrand hairStrand;

        bA = rng.Get();
        bB = rng.Get() * (1 - bA);
        bC = 1 - bA - bB;

        hairStrand.guideHairsCount = 3;
        hairStrand.guideHairs = new csGuideHairReference[hairStrand.guideHairsCount];
        int indexA = guideHairsTriangles.Get(iter).a;
        int indexB = guideHairsTriangles.Get(iter).b;
        int indexC = guideHairsTriangles.Get(iter).c;

        hairStrand.guideHairs[0].distance = bA;
        hairStrand.guideHairs[0].index = indexA;
        hairStrand.guideHairs[1].distance = bB;
        hairStrand.guideHairs[1].index = indexB;
        hairStrand.guideHairs[2].distance = bC;
        hairStrand.guideHairs[2].index = indexC;

        hairStrand.controlPointsCount = csMin(
          (csMin(guideHairs.Get(indexA).controlPointsCount,
          guideHairs.Get(indexB).controlPointsCount)),
          (guideHairs.Get(indexC).controlPointsCount));

        hairStrand.controlPoints = new csVector3[ hairStrand.controlPointsCount ];

        for ( size_t i = 0 ; i < hairStrand.controlPointsCount ; i ++ )
        {
          hairStrand.controlPoints[i] = csVector3(0);
          for ( size_t j = 0 ; j < hairStrand.guideHairsCount ; j ++ )
            hairStrand.controlPoints[i] += hairStrand.guideHairs[j].distance *
            guideHairs.Get(hairStrand.guideHairs[j].index).controlPoints[i];
        }

        hairStrands.Push(hairStrand);		
      }
    }
  }

  void FurMaterial::SetPhysicsControl (iFurPhysicsControl* physicsControl)
  {
    this->physicsControl = physicsControl;
  }

  void FurMaterial::SetMeshFactory ( iAnimatedMeshFactory* meshFactory)
  {
    this->meshFactory = meshFactory;
  }

  void FurMaterial::SetMeshFactorySubMesh ( iAnimatedMeshFactorySubMesh* 
    meshFactorySubMesh )
  { 
    this->meshFactorySubMesh = meshFactorySubMesh;
  }

  void FurMaterial::SetFurMaterialWrapper( iFurMaterialWrapper* furMaterialWrapper)
  {
    this->furMaterialWrapper = furMaterialWrapper;
  }

  iFurMaterialWrapper* FurMaterial::GetFurMaterialWrapper( )
  {
    return furMaterialWrapper;
  }

  void FurMaterial::SetMaterial ( iMaterial* material )
  {
    this->material = material;

    SetColor(csColor(1,1,0));
    SetDensitymap();
    SetHeightmap();
    SetStrandWidth();
    SetDisplaceEps();
  }

  void FurMaterial::SetColor(csColor color)
  {
    CS::ShaderVarName furColorName (svStrings, "mat furcolor");	
    csRef<csShaderVariable> shaderVariable = 
      material->GetVariable(furColorName);
    if(!shaderVariable)
    {
      shaderVariable = material->GetVariableAdd(furColorName);
      shaderVariable->SetValue(color);	
    }
  }

  void FurMaterial::SetStrandWidth()
  {
    CS::ShaderVarName strandWidthName (svStrings, "width");	
    csRef<csShaderVariable> shaderVariable = material->GetVariable(strandWidthName);

    shaderVariable->GetValue(strandWidth);
  }

  void FurMaterial::SetDensitymap ()
  {
    CS::ShaderVarName densitymapName (svStrings, "density map");	
    csRef<csShaderVariable> shaderVariable = material->GetVariable(densitymapName);

    shaderVariable->GetValue(densitymap);
    //printf("%s\n", densitymap->GetImageName());
  }

  void FurMaterial::SetHeightmap ()
  {
    CS::ShaderVarName heightmapName (svStrings, "height map");	
    csRef<csShaderVariable> shaderVariable = material->GetVariable(heightmapName);

    shaderVariable->GetValue(heightmap);
    //printf("%s\n", heightmap->GetImageName());
  }

  void FurMaterial::SetDisplaceEps()
  {
    displaceEps = 0.02f;

    CS::ShaderVarName displaceEpsName (svStrings, "displaceEps");	
    csRef<csShaderVariable> shaderVariable = 
      material->GetVariable(displaceEpsName);

    if (shaderVariable)
      shaderVariable->GetValue(displaceEps);
  }

  void FurMaterial::SetShader (csStringID type, iShader* shd)
  {
    shaders.PutUnique (type, shd);
  }

  iShader* FurMaterial::GetShader(csStringID type)
  {
    return shaders.Get (type, (iShader*)0);
  }

  iShader* FurMaterial::GetFirstShader (const csStringID* types,
    size_t numTypes)
  {
    iShader* s = 0;
    for (size_t i = 0; i < numTypes; i++)
    {
      s = shaders.Get (types[i], (iShader*)0);
      if (s != 0) break;
    }
    return s;
  }

  iTextureHandle *FurMaterial::GetTexture ()
  {
    return 0;
  }

  iTextureHandle* FurMaterial::GetTexture (CS::ShaderVarStringID name)
  {
    return 0;
  }  

  /********************
  *  FurAnimationControl
  ********************/

  CS_LEAKGUARD_IMPLEMENT(FurAnimationControl);

  FurAnimationControl::FurAnimationControl (FurMaterial* furMaterial)
    : scfImplementationType (this), lastTicks (0), furMaterial(furMaterial)
  {
  }

  FurAnimationControl::~FurAnimationControl ()
  {
  }  

  bool FurAnimationControl::AnimatesColors () const
  {
    return false;
  }

  bool FurAnimationControl::AnimatesNormals () const
  {
    return false;
  }

  bool FurAnimationControl::AnimatesTexels () const
  {
    return false;
  }

  bool FurAnimationControl::AnimatesVertices () const
  {
    return true;
  }

  void FurAnimationControl::UpdateHairStrand (csHairStrand* hairStrand)
  {
    for ( size_t i = 0 ; i < hairStrand->controlPointsCount; i++ )
    {
      hairStrand->controlPoints[i] = csVector3(0);
      for ( size_t j = 0 ; j < hairStrand->guideHairsCount ; j ++ )
        hairStrand->controlPoints[i] += hairStrand->guideHairs[j].distance * (
        furMaterial->guideHairs.Get(hairStrand->guideHairs[j].index).controlPoints[i] + 
        (int)pow(-1.0,(int)i) * csVector3(0.0f,0.0f,0.0f));
    }
  }

  void FurAnimationControl::Update (csTicks current, int num_verts, uint32 version_id)
  {
    // update shader
    if (furMaterial->furMaterialWrapper)
      furMaterial->furMaterialWrapper->Update();

    // first update the control points
    if (furMaterial->physicsControl)
      for (size_t i = 0 ; i < furMaterial->guideHairs.GetSize(); i ++)
        furMaterial->physicsControl->AnimateStrand(i,
        furMaterial->guideHairs.Get(i).controlPoints,
        furMaterial->guideHairs.Get(i).controlPointsCount);

    // then update the hair strands
    if (furMaterial->physicsControl)
      for (size_t i = 0 ; i < furMaterial->hairStrands.GetSize(); i ++)
        UpdateHairStrand(&furMaterial->hairStrands.Get(i));

    const csOrthoTransform& tc = furMaterial->view -> GetCamera() ->GetTransform ();
    /*
    CS::ShaderVarName objEyePos (furMaterial->svStrings, "objEyePos");	
    csRef<csShaderVariable> shaderVariable = furMaterial->furMaterial->GetVariable(objEyePos);

    shaderVariable->SetValue(tc.GetOrigin());
    */
    //printf("%f\n", tc.GetOrigin().y);

    int controlPoints = furMaterial->hairStrands.Get(0).controlPointsCount;
    int numberOfStrains = furMaterial->hairStrands.GetSize();

    csVector3 *vbuf = furMaterial->factoryState->GetVertices (); 

    for ( int x = 0 ; x < numberOfStrains ; x ++ )
    {
      int y = 0;
      csVector3 strip = csVector3(0);

      for ( y = 0 ; y < controlPoints - 1; y ++ )
      {

        csVector3 firstPoint = furMaterial->hairStrands.Get(x).controlPoints[y];
        csVector3 secondPoint = furMaterial->hairStrands.Get(x).controlPoints[y + 1];
        csVector3 normal;

        csMath3::CalcNormal(normal, firstPoint, secondPoint, tc.GetOrigin());
        normal.Normalize();
        strip = furMaterial->strandWidth * normal;

        vbuf[ x * 2 * controlPoints + 2 * y].Set( firstPoint );
        vbuf[ x * 2 * controlPoints + 2 * y + 1].Set( firstPoint + strip );
      }

      vbuf[ x * 2 * controlPoints + 2 * y].Set
        ( furMaterial->hairStrands.Get(x).controlPoints[y] );
      vbuf[ x * 2 * controlPoints + 2 * y + 1].Set
        ( furMaterial->hairStrands.Get(x).controlPoints[y] + strip );
    }

    furMaterial->factoryState->CalculateNormals();
    furMaterial->factoryState->Invalidate();

    furMaterial->factoryState->AddRenderBuffer(CS_BUFFER_TANGENT, 
      furMaterial->factoryState->GetRenderBuffer(CS_BUFFER_TANGENT));
    furMaterial->factoryState->RemoveRenderBuffer(CS_BUFFER_TANGENT);

  }

  const csColor4* FurAnimationControl::UpdateColors (csTicks current, 
    const csColor4* colors, int num_colors, uint32 version_id)
  {
    return colors;
  }

  const csVector3* FurAnimationControl::UpdateNormals (csTicks current, 
    const csVector3* normals, int num_normals, uint32 version_id)
  {
    return normals;
  }

  const csVector2* FurAnimationControl::UpdateTexels (csTicks current, 
    const csVector2* texels, int num_texels, uint32 version_id)
  {
    return texels;
  }

  const csVector3* FurAnimationControl::UpdateVertices (csTicks current, 
    const csVector3* verts, int num_verts, uint32 version_id)
  {
    return 0;
  }


  /********************
  *  FurPhysicsControl
  ********************/
  SCF_IMPLEMENT_FACTORY (FurPhysicsControl)

    CS_LEAKGUARD_IMPLEMENT(FurPhysicsControl);	

  FurPhysicsControl::FurPhysicsControl (iBase* parent)
    : scfImplementationType (this, parent), object_reg(0)
  {
  }

  FurPhysicsControl::~FurPhysicsControl ()
  {
    guideRopes.DeleteAll();
  }

  // From iComponent
  bool FurPhysicsControl::Initialize (iObjectRegistry* r)
  {
    object_reg = r;
    return true;
  }

  //-- iFurPhysicsControl

  void FurPhysicsControl::SetRigidBody (iRigidBody* rigidBody)
  {
    this->rigidBody = rigidBody;
  }

  void FurPhysicsControl::SetBulletDynamicSystem (iBulletDynamicSystem* 
    bulletDynamicSystem)
  {
    this->bulletDynamicSystem = bulletDynamicSystem;
  }

  // Initialize the strand with the given ID
  void FurPhysicsControl::InitializeStrand (size_t strandID, const csVector3* 
    coordinates, size_t coordinatesCount)
  {
    csVector3 first = coordinates[0];
    csVector3 last = coordinates[coordinatesCount - 1];

    iBulletSoftBody* bulletBody = bulletDynamicSystem->
      CreateRope(first, first + csVector3(0, 1, 0) * (last - first).Norm() , 
      coordinatesCount - 2);	//	replace with -1
    bulletBody->SetMass (0.1f);
    bulletBody->SetRigidity (0.99f);
    bulletBody->AnchorVertex (0, rigidBody);

    guideRopes.PutUnique(strandID, bulletBody);
  }

  // Animate the strand with the given ID
  void FurPhysicsControl::AnimateStrand (size_t strandID, csVector3* 
    coordinates, size_t coordinatesCount)
  {
    csRef<iBulletSoftBody> bulletBody = guideRopes.Get (strandID, 0);

    if(!bulletBody)
      return;

    CS_ASSERT(coordinatesCount == bulletBody->GetVertexCount());
    //printf("%d\t%d\n", coordinatesCount, bulletBody->GetVertexCount());

    for ( size_t i = 0 ; i < coordinatesCount ; i ++ )
      coordinates[i] = bulletBody->GetVertexPosition(i);
    /*
    if (strandID % 3 == 0)
    for ( size_t i = 0 ; i < coordinatesCount ; i ++ )
    coordinates[i] = bulletBody->GetVertexPosition(0)+ i*0.05 *
    csVector3(1,0,0);
    else if (strandID % 3 == 1)
    for ( size_t i = 0 ; i < coordinatesCount ; i ++ )
    coordinates[i] = bulletBody->GetVertexPosition(0)+ i*0.05 *
    csVector3(0,1,0);
    else
    for ( size_t i = 0 ; i < coordinatesCount ; i ++ )
    coordinates[i] = bulletBody->GetVertexPosition(0)+ i*0.05 *
    csVector3(0,0,1);
    */
  }

  void FurPhysicsControl::RemoveStrand (size_t strandID)
  {
    csRef<iBulletSoftBody> bulletBody = guideRopes.Get (strandID, 0);
    if(!bulletBody)
      return;

    guideRopes.Delete(strandID, bulletBody);
  }

  void FurPhysicsControl::RemoveAllStrands ()
  {
    guideRopes.DeleteAll();
  }


  /************************
  *  FurMaterialWrapper
  ************************/  
  SCF_IMPLEMENT_FACTORY (FurMaterialWrapper)

    CS_LEAKGUARD_IMPLEMENT(FurMaterialWrapper);	

  FurMaterialWrapper::FurMaterialWrapper (iBase* parent)
    : scfImplementationType (this, parent), object_reg(0), material(0), 
    valid(false), width(256), height(256), M(0), m_buf(0), gauss_matrix(0),
    N(0), n_buf(0)
  {
  }

  FurMaterialWrapper::~FurMaterialWrapper ()
  {
  }

  // From iComponent
  bool FurMaterialWrapper::Initialize (iObjectRegistry* r)
  {
    object_reg = r;

    svStrings = csQueryRegistryTagInterface<iShaderVarStringSet> (
      object_reg, "crystalspace.shader.variablenameset");
  
    if (!svStrings) 
    {
      printf ("No SV names string set!\n");
      return false;
    }

    g3d = csQueryRegistry<iGraphics3D> (object_reg);
    
    if (!g3d) 
    {
      printf("No g3d found!\n");
      return false;
    }

    mc = new MarschnerConstants();

    return true;
  }

  // From iFurMaterialWrapper
  iMaterial* FurMaterialWrapper::GetMaterial()
  {
    return material;
  }

  void FurMaterialWrapper::SetMaterial(iMaterial* material)
  {
    this->material = material;
  }

  void FurMaterialWrapper::Invalidate()
  {
    valid = false;
  }

  void FurMaterialWrapper::Update()
  {
    if(!valid && material)
    {
      UpdateM();
      UpdateN();
      valid = true;
    }
  }

  // Marschner specific methods
  void FurMaterialWrapper::UpdateM()
  {
    if(!M)
    {
      CS::ShaderVarName strandWidthName (svStrings, "tex M");	
      csRef<csShaderVariable> shaderVariable = material->GetVariableAdd(strandWidthName);

      M = g3d->GetTextureManager()->CreateTexture(width, height, csimg2D, "abgr8", 
        CS_TEXTURE_3D | CS_TEXTURE_NOMIPMAPS | CS_TEXTURE_NOFILTER);

      if(!M)
      {
        printf ("Failed to load M texture!\n");
        return;
      }

      shaderVariable->SetValue(M);

      m_buf = new uint8 [width * height * 4];
      gauss_matrix = new float [width * height];

      for( int x = 0 ; x < width ; x ++ )
        for (int y = 0 ; y < height; y ++)
        {
          m_buf[ 4 * (x + y * width ) ] = 255; // red
          m_buf[ 4 * (x + y * width ) + 1 ] = 0; // green
          m_buf[ 4 * (x + y * width ) + 2 ] = 0; // blue
          m_buf[ 4 * (x + y * width ) + 3 ] = 255; // alpha
        }
    }

    ComputeM(mc->aR, mc->bR, 0);
    ComputeM(mc->aTT, mc->bTT, 1);
    ComputeM(mc->aTRT, mc->bTRT, 2);

    // send buffer to texture
    M->Blit(0, 0, width, height / 2, m_buf);
    M->Blit(0, height / 2, width, height / 2, m_buf + (width * height * 2));

    // test new texture
    CS::StructuredTextureFormat readbackFmt 
      (CS::TextureFormatStrings::ConvertStructured ("abgr8"));

    csRef<iDataBuffer> db = M->Readback(readbackFmt);
    SaveImage(db->GetUint8(), "/data/hairtest/debug/M_debug.png");
  }

  float FurMaterialWrapper::ComputeM(float a, float b, int channel)
  {
    float max = 0;
    
    // find max
    for (int x = 0; x < width; x++)
      for (int y = 0; y < height; y++)    
      {
        float sin_thI = -1.0f + (x * 2.0f) / width;
        float sin_thR = -1.0f + (y * 2.0f) / height;
        float thI = (180 * asin(sin_thI) / PI);
        float thR = (180 * asin(sin_thR) / PI);
        float thH = (thR + thI) / 2;
        float thH_a = thH - a;

        float gauss = MarschnerHelper::GaussianDistribution(b, thH_a);
        gauss_matrix[x + y * width] = gauss;

        if (255 * gauss > max)
          max = 255 * gauss;
      }

    // normalize
    for (int x = 0; x < width; x++)
      for (int y = 0; y < height; y++)
      {
        float gauss = gauss_matrix[x + y * width];
        m_buf[4 * (x + y * width) + channel] = (uint8)(255 * 255 * gauss / max);
      }

    return max;
  }

  void FurMaterialWrapper::UpdateN()
  {
    if(!N)
    {
      CS::ShaderVarName strandWidthName (svStrings, "tex N");	
      csRef<csShaderVariable> shaderVariable = material->GetVariableAdd(strandWidthName);

       N = g3d->GetTextureManager()->CreateTexture(width, height, csimg2D, "abgr8", 
         CS_TEXTURE_3D | CS_TEXTURE_NOMIPMAPS | CS_TEXTURE_NOFILTER);

      if(!N)
      {
        printf ("Failed to load N texture!\n");
        return;
      }

      shaderVariable->SetValue(N);

      n_buf = new uint8 [width * height * 4];

      for( int x = 0 ; x < width ; x ++ )
        for (int y = 0 ; y < height; y ++)
        {
          n_buf[ 4 * (x + y * width ) ] = 0; // red
          n_buf[ 4 * (x + y * width ) + 1 ] = 0; // green
          n_buf[ 4 * (x + y * width ) + 2 ] = 0; // blue
          n_buf[ 4 * (x + y * width ) + 3 ] = 255; // alpha
        }
    }

    for( int x = 0 ; x < width ; x ++ )
      for (int y = 0 ; y < height; y ++)
      {
        float cos_phiD = -1.0f + (x * 2.0f) / width;
        float cos_thD = -1.0f + (y * 2.0f) / height;
        float phiD = acos(cos_phiD);
        float thD = acos(cos_thD);
        n_buf[ 4 * (x + y * width ) ] = (uint8)(255 * SimpleNP(phiD, thD)); // red
        n_buf[ 4 * (x + y * width ) ] = (uint8)(255 * ComputeNP(0, phiD, thD)); // red
        n_buf[ 4 * (x + y * width ) + 1 ] = (uint8)(255 * ComputeNP(1, phiD, thD)); // green
        n_buf[ 4 * (x + y * width ) + 2 ] = (uint8)(255 * ComputeNP(2, phiD, thD)); // blue
      }

    // send buffer to texture
    N->Blit(0, 0, width, height / 2, n_buf);
    N->Blit(0, height / 2, width, height / 2, n_buf + (width * height * 2));

    // test new texture
    CS::StructuredTextureFormat readbackFmt 
      (CS::TextureFormatStrings::ConvertStructured ("abgr8"));

    csRef<iDataBuffer> db = N->Readback(readbackFmt);
    SaveImage(db->GetUint8(), "/data/hairtest/debug/N_debug.png");
  }

  float FurMaterialWrapper::ComputeT(float absorption, float gammaT)
  {
    float l = 1 + cos(2 * gammaT);	// l = ls / cos qt = 2r cos h0t / cos qt
    return exp(-2.0f * absorption * l);
  }

  float FurMaterialWrapper::ComputeA(float absorption, int p, float h, 
    float refraction, float etaPerpendicular, float etaParallel)
  {
    float gammaI = asin(h);

    //A(0; h) = F(h0; h00; gi)
    if (p == 0)
      return MarschnerHelper::Fresnel(etaPerpendicular, etaParallel, gammaI);

    //A(p; h) = ( (1 - F(h0; h00; gi) ) ^ 2 ) * ( F(1 / h0; 1 / h00; gi) ^ (p - 1) ) * ( T(s0a; h) ^ p )
    float gammaT = asin(h / etaPerpendicular);	// h0 sin gt = h

    float fresnel = MarschnerHelper::Fresnel(etaPerpendicular, etaParallel, gammaI);
    float invFrenel = MarschnerHelper::Fresnel(1 / etaPerpendicular, 1 / etaParallel, gammaT);
    float t = ComputeT(absorption, gammaT);

    return (1.0f - fresnel) * (1.0f - fresnel) * pow(invFrenel, p - 1.0f) * pow(t, p);
  }

  float FurMaterialWrapper::ComputeNP(int p, float phi, float thD)
  {
    float refraction = mc->eta;
    float absorption = mc->absorption;

    float etaPerpendicular = MarschnerHelper::BravaisIndex(thD, refraction);
    float etaParallel = (refraction * refraction) / etaPerpendicular;

    csVector4 roots = EquationsSolver::Roots(p, etaPerpendicular, phi);
    float result = 0;

    for (int index = 0; index < roots.w; index++ )
    {
      float gammaI = roots[index];

      //if (fabs(gammaI) <= PI/2)
      {
        float h = sin(gammaI);
        float finalAbsorption = ComputeA(absorption, p, h, refraction, 
          etaPerpendicular, etaParallel);
        float inverseDerivateAngle = 
          EquationsSolver::InverseFirstDerivate(p, etaPerpendicular, h);

        result += finalAbsorption * 2 * fabs(inverseDerivateAngle); //0.5 here
      }
    }

    return csMin(1.0f, result);
  }

  float FurMaterialWrapper::SimpleNP(float phi, float thD )
  {
    float refraction = mc->eta;

    float etaPerpendicular = MarschnerHelper::BravaisIndex(thD, refraction);
    float etaParallel = (refraction * refraction) / etaPerpendicular;
    float gammaI = -phi / 2.0f;

    float h = sin(gammaI);

    float result = (sqrt(1 - h * h));

    result *= MarschnerHelper::Fresnel(etaPerpendicular, etaParallel, gammaI);

    return csMin(1.0f, result);
  }

  void FurMaterialWrapper::SaveImage(uint8* buf, const char* texname)
  {
    csRef<iImageIO> imageio = csQueryRegistry<iImageIO> (object_reg);
    csRef<iVFS> VFS = csQueryRegistry<iVFS> (object_reg);

    if(!buf)
    {
      printf("Bad data buffer!\n");
      return;
    }

    csRef<iImage> image;
    image.AttachNew(new csImageMemory (width, height, buf,false,
      CS_IMGFMT_TRUECOLOR | CS_IMGFMT_ALPHA));

    if(!image.IsValid())
    {
      printf("Error loading image\n");
      return;
    }

    csPrintf ("Saving %zu KB of data.\n", 
      csImageTools::ComputeDataSize (image)/1024);

    csRef<iDataBuffer> db = imageio->Save (image, "image/png", "progressive");
    if (db)
    {
      if (!VFS->WriteFile (texname, (const char*)db->GetData (), db->GetSize ()))
      {
        printf("Failed to write file '%s'!", texname);
        return;
      }
    }
    else
    {
      printf("Failed to save png image for basemap!");
      return;
    }	    
  }
}
CS_PLUGIN_NAMESPACE_END(FurMaterial)

