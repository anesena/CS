/*
  Copyright (C) 2005 by Frank Richter
	    (C) 2005 by Jorrit Tyberghein

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
#include "csutil/blockallocator.h"
#include "csutil/set.h"
#include "csutil/sysfunc.h"
#include "csutil/util.h"
#include "csutil/xmltiny.h"
#include "imap/services.h"
#include "ivaria/reporter.h"
#include "iutil/vfs.h"
#include "iutil/document.h"

#include "docwrap_replacer.h"

//---------------------------------------------------------------------------

CS_PLUGIN_NAMESPACE_BEGIN(XMLShader)
{

csReplacerDocumentNode::csReplacerDocumentNode (iDocumentNode* wrappedNode,
  csReplacerDocumentNode* parent, csReplacerDocumentNodeFactory* shared, 
  Substitutions* subst) : scfPooledImplementationType (this), 
  wrappedNode (wrappedNode), parent (parent), shared (shared), subst (subst)
{
  shared->Substitute (wrappedNode->GetValue(), value, *this->subst);
}

csReplacerDocumentNode::~csReplacerDocumentNode ()
{
}

bool csReplacerDocumentNode::Equals (iDocumentNode* other)
{
  return ((csReplacerDocumentNode*)other)->Equals (wrappedNode);
}

csRef<iDocumentNodeIterator> csReplacerDocumentNode::GetNodes ()
{
  csReplacerDocumentNodeIterator* iter = 
    new (shared->iterPool) csReplacerDocumentNodeIterator (this);
  return csPtr<iDocumentNodeIterator> (iter);
}

csRef<iDocumentNode> csReplacerDocumentNode::GetNode (
  const char* value)
{
  csRef<iDocumentNode> retNode = wrappedNode->GetNode (value);
  if (!retNode.IsValid()) return 0;
  return shared->CreateWrapper (retNode, this, subst);
}

csRef<iDocumentAttributeIterator> 
csReplacerDocumentNode::GetAttributes ()
{
  csReplacerDocumentAttributeIterator* iter = 
    new (shared->attrIterPool) csReplacerDocumentAttributeIterator (this);
  return csPtr<iDocumentAttributeIterator> (iter);
}

CS_IMPLEMENT_STATIC_VAR (ReplacerAttrAlloc, 
                         csBlockAllocator<csReplacerDocumentAttribute>, ())

csRef<iDocumentAttribute> csReplacerDocumentNode::GetAttribute (
  const char* name)
{
  csRef<iDocumentAttribute> wrappedAttr = attrCache.Get (name, 0);
  if (wrappedAttr.IsValid()) return wrappedAttr;

  wrappedAttr = wrappedNode->GetAttribute (name);
  if (!wrappedAttr.IsValid()) return 0;
  //csReplacerDocumentAttribute* attr = 
    //new (shared->attrPool) csReplacerDocumentAttribute (this, wrappedAttr);
  csReplacerDocumentAttribute* attr = ReplacerAttrAlloc()->Alloc ();
  attr->SetData (this, wrappedAttr);
  wrappedAttr.AttachNew (attr);
  attrCache.Put (name, wrappedAttr);
  return wrappedAttr;
}

//---------------------------------------------------------------------------

csReplacerDocumentNodeIterator::csReplacerDocumentNodeIterator (
  csReplacerDocumentNode* node) : scfPooledImplementationType (this), 
  node (node)
{
  wrappedIter = node->wrappedNode->GetNodes();
}

csReplacerDocumentNodeIterator::~csReplacerDocumentNodeIterator ()
{
}

bool csReplacerDocumentNodeIterator::HasNext ()
{
  if (!wrappedIter.IsValid()) return false;
  return wrappedIter->HasNext();
}

csRef<iDocumentNode> csReplacerDocumentNodeIterator::Next ()
{
  csRef<iDocumentNode> wrappedNode = wrappedIter->Next();
  if (!wrappedNode.IsValid()) return 0;
  return node->shared->CreateWrapper (wrappedNode, node, node->subst);
}

//---------------------------------------------------------------------------

csReplacerDocumentAttributeIterator::csReplacerDocumentAttributeIterator (
  csReplacerDocumentNode* node) : scfPooledImplementationType (this), 
  node (node)
{
  wrappedIter = node->wrappedNode->GetAttributes();
}

csReplacerDocumentAttributeIterator::~csReplacerDocumentAttributeIterator ()
{
}

bool csReplacerDocumentAttributeIterator::HasNext ()
{
  if (!wrappedIter.IsValid()) return false;
  return wrappedIter->HasNext();
}

csRef<iDocumentAttribute> csReplacerDocumentAttributeIterator::Next ()
{
  csRef<iDocumentAttribute> wrappedAttr = wrappedIter->Next();
  if (!wrappedAttr.IsValid()) return 0;
  /*csReplacerDocumentAttribute* attr = 
    new (node->shared->attrPool) csReplacerDocumentAttribute (
    node, wrappedAttr);*/
  csReplacerDocumentAttribute* attr = ReplacerAttrAlloc()->Alloc ();
  attr->SetData (node, wrappedAttr);
  return csPtr<iDocumentAttribute> (attr);
}

//---------------------------------------------------------------------------

csReplacerDocumentAttribute::csReplacerDocumentAttribute (
  /*csReplacerDocumentNode* node, iDocumentAttribute* wrappedAttr*/) : 
  /*scfPooledImplementationType (this)*/scfImplementationType (this)
{
  /*node->shared->Substitute (wrappedAttr->GetName(), name, *node->subst);
  node->shared->Substitute (wrappedAttr->GetValue(), val, *node->subst);*/
}

void csReplacerDocumentAttribute::DecRef ()
{
  CS_ASSERT_MSG("Refcount decremented for destroyed object", 
    scfRefCount != 0);
  csRefTrackerAccess::TrackDecRef (scfObject, scfRefCount);
  scfRefCount--;
  if (scfRefCount == 0)
  {
    scfRemoveRefOwners ();
    if (scfParent) scfParent->DecRef();
    ReplacerAttrAlloc()->Free (this);
  }
}

void csReplacerDocumentAttribute::SetData (csReplacerDocumentNode* node, 
                                           iDocumentAttribute* wrappedAttr)
{
  node->shared->Substitute (wrappedAttr->GetName(), name, *node->subst);
  node->shared->Substitute (wrappedAttr->GetValue(), val, *node->subst);
}

csReplacerDocumentAttribute::~csReplacerDocumentAttribute ()
{
}

//---------------------------------------------------------------------------

csReplacerDocumentNodeFactory::csReplacerDocumentNodeFactory ()
{
}

csRef<iDocumentNode> csReplacerDocumentNodeFactory::CreateWrapper (
  iDocumentNode* wrappedNode, csReplacerDocumentNode* parent, 
  Substitutions* subst)
{
  csReplacerDocumentNode* newNode = 
    new (nodePool) csReplacerDocumentNode (wrappedNode, parent, this, subst);
  return csPtr<iDocumentNode> (newNode);
}

void csReplacerDocumentNodeFactory::Substitute (const char* in, 
						csString& out, 
						const Substitutions& subst)
{
  out.Clear();
  if (in == 0) return;

  const char* p = in;
  while (*p != 0)
  {
    if (*p == '$')
    {
      p++;
      const char* varStart = p;
      while ((*p != '$') && (*p != 0)) p++;
      const char* varEnd = p;
      csString varName (varStart, varEnd-varStart);
      if (varName.IsEmpty())
	out << '$';
      else
	out << subst.Get (varName, ""); // @@@ FIXME: Error reporting
      if (*p == 0) break;
    }
    else
      out << *p;
    p++;
  }
}

}
CS_PLUGIN_NAMESPACE_END(XMLShader)
