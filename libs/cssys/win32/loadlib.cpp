/*
    Copyright (C) 1998 by Jorrit Tyberghein

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
#include "cssys/csshlib.h"
#include "cssys/sysfunc.h"
#include "cssys/syspath.h"
#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif
#include "csutil/csstring.h"
#include "csutil/csstrvec.h"
#include "csutil/scfstr.h"
#include "csutil/scfstrv.h"
#include "csutil/util.h"
#include "csutil/xmltiny.h"
#include "iutil/document.h"
#include <windows.h>

static csStrVector ErrorMessages;

#define LOADLIBEX_FLAGS		    LOAD_WITH_ALTERED_SEARCH_PATH

csLibraryHandle csLoadLibrary (const char* iName)
{
  csLibraryHandle handle;
  DWORD errorCode;

  handle = LoadLibraryEx (iName, 0, LOADLIBEX_FLAGS);
  errorCode = GetLastError();

#ifdef __CYGWIN__
 // A load attempt might fail if the DLL depends implicitly upon some other
 // DLLs which reside in the Cygwin /bin directory.  To deal with this case, we
 // add the Cygwin /bin directory to the PATH environment variable and retry.
 if (handle == 0)
 {
   char *OLD_PATH = new char[4096];
   char *DLLDIR = new char[1024];
   GetEnvironmentVariable("PATH", OLD_PATH, 4096);
   if (cygwin_conv_to_win32_path ("/bin/",DLLDIR))
   {
     ErrorMessages.Push(csStrNew(
       "LoadLibraryEx() '/bin/' Cygwin/Win32 path conversion failed."));
     delete[] DLLDIR;
     delete[] OLD_PATH;
     return 0;
   }
   SetEnvironmentVariable("PATH", DLLDIR);
   handle = LoadLibraryEx (iName, 0, LOADLIBEX_FLAGS);
   errorCode = GetLastError();
   SetEnvironmentVariable("PATH", OLD_PATH);
   delete[] DLLDIR;
   delete[] OLD_PATH;
 }
#endif

  if (handle == 0)
  {
    char *buf;
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        0, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &buf, 0, 0);
    char *str = new char[strlen(buf) + strlen(iName) + 50];
    sprintf (str, "LoadLibraryEx('%s') error %d: %s",
	iName, (int)errorCode, buf);
    ErrorMessages.Push (str);
    LocalFree (buf);
    return 0;
  }

  typedef const char* (*pfnGetPluginCompiler)();
  pfnGetPluginCompiler get_plugin_compiler = 
    (pfnGetPluginCompiler) GetProcAddress ((HMODULE)handle, "plugin_compiler");
  if (!get_plugin_compiler)
  {
    const char *noPluginCompiler =
      "%s: DLL does not export \"plugin_compiler\".\n";
    char *msg = new char[strlen(noPluginCompiler) + strlen(iName)];
    sprintf (msg, noPluginCompiler, iName);
    ErrorMessages.Push (msg);
    FreeLibrary ((HMODULE)handle);
    return 0;
  }
  const char* plugin_compiler = get_plugin_compiler();
  if (strcmp(plugin_compiler, CS_COMPILER_NAME) != 0)
  {
    const char *compilerMismatch =
      "%s: plugin compiler mismatches app compiler: %s != "
      CS_COMPILER_NAME "\n";
    char *msg = new char[strlen(compilerMismatch) + strlen(iName) + 
      strlen(plugin_compiler)];
    sprintf (msg, compilerMismatch, iName, plugin_compiler);
    ErrorMessages.Push (msg);
    FreeLibrary ((HMODULE)handle);
    return 0;
  }

  return handle;
}

void* csGetLibrarySymbol(csLibraryHandle Handle, const char* Name)
{
  void *ptr = (void*)GetProcAddress ((HMODULE)Handle, Name);
  if (!ptr)
  {
    char *Name2;
    Name2 = new char [strlen (Name) + 2];
    strcpy (Name2, "_");
    strcat (Name2, Name);
    ptr = (void*)GetProcAddress ((HMODULE)Handle, Name2);
    delete [] Name2;
  }
  return ptr;
}

bool csUnloadLibrary (csLibraryHandle Handle)
{
#if defined(CS_EXTENSIVE_MEMDEBUG) && defined(COMP_VC)
  // Why not? - Because the source file information
  // for leaked objects would get lost
  return true;
#else
  return FreeLibrary ((HMODULE)Handle)!=0;
#endif
}

void csPrintLibraryError (const char *iModule)
{
  char *str;
  while((str = (char*)ErrorMessages.Pop()) != 0)
  {
    fprintf (stderr, "  %s", str);
    delete[] str;
  }
}

static void AppendStrVecString (iStrVector*& strings, const char* str)
{
  if (!strings)
  {
    strings = new scfStrVector ();
  }
  strings->Push (csStrNew (str));
}

static void AppendWin32Error (const char* text,
			      DWORD errorCode,
			      iStrVector*& strings)
{
  char *buf;

  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      0, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &buf, 0, 0);

  csString errorMsg;
  errorMsg.Format ("%s: %s [%.8x]", text, buf, 
    (unsigned)errorCode);
  AppendStrVecString (strings, errorMsg);
  LocalFree (buf);
}

static csRef<iString> InternalGetPluginMetadata (const char* fullPath, 
						 csRef<iDocument>& metadata,
						 iDocumentSystem* docsys)
{
  iString* result = 0;

  HMODULE hLibrary = LoadLibraryEx (fullPath, 0, 
    LOADLIBEX_FLAGS | LOAD_LIBRARY_AS_DATAFILE);
  if (hLibrary != 0)
  {
    HRSRC res = FindResource (hLibrary, MAKEINTRESOURCE (0x444d),
      RT_RCDATA);
    if (res != 0)
    {
      HGLOBAL resDataHandle = LoadResource (hLibrary, res);
      if (resDataHandle != 0)
      {
	LPVOID resData = LockResource (resDataHandle);
	if (resData != 0)
	{
	  csRef<iDocument> doc = docsys->CreateDocument();
	  char const* errmsg = doc->Parse ((char*)resData);
	  if (errmsg == 0)
	  {
	    metadata = doc;
	  }
	  else
	  {
	    metadata = 0;

	    csString errstr;
	    errstr.Format ("Error parsing metadata from %s: %s",
	      fullPath, errmsg);

	    result = new scfString (errstr);
	  }
	}
      }
    }

    FreeLibrary (hLibrary);
  }
  /*
    No error messages are emitted - this simply may not be a plugin...
    And "errors" would be confusing in this case.
    We can't really determine whether a DLL is intended to be a CS plugin
    or not.
    */

  return csPtr<iString> (result);
}

csRef<iString> csGetPluginMetadata (const char* fullPath, 
				    csRef<iDocument>& metadata)
{
  csRef<iDocumentSystem> docsys = csPtr<iDocumentSystem>
    (new csTinyDocumentSystem ());
  return InternalGetPluginMetadata (fullPath, metadata, docsys);
}

extern char* csGetConfigPath ();

void InternalScanPluginDir (iStrVector*& messages,
			    iDocumentSystem* docsys,
			    const char* dir, 
			    csRef<iStrVector>& plugins,
			    csRefArray<iDocument>& metadata,
			    bool recursive)
{
  csString filemask;
  filemask << dir << PATH_SEPARATOR << "*.*";

  WIN32_FIND_DATA findData;
  HANDLE hSearch = FindFirstFile (filemask, &findData);
  if (hSearch != INVALID_HANDLE_VALUE)
  {
    do
    {
      csString fullPath;
      fullPath << dir << PATH_SEPARATOR << findData.cFileName;

      if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
	if (recursive && (strcmp (findData.cFileName, ".") != 0)
	  && (strcmp (findData.cFileName, "..") != 0))
	{
	  iStrVector* subdirMessages = 0;

	  InternalScanPluginDir (subdirMessages, docsys, fullPath,
	    plugins, metadata, recursive);

	  if (subdirMessages != 0)
	  {
	    for (int i = 0; i < subdirMessages->Length(); i++)
	    {
	      AppendStrVecString (messages, subdirMessages->Get (i));
	    }

	    subdirMessages->DecRef();
	  }
	}
      }
      else
      {
	char* ext = strrchr (fullPath, '.');
        if (ext && (strcasecmp (ext, ".dll") == 0))
	{
	  csRef<iDocument> doc;
	  csRef<iString> error = InternalGetPluginMetadata (
	    fullPath, doc, docsys);
	  if (error == 0)
	  {
	    plugins->Push (csStrNew (fullPath));
	    metadata.Push (doc);
	  }
	  else
	  {
	    AppendStrVecString (messages, error->GetData ());
	  }
	}
      }
    }
    while (FindNextFile (hSearch, &findData));
    FindClose (hSearch);
  }
  else
  {
    DWORD errorCode = GetLastError();

    /*
      The plugin paths list also contains non-existent entries -
      don't emit an error for those.
     */
    if (errorCode != ERROR_PATH_NOT_FOUND)
    {
      AppendWin32Error ("FindFirst() call failed",
	errorCode,
	messages);
    }
  }
}

csRef<iStrVector> csScanPluginDir (const char* dir, 
				   csRef<iStrVector>& plugins,
				   csRefArray<iDocument>& metadata,
				   bool recursive)
{
  iStrVector* messages = 0;

  if (!plugins)
    plugins.AttachNew (new scfStrVector ());

  csRef<iDocumentSystem> docsys = csPtr<iDocumentSystem>
    (new csTinyDocumentSystem ());

  InternalScanPluginDir (messages, docsys, dir, plugins, metadata, 
    recursive);
	 
  return csPtr<iStrVector> (messages);
}

csRef<iStrVector> csScanPluginDirs (csPluginPaths* dirs, 
				    csRef<iStrVector>& plugins,
				    csRefArray<iDocument>& metadata)
{
  iStrVector* messages = 0;

  if (!plugins)
    plugins.AttachNew (new scfStrVector ());

  /*
    TinyXML documents hold references to the document system.
    So we have to create a new csTinyDocumentSystem (). Using just
    'csTinyDocumentSystem docsys' would result in a crash when the
    documents try to DecRef() to already destructed document system.
   */
  csRef<iDocumentSystem> docsys = csPtr<iDocumentSystem>
    (new csTinyDocumentSystem ());

  for (int i = 0; i < dirs->GetCount (); i++)
  {
    iStrVector* dirMessages = 0;
    InternalScanPluginDir (dirMessages, docsys, (*dirs)[i].path, 
      plugins, metadata, (*dirs)[i].scanRecursive);
    
    if (dirMessages != 0)
    {
      csString tmp;
      tmp.Format ("The following error(s) occured while scanning '%s':",
	(*dirs)[i].path);

      AppendStrVecString (messages, tmp);

      for (int j = 0; j < dirMessages->Length(); j++)
      {
	tmp.Format (" %s", dirMessages->Get (j));
	AppendStrVecString (messages, tmp);
      }
      dirMessages->DecRef();
    }
  }
	 
  return csPtr<iStrVector> (messages);
}

