/*
    Copyright (C) 1998, 1999 by Nathaniel 'NooTe' Saint Martin
    Copyright (C) 1998, 1999 by Jorrit Tyberghein
    Written by Nathaniel 'NooTe' Saint Martin

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
#include "csutil/scf.h"
#include "isystem.h"

#include "ia3dapi.h"

#include "sndrdr.h"
#include "sndlstn.h"

IMPLEMENT_FACTORY(csSoundListenerA3D)

IMPLEMENT_IBASE(csSoundListenerA3D)
	IMPLEMENTS_INTERFACE(iSoundListener)
IMPLEMENT_IBASE_END;

csSoundListenerA3D::csSoundListenerA3D(iBase *piBase)
{
	CONSTRUCT_IBASE(piBase);
  fPosX = fPosY = fPosZ = 0.0;
  fDirTopX = fDirTopY = fDirTopZ = 0.0;
  fDirFrontX = fDirFrontY = fDirFrontZ = 0.0;
  fVelX = fVelY = fVelZ = 0.0;
  fDoppler = 1.0;
  fDistance = 1.0;
  fRollOff = 1.0;

  m_p3DAudioRenderer = NULL;
	m_p3DListener = NULL;
}

csSoundListenerA3D::~csSoundListenerA3D()
{
  DestroyListener();
}

int csSoundListenerA3D::CreateListener(iSoundRender * render)
{
  HRESULT hr;
  csSoundRenderA3D *renderA3D;

  if(!render) return E_FAIL;
  renderA3D = (csSoundRenderA3D *)render;

  m_p3DAudioRenderer = renderA3D->m_p3DAudioRenderer;
  
  if (!m_p3DAudioRenderer)
    return(E_FAIL);
  
  if ((hr = m_p3DAudioRenderer->QueryInterface(IID_IA3dListener, (void **) &m_p3DListener)) != S_OK)
  {
    return E_FAIL;
  }

  if ((hr = m_p3DAudioRenderer->SetDopplerScale(fDoppler)) != S_OK)
  {
    return E_FAIL;
  }
  
  if ((hr = m_p3DAudioRenderer->SetDistanceModelScale(fDistance)) != S_OK)
  {
    return E_FAIL;
  }
  
  return S_OK;
}

int csSoundListenerA3D::DestroyListener()
{
  HRESULT hr;
  
  if (m_p3DListener)
  {
    if ((hr = m_p3DListener->Release()) < S_OK)
      return(hr);
    
    m_p3DListener = NULL;
  }
  
  fDoppler = 1.0f;
  fDistance = 1.0f;
  fRollOff = 1.0f;

  return S_OK;
}

void csSoundListenerA3D::SetPosition(float x, float y, float z)
{
  fPosX = x; fPosY = y; fPosZ = z;

  m_p3DListener->SetPosition3f(fPosX, fPosY, fPosZ);
}

void csSoundListenerA3D::SetDirection(float fx, float fy, float fz, float tx, float ty, float tz)
{
  fDirFrontX = fx; fDirFrontY = fy; fDirFrontZ = fz;
  fDirTopX = tx; fDirTopY = ty; fDirTopZ = tz;

  m_p3DListener->SetOrientation6f(
	  fDirFrontX, fDirFrontY, fDirFrontZ,
	  fDirTopX, fDirTopY, fDirTopZ);
}

void csSoundListenerA3D::SetHeadSize(float size)
{
  fHeadSize = size;
}

void csSoundListenerA3D::SetVelocity(float x, float y, float z)
{
  fVelX = x; fVelY = y; fVelZ = z;
  if(!m_p3DListener) return;
  m_p3DListener->SetVelocity3f(fVelX, fVelY, fVelZ);
}

void csSoundListenerA3D::SetDopplerFactor(float factor)
{
  fDoppler = factor;
  if(!m_p3DAudioRenderer) return;
  m_p3DAudioRenderer->SetDopplerScale(fDoppler);
}

void csSoundListenerA3D::SetDistanceFactor(float factor)
{
  fDistance = factor;
  if(!m_p3DAudioRenderer) return;
m_p3DAudioRenderer->SetDistanceModelScale(fDistance);
}

void csSoundListenerA3D::SetRollOffFactor(float factor)
{
  fRollOff = factor;
}

void csSoundListenerA3D::SetEnvironment(SoundEnvironment env)
{
  Environment = env;

  if(m_p3DAudioRenderer)
  {
    switch(Environment)
    {
    case ENVIRONMENT_GENERIC:
      m_p3DAudioRenderer->SetEq(1.0);
      break;

    case ENVIRONMENT_UNDERWATER:
      m_p3DAudioRenderer->SetEq(0.3);
      break;

    // I need to define others audio environments....
    default:
      break;
    }
  }
}

void csSoundListenerA3D::GetPosition(float &x, float &y, float &z)
{
  x = fPosX; y = fPosY; z = fPosZ;
}

void csSoundListenerA3D::GetDirection(float &fx, float &fy, float &fz, float &tx, float &ty, float &tz)
{
  fx = fDirFrontX; fy = fDirFrontY; fz = fDirFrontZ;
  tx = fDirTopX; ty = fDirTopY; tz = fDirTopZ;
}

float csSoundListenerA3D::GetHeadSize()
{
 return fHeadSize;
}

void csSoundListenerA3D::GetVelocity(float &x, float &y, float &z)
{
  x = fVelX; y = fVelY; z = fVelZ;
}

float csSoundListenerA3D::GetDopplerFactor()
{
  return fDoppler;
}

float csSoundListenerA3D::GetDistanceFactor()
{
  return fDistance;
}

float csSoundListenerA3D::GetRollOffFactor()
{
  return fRollOff;

  return S_OK;
}

SoundEnvironment csSoundListenerA3D::GetEnvironment()
{
  return Environment;
}
