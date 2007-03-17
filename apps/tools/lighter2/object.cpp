/*
  Copyright (C) 2005-2006 by Marten Svanfeldt

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

#include "crystalspace.h"

#include "common.h"
#include "lighter.h"
#include "lightmap.h"
#include "lightmapuv.h"
#include "object.h"
#include "config.h"

namespace lighter
{

  ObjectFactory::ObjectFactory ()
    : lightPerVertex (false),
    lmuScale (globalConfig.GetLMProperties ().uTexelPerUnit),
    lmvScale (globalConfig.GetLMProperties ().vTexelPerUnit), 
    factoryWrapper (0)
  {
  }

  bool ObjectFactory::PrepareLightmapUV (LightmapUVFactoryLayouter* uvlayout)
  {
    BeginSubmeshRemap ();
    if (lightPerVertex)
    {
      size_t oldSize = unlayoutedPrimitives.GetSize();
      for (size_t i = 0; i < oldSize; i++)
      {
        layoutedPrimitives.Push (LayoutedPrimitives (unlayoutedPrimitives[i],
          0, 0));
        AddSubmeshRemap (i, i);
      }
      unlayoutedPrimitives.DeleteAll();
    }
    else
    {
      size_t oldSize = unlayoutedPrimitives.GetSize();
      csBitArray usedVertices;
      usedVertices.SetSize (vertexData.positions.GetSize ());

      for (size_t i = 0; i < oldSize; i++)
      {
        csArray<FactoryPrimitiveArray> newPrims;
        csRef<LightmapUVObjectLayouter> lightmaplayout = 
          uvlayout->LayoutFactory (unlayoutedPrimitives[i], vertexData, this, 
          newPrims, usedVertices);
        if (!lightmaplayout) return false;

        for (size_t n = 0; n < newPrims.GetSize(); n++)
        {
          layoutedPrimitives.Push (LayoutedPrimitives (newPrims[n],
            lightmaplayout, n));

          AddSubmeshRemap (i, layoutedPrimitives.GetSize () - 1);
        }
      }
      unlayoutedPrimitives.DeleteAll();
    }
    FinishSubmeshRemap ();

    return true;
  }

  csPtr<Object> ObjectFactory::CreateObject ()
  {
    return csPtr<Object> (new Object (this));
  }

  void ObjectFactory::ParseFactory (iMeshFactoryWrapper *factory)
  {
    this->factoryWrapper = factory;
    // Get the name
    this->factoryName = factoryWrapper->QueryObject ()->GetName ();   


    csRef<iObjectIterator> objiter = 
      factoryWrapper->QueryObject ()->GetIterator();
    while (objiter->HasNext())
    {
      iObject* obj = objiter->Next();
      csRef<iKeyValuePair> kvp = 
        scfQueryInterface<iKeyValuePair> (obj);
      if (kvp.IsValid() && (strcmp (kvp->GetKey(), "lighter2") == 0))
      {
        const char* vVertexlight = kvp->GetValue ("vertexlight");
        if (vVertexlight != 0)
          lightPerVertex = (strcmp (vVertexlight, "yes") == 0);

        const char* vLMScale = kvp->GetValue ("lmscale");
        if (vLMScale)
        {
          float u=0,v=0;
          if (sscanf (vLMScale, "%f,%f", &u, &v) == 2)
          {
            lmuScale = u;
            lmvScale = v;
          }
        }
      }
    }
  }

  void ObjectFactory::SaveFactory (iDocumentNode *node)
  {
    // Save out the factory to the node
    csRef<iSaverPlugin> saver = 
      csQueryPluginClass<iSaverPlugin> (globalLighter->pluginManager,
      saverPluginName);      
    if (!saver) 
      saver = csLoadPlugin<iSaverPlugin> (globalLighter->pluginManager,
      saverPluginName);
    if (saver) 
    {
      // Write new mesh
      csRef<iDocumentNode> paramChild = node->GetNode ("params");
      saver->WriteDown(factoryWrapper->GetMeshObjectFactory (), node,
        0/*ssource*/);
      if (paramChild) 
      {
        // Move all nodes after the old params node to after the new params node
        csRef<iDocumentNodeIterator> nodes = node->GetNodes();
        while (nodes->HasNext())
        {
          csRef<iDocumentNode> child = nodes->Next();
          if (child->Equals (paramChild)) break;
        }
        // Skip <params>
        if (nodes->HasNext()) 
        {
          // Actual moving
          while (nodes->HasNext())
          {
            csRef<iDocumentNode> child = nodes->Next();
            if ((child->GetType() == CS_NODE_ELEMENT)
              && (strcmp (child->GetValue(), "params") == 0))
              break;
            csRef<iDocumentNode> newNode = node->CreateNodeBefore (
              child->GetType(), 0);
            CS::DocumentHelper::CloneNode (child, newNode);
            node->RemoveNode (child);
          }
        }
        node->RemoveNode (paramChild);
      }
    }
  }

  //-------------------------------------------------------------------------

  Object::Object (ObjectFactory* fact)
    : lightPerVertex (fact->lightPerVertex), litColors (0), litColorsPD (0), 
      factory (fact)
  {
  }
  
  Object::~Object ()
  {
    delete litColors;
    delete litColorsPD;
  }

  bool Object::Initialize ()
  {
    if (!factory || !meshWrapper) return false;
    const csReversibleTransform transform = meshWrapper->GetMovable ()->
      GetFullTransform ();

    //Copy over data, transform the radprimitives..
    vertexData = factory->vertexData;
    vertexData.Transform (transform);

    unsigned int i = 0;
    this->allPrimitives.SetCapacity (factory->layoutedPrimitives.GetSize ());
    for(size_t j = 0; j < factory->layoutedPrimitives.GetSize (); ++j)
    {
      FactoryPrimitiveArray& factPrims = factory->layoutedPrimitives[j].primitives;
      PrimitiveArray& allPrimitives =
        this->allPrimitives.GetExtend (j);

      allPrimitives.SetCapacity (allPrimitives.GetSize() + factPrims.GetSize());
      for (i = 0; i < factPrims.GetSize(); i++)
      {
        Primitive newPrim (vertexData);
        
        Primitive& prim = allPrimitives[allPrimitives.Push (newPrim)];
        //prim.SetOriginalPrimitive (&factPrims[i]);
        prim.SetTriangle (factPrims[i].GetTriangle ()); 
        prim.ComputePlane ();
      }

      if (!lightPerVertex)
      {
        // FIXME: probably separate out to allow for better progress display
        bool res = 
          factory->layoutedPrimitives[j].factory->LayoutUVOnPrimitives (
            allPrimitives, factory->layoutedPrimitives[j].group, vertexData, 
            lightmapIDs.GetExtend (j));
        if (!res) return false;
      }

    }

    factory.Invalidate();

    return true;
  }

  void Object::PrepareLighting ()
  {
    for (size_t j = 0; j < this->allPrimitives.GetSize(); j++)
    {
      PrimitiveArray& allPrimitives = this->allPrimitives[j];
      allPrimitives.ShrinkBestFit ();

      for (size_t i = 0; i < allPrimitives.GetSize(); i++)
      {
        Primitive& prim = allPrimitives[i];
        prim.ComputeUVTransform ();
        prim.SetObject (this);
        prim.Prepare ();
      }
    }

    if (lightPerVertex)
    {
      litColors = new LitColorArray();
      litColors->SetSize (vertexData.positions.GetSize(), 
        csColor (0.0f, 0.0f, 0.0f));
      litColorsPD = new LitColorsPDHash();
    }
  }

  void Object::StripLightmaps (csSet<csString>& lms)
  {
    iShaderVariableContext* svc = meshWrapper->GetSVContext();
    csShaderVariable* sv = svc->GetVariable (
      globalLighter->strings->Request ("tex lightmap"));
    if (sv != 0)
    {
      iTextureWrapper* tex;
      sv->GetValue (tex);
      if (tex != 0)
        lms.Add (tex->QueryObject()->GetName());
      svc->RemoveVariable (sv);
    }
  }

  void Object::ParseMesh (iMeshWrapper *wrapper)
  {
    this->meshWrapper = wrapper;

    const csFlags& meshFlags = wrapper->GetFlags ();
    if (meshFlags.Check (CS_ENTITY_NOSHADOWS))
      objFlags.Set (OBJECT_FLAG_NOSHADOW);
    if (meshFlags.Check (CS_ENTITY_NOLIGHTING))
      objFlags.Set (OBJECT_FLAG_NOLIGHT);

    this->meshName = wrapper->QueryObject ()->GetName ();
    csRef<iObjectIterator> objiter = 
      wrapper->QueryObject ()->GetIterator();
    while (objiter->HasNext())
    {
      iObject* obj = objiter->Next();
      csRef<iKeyValuePair> kvp = 
        scfQueryInterface<iKeyValuePair> (obj);
      if (kvp.IsValid() && (strcmp (kvp->GetKey(), "lighter2") == 0))
      {
        if (!factory->lightPerVertex)
        {
          /* Disallow "disabling" of per-vertex lighting in an object when
           * it's enabled for the factory. */
          const char* vVertexlight = kvp->GetValue ("vertexlight");
          if (vVertexlight != 0)
            lightPerVertex = (strcmp (vVertexlight, "yes") == 0);
        }
      }
    }
  }

  void Object::SaveMesh (Sector* /*sector*/, iDocumentNode* node)
  {
    // Save out the object to the node
    csRef<iSaverPlugin> saver = 
      csQueryPluginClass<iSaverPlugin> (globalLighter->pluginManager,
      saverPluginName);      
    if (!saver) 
      saver = csLoadPlugin<iSaverPlugin> (globalLighter->pluginManager,
      saverPluginName);
    if (saver) 
    {
      // Write new mesh
      csRef<iDocumentNode> paramChild = node->GetNode ("params");
      saver->WriteDown(meshWrapper->GetMeshObject (), node, 0/*ssource*/);
      if (paramChild) 
      {
        // Move all nodes after the old params node to after the new params node
        csRef<iDocumentNodeIterator> nodes = node->GetNodes();
        while (nodes->HasNext())
        {
          csRef<iDocumentNode> child = nodes->Next();
          if (child->Equals (paramChild)) break;
        }
        // Skip <params>
        if (nodes->HasNext()) 
        {
          // Actual moving
          while (nodes->HasNext())
          {
            csRef<iDocumentNode> child = nodes->Next();
            if ((child->GetType() == CS_NODE_ELEMENT)
              && (strcmp (child->GetValue(), "params") == 0))
              break;
            csRef<iDocumentNode> newNode = node->CreateNodeBefore (
              child->GetType(), 0);
            CS::DocumentHelper::CloneNode (child, newNode);
            node->RemoveNode (child);
          }
        }
        node->RemoveNode (paramChild);
      }
    }
  }

  void Object::FreeNotNeededForLighting ()
  {
    meshWrapper.Invalidate();
  }

  void Object::FillLightmapMask (LightmapMaskArray& masks)
  {
    if (lightPerVertex) return;

    float totalArea = 0;

    // And fill it with data
    for (size_t i = 0; i < allPrimitives.GetSize(); i++)
    {
      LightmapMask &mask = masks[lightmapIDs[i]];
      PrimitiveArray::Iterator primIt = allPrimitives[i].GetIterator ();
      while (primIt.HasNext ())
      {
        const Primitive &prim = primIt.Next ();
        totalArea = (prim.GetuFormVector ()%prim.GetvFormVector ()).Norm ();
        float area2pixel = 1.0f / totalArea;

        int minu,maxu,minv,maxv;
        prim.ComputeMinMaxUV (minu,maxu,minv,maxv);
        uint findex = 0;

        // Go through all lightmap cells and add their element areas to the mask
        for (uint v = minv; v <= (uint)maxv;v++)
        {
          uint vindex = v * mask.width;
          for (uint u = minu; u <= (uint)maxu; u++, findex++)
          {
            const float elemArea = prim.GetElementAreas ().GetElementArea (findex);
            if (elemArea == 0) continue; // No area, skip

            mask.maskData[vindex+u] += elemArea * area2pixel; //Accumulate
          }
        } 
      }
    }

  }

  Object::LitColorArray* Object::GetLitColorsPD (Light* light)
  {
    LitColorArray* colors = litColorsPD->GetElementPointer (light);
    if (colors != 0) return colors;

    LitColorArray newArray;
    newArray.SetSize (litColors->GetSize(), csColor (0));
    litColorsPD->Put (light, newArray);
    return litColorsPD->GetElementPointer (light);
  }

  void Object::RenormalizeLightmapUVs (const LightmapPtrDelArray& lightmaps,
                                       csVector2* lmcoords)
  {
    if (lightPerVertex) return;

    BoolArray vertexProcessed;
    vertexProcessed.SetSize (vertexData.lightmapUVs.GetSize (), false);

    for (size_t p = 0; p < allPrimitives.GetSize (); p++)
    {
      const Lightmap* lm = lightmaps[lightmapIDs[p]];
      const float factorX = 1.0f / lm->GetWidth ();
      const float factorY = 1.0f / lm->GetHeight ();
      const PrimitiveArray& prims = allPrimitives[p];
      csSet<size_t> indicesRemapped;
      // Iterate over lightmaps and renormalize UVs
      for (size_t j = 0; j < prims.GetSize (); ++j)
      {
        //TODO make sure no vertex is used in several lightmaps.. 
        const Primitive &prim = prims.Get (j);
        const Primitive::TriangleType& t = prim.GetTriangle ();
        for (size_t i = 0; i < 3; ++i)
        {
          size_t index = t[i];
          if (!indicesRemapped.Contains (index))
          {
            const csVector2 &lmUV = vertexData.lightmapUVs[index];
            csVector2& outUV = lmcoords[index];
            outUV.x = (lmUV.x + 0.5f) * factorX;
            outUV.y = (lmUV.y + 0.5f) * factorY;
            indicesRemapped.AddNoTest (index);
          }
        }
      }
    }
  }

  csString Object::GetFileName() const
  {
    csString filename (meshName);
    filename.ReplaceAll ("\\", "_");
    filename.ReplaceAll ("/", "_"); 
    filename.ReplaceAll (" ", "_"); 
    filename.ReplaceAll (".", "_"); 
    return filename;
  }
}
