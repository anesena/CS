/*
    Copyright (C) 2001 by Jorrit Tyberghein

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

#include <string.h>
#include "cssysdef.h"
#include "csver.h"
#include "csutil/scf.h"
#include "csutil/util.h"
#include "csutil/refarr.h"
#include "reporter.h"

CS_IMPLEMENT_PLUGIN

SCF_IMPLEMENT_FACTORY (csReporter)

SCF_EXPORT_CLASS_TABLE (reporter)
  SCF_EXPORT_CLASS (csReporter, "crystalspace.utilities.reporter",
    "Reporting utility")
SCF_EXPORT_CLASS_TABLE_END

//-----------------------------------------------------------------------

class csReporterIterator : public iReporterIterator
{
public:
  csPDelArray<csReporterMessage> messages;
  int idx;

public:
  csReporterIterator ()
  {
    SCF_CONSTRUCT_IBASE (NULL);
    idx = 0;
  }

  SCF_DECLARE_IBASE;

  virtual ~csReporterIterator ()
  {
  }

  virtual bool HasNext ()
  {
    return idx < messages.Length ();
  }

  virtual void Next ()
  {
    idx++;
  }

  virtual int GetMessageSeverity () const
  {
    return messages[idx-1]->severity;
  }

  virtual const char* GetMessageId () const
  {
    return messages[idx-1]->id;
  }

  virtual const char* GetMessageDescription () const
  {
    return messages[idx-1]->description;
  }
};

SCF_IMPLEMENT_IBASE (csReporterIterator)
  SCF_IMPLEMENTS_INTERFACE (iReporterIterator)
SCF_IMPLEMENT_IBASE_END

//-----------------------------------------------------------------------

SCF_IMPLEMENT_IBASE (csReporter)
  SCF_IMPLEMENTS_INTERFACE (iReporter)
  SCF_IMPLEMENTS_EMBEDDED_INTERFACE (iComponent)
SCF_IMPLEMENT_IBASE_END

SCF_IMPLEMENT_EMBEDDED_IBASE (csReporter::eiComponent)
  SCF_IMPLEMENTS_INTERFACE (iComponent)
SCF_IMPLEMENT_EMBEDDED_IBASE_END

csReporter::csReporter (iBase *iParent)
{
  SCF_CONSTRUCT_IBASE (iParent);
  SCF_CONSTRUCT_EMBEDDED_IBASE (scfiComponent);
  object_reg = NULL;
  mutex = csMutex::Create (true);
}

csReporter::~csReporter ()
{
  Clear (-1);
}

bool csReporter::Initialize (iObjectRegistry *object_reg)
{
  csReporter::object_reg = object_reg;
  return true;
}

void csReporter::Report (int severity, const char* msgId,
  	const char* description, ...)
{
  va_list arg;
  va_start (arg, description);
  ReportV (severity, msgId, description, arg);
  va_end (arg);
}

void csReporter::ReportV (int severity, const char* msgId,
  	const char* description, va_list arg)
{
  char buf[4000];
  vsprintf (buf, description, arg);

  // To ensure thread-safety we first copy the listeners.
  csRefArray<iReporterListener> copy;
  {
    csScopedMutexLock lock (mutex);
    int i;
    for (i = 0 ; i < listeners.Length () ; i++)
    {
      iReporterListener* listener = listeners[i];
      copy.Push (listener);
    }
  }

  bool add_msg = true;
  int i;
  for (i = 0 ; i < copy.Length () ; i++)
  {
    iReporterListener* listener = copy[i];
    if (listener->Report (this, severity, msgId, buf))
    {
      add_msg = false;
      break;
    }
  }

  if (add_msg)
  {
    csReporterMessage* msg = new csReporterMessage ();
    msg->severity = severity;
    msg->id = csStrNew (msgId);
    msg->description = csStrNew (buf);
    csScopedMutexLock lock (mutex);
    messages.Push (msg);
  }
}

void csReporter::Clear (int severity)
{
  csScopedMutexLock lock (mutex);

  int i = 0;
  int len = messages.Length ();
  while (i < len)
  {
    csReporterMessage* msg = messages[i];
    if (severity == -1 || msg->severity == severity)
    {
      messages.Delete (i);
      len--;
    }
    else
    {
      i++;
    }
  }
}

void csReporter::Clear (const char* mask)
{
  csScopedMutexLock lock (mutex);
  int i = 0;
  int len = messages.Length ();
  while (i < len)
  {
    csReporterMessage* msg = messages[i];
    if (csGlobMatches (msg->id, mask))
    {
      messages.Delete (i);
      len--;
    }
    else
    {
      i++;
    }
  }
}

csPtr<iReporterIterator> csReporter::GetMessageIterator ()
{
  csScopedMutexLock lock (mutex);
  csReporterIterator* it = new csReporterIterator ();
  int i;
  for (i = 0 ; i < messages.Length () ; i++)
  {
    csReporterMessage* msg = new csReporterMessage ();
    msg->severity = messages[i]->severity;
    msg->id = csStrNew (messages[i]->id);
    msg->description = csStrNew (messages[i]->description);
    it->messages.Push (msg);
  }
  return csPtr<iReporterIterator> (it);
}

void csReporter::AddReporterListener (iReporterListener* listener)
{
  csScopedMutexLock lock (mutex);
  listeners.Push (listener);
}

void csReporter::RemoveReporterListener (iReporterListener* listener)
{
  csScopedMutexLock lock (mutex);
  int idx = listeners.Find (listener);
  if (idx != -1)
  {
    listeners.Delete (idx);
  }
}

bool csReporter::FindReporterListener (iReporterListener* listener)
{
  csScopedMutexLock lock (mutex);
  int idx = listeners.Find (listener);
  return idx != -1;
}

csReporterMessage::~csReporterMessage ()
{
  delete[] id;
  delete[] description;
}

