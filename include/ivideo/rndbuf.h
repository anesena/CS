/*
    Copyright (C) 2002 by M�rten Svanfeldt
                          Anders Stenberg

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

#ifndef __CS_IVIDEO_RNDBUF_H__
#define __CS_IVIDEO_RNDBUF_H__

/** \file 
 * Render buffer interface
 */
 
/**
 * \addtogroup gfx3d
 * @{ */

#include "csutil/strset.h"

#include "ivideo/material.h"

class csVector3;
class csVector2;
class csColor;
class csReversibleTransform;
struct iTextureHandle;
struct iMaterialWrapper;


/**
 * Where the buffer is placed
 * CS_BUF_INDEX is special and only to be used by indexbuffers
 */
enum csRenderBufferType
{
  CS_BUF_DYNAMIC,
  CS_BUF_STATIC,
  CS_BUF_INDEX
};

/// Type of components
enum csRenderBufferComponentType
{
  CS_BUFCOMP_BYTE,
  CS_BUFCOMP_UNSIGNED_BYTE,
  CS_BUFCOMP_SHORT,
  CS_BUFCOMP_UNSIGNED_SHORT,
  CS_BUFCOMP_INT,
  CS_BUFCOMP_UNSIGNED_INT,
  CS_BUFCOMP_FLOAT,
  CS_BUFCOMP_DOUBLE
};

/**
  * Type of lock
  * CS_BUF_LOCK_NORMAL: Just get a point to the buffer, nothing special
  * CS_BUF_LOCK_RENDER: Special lock only to be used by renderer
  */
enum csRenderBufferLockType
{
  CS_BUF_LOCK_NOLOCK,
  CS_BUF_LOCK_NORMAL,
  CS_BUF_LOCK_RENDER
};

SCF_VERSION (iRenderBuffer, 0, 0, 2);

/**
 * This is a general buffer to be used by the renderer. It can ONLY be
 * created by the VB manager
 */
struct iRenderBuffer : public iBase
{
  /**
   * Lock the buffer to allow writing and give us a pointer to the data
   * The pointer will be 0 if there was some error
   */
  virtual void* Lock(csRenderBufferLockType lockType) = 0;

  /// Releases the buffer. After this all writing to the buffer is illegal
  virtual void Release() = 0;

  /// Gets the number of components per element
  virtual int GetComponentCount () const = 0;

  /// Gets the component type
  virtual csRenderBufferComponentType GetComponentType () const = 0;

  /// Returns wheter the buffer is discarded or not
  virtual bool IsDiscarded() const = 0;

  /// Set if buffer can be discarded or not
  virtual void CanDiscard(bool value) = 0;

  /// Get type of buffer (where it's located)
  virtual csRenderBufferType GetBufferType() const = 0;

  /// Get the size of the buffer (in bytes)
  virtual int GetSize() const = 0;

};

SCF_VERSION (iStreamSource, 0, 0, 1);

struct iStreamSource : public iBase
{
  /// Get a named buffer
  virtual iRenderBuffer* GetBuffer (csStringID name) = 0;
};

/** @} */

#endif // __CS_IVIDEO_RNDBUF_H__
