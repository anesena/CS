/*
    Copyright (C) 2002 by Anders Stenberg
    Written by Anders Stenberg

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

#include "eflayer.h"

csEffectLayer::csEffectLayer ()
{
  SCF_CONSTRUCT_IBASE (NULL);
}

csEffectLayer::~csEffectLayer ()
{
}

iBase* csEffectLayer::GetRendererData ()
{
  return rendererData;
}

void csEffectLayer::SetRendererData (iBase* data)
{
  rendererData = data;
}

SCF_IMPLEMENT_IBASE( csEffectLayer )
  SCF_IMPLEMENTS_INTERFACE( iEffectLayer )
SCF_IMPLEMENT_IBASE_END

