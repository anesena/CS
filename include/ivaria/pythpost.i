/*
    Copyright (C) 2003 Rene Jager <renej_frog@users.sourceforge.net>

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

/*
	Python specific stuff for SWIG interface in post-include phase.

	See include/ivaria/cspace.i
*/

#ifdef SWIGPYTHON

#ifndef CS_MICRO_SWIG
%pythoncode %{

	CSMASK_Nothing = (1 << csevNothing)
	CSMASK_FrameProcess = CSMASK_Nothing
	CSMASK_Keyboard = (1 << csevKeyboard)
	CSMASK_MouseMove = (1 << csevMouseMove)
	CSMASK_MouseDown = (1 << csevMouseDown)
	CSMASK_MouseUp = (1 << csevMouseUp)
	CSMASK_MouseClick = (1 << csevMouseClick)
	CSMASK_MouseDoubleClick = (1 << csevMouseDoubleClick)
	CSMASK_JoystickMove = (1 << csevJoystickMove)
	CSMASK_JoystickDown = (1 << csevJoystickDown)
	CSMASK_JoystickUp = (1 << csevJoystickUp)
	CSMASK_Command = (1 << csevCommand)
	CSMASK_Broadcast = (1 << csevBroadcast)
	CSMASK_Network = (1 << csevNetwork)

	CSMASK_Mouse = (CSMASK_MouseMove | CSMASK_MouseDown | CSMASK_MouseUp
		| CSMASK_MouseClick | CSMASK_MouseDoubleClick)
	CSMASK_Joystick = (CSMASK_JoystickMove | CSMASK_JoystickDown
		| CSMASK_JoystickUp)
	CSMASK_Input = (CSMASK_Keyboard | CSMASK_Mouse | CSMASK_Joystick)

%}

/*
struct _csPyEventHandler : public iEventHandler
{
	SCF_DECLARE_IBASE;
	_csPyEventHandler (PyObject * obj);
	virtual ~_csPyEventHandler ();
	virtual bool HandleEvent (iEvent &);
};
*/

%inline %{

	struct _csPyEventHandler : public iEventHandler
	{
		SCF_DECLARE_IBASE;
		_csPyEventHandler (PyObject * obj) : _pySelf(obj)
		{
			SCF_CONSTRUCT_IBASE(0);
			IncRef();
		}
		virtual ~_csPyEventHandler ()
		{
			SCF_DESTRUCT_IBASE();
			DecRef();
		}
		virtual bool HandleEvent (iEvent & event)
		{
			PyObject * event_obj = SWIG_NewPointerObj(
				(void *) &event, SWIG_TypeQuery("iEvent *"), 0
			);
			PyObject * result = PyObject_CallMethod(
				_pySelf, "HandleEvent", "(O)", event_obj
			);
			Py_DECREF(event_obj);
			if (!result)
			{
				return false;
			}
			bool res = PyInt_AsLong(result);
			Py_DECREF(result);
			return res;
		}
	private:
		PyObject * _pySelf;
	};

%}

%{
	SCF_IMPLEMENT_IBASE(_csPyEventHandler)
	SCF_IMPLEMENT_IBASE_END
%}

%pythoncode %{

	class csPyEventHandler (_csPyEventHandler):
		"""Python version of iEventHandler implementation.
		   This class can be used as base class for event handlers in Python.
		   Call csPyEventHandler.__init__(self) in __init__ of derived class.
		"""
		def __init__ (self):
			_csPyEventHandler.__init__(self, self)

	class _EventHandlerFuncWrapper (csPyEventHandler):
		def __init__ (self, func):
			csPyEventHandler.__init__(self)
			self._func = func
			# Make sure a reference keeps to this wrapper instance.
			self._func._cs_event_handler_wrapper = self
		def HandleEvent (self, event):
			return self._func(event)

	def _csInitializer_SetupEventHandler (reg, obj,
			mask=(CSMASK_FrameProcess|CSMASK_Input|CSMASK_Broadcast)):
		"""Replacement of C++ versions."""
		if callable(obj):
			# obj is a function
			hdlr = _EventHandlerFuncWrapper(obj)
			hdlr.thisown = 1
		else:
			# assume it is a iEventHandler
			hdlr = obj
		return csInitializer._SetupEventHandler(reg, hdlr, mask)

	csInitializer.SetupEventHandler = staticmethod(_csInitializer_SetupEventHandler)

%}
#endif // CS_MICRO_SWIG

%pythoncode %{
	def _csInitializer_RequestPlugins (reg, plugins):
		"""Replacement of C++ version."""
		def _get_tuple (x):
			if callable(x):
				return tuple(x())
			else:
				return tuple(x)
		requests = csPluginRequestArray()
		for cls, intf, ident, ver in map(
				lambda x: _get_tuple(x), plugins):
			requests.Push(csPluginRequest(
				csString(cls), csString(intf), ident, ver))
		return csInitializer._RequestPlugins(reg, requests)

	csInitializer.RequestPlugins = staticmethod(_csInitializer_RequestPlugins)

%}

%inline %{

csWrapPtr _CS_QUERY_REGISTRY (iObjectRegistry *reg, const char *iface,
	int iface_ver)
{
  csRef<iBase> b;
  b.AttachNew(reg->Get
	(iface, iSCF::SCF->GetInterfaceID (iface), iface_ver));
  return csWrapPtr (iface, b);
}

csWrapPtr _CS_QUERY_REGISTRY_TAG_INTERFACE (iObjectRegistry *reg,
	const char *tag, const char *iface, int iface_ver)
{
  csRef<iBase> b;
  b.AttachNew(reg->Get
	(tag, iSCF::SCF->GetInterfaceID (iface), iface_ver));
  return csWrapPtr (iface, b);
}

csWrapPtr _SCF_QUERY_INTERFACE (iBase *obj, const char *iface, int iface_ver)
{
  return csWrapPtr (iface, obj->QueryInterface
	(iSCF::SCF->GetInterfaceID (iface), iface_ver));
}

csWrapPtr _SCF_QUERY_INTERFACE_SAFE (iBase *obj, const char *iface,
	int iface_ver)
{
  return csWrapPtr (iface, iBase::QueryInterfaceSafe
	(obj, iSCF::SCF->GetInterfaceID (iface), iface_ver));
}

csWrapPtr _CS_QUERY_PLUGIN_CLASS (iPluginManager *obj, const char *id,
	const char *iface, int iface_ver)
{
  return csWrapPtr (iface, obj->QueryPlugin (id, iface, iface_ver));
}

csWrapPtr _CS_LOAD_PLUGIN (iPluginManager *obj, const char *id,
	const char *iface, int iface_ver)
{
  return csWrapPtr (iface, obj->LoadPlugin (id, iface, iface_ver));
}

csWrapPtr _CS_GET_CHILD_OBJECT (iObject *obj, const char *iface, int iface_ver)
{
  return csWrapPtr (iface, obj->GetChild
	(iSCF::SCF->GetInterfaceID (iface), iface_ver));
}

csWrapPtr _CS_GET_NAMED_CHILD_OBJECT (iObject *obj, const char *iface,
	int iface_ver, const char *name)
{
  return csWrapPtr (iface, obj->GetChild
	(iSCF::SCF->GetInterfaceID (iface), iface_ver, name));
}

csWrapPtr _CS_GET_FIRST_NAMED_CHILD_OBJECT (iObject *obj, const char *iface,
	int iface_ver, const char *name)
{
  return csWrapPtr (iface, obj->GetChild
	(iSCF::SCF->GetInterfaceID (iface), iface_ver, name, true));
}

%}

#ifndef CS_MINI_SWIG
%pythoncode %{

	csReport = csReporterHelper.Report

%}
#endif

%pythoncode %{

	def _GetIntfId (intf):
		return cvar.iSCF_SCF.GetInterfaceID(intf.__name__)
	def _GetIntfVersion (intf):
		return eval('%s_scfGetVersion()' % intf.__name__, locals(), globals())

	def CS_QUERY_REGISTRY (reg, intf):
		return _CS_QUERY_REGISTRY (reg, intf.__name__, _GetIntfVersion(intf))

	def CS_QUERY_REGISTRY_TAG_INTERFACE (reg, tag, intf):
		return _CS_QUERY_REGISTRY_TAG_INTERFACE (reg, tag, intf.__name__,
			_GetIntfVersion(intf))

	def SCF_QUERY_INTERFACE (obj, intf):
		return _SCF_QUERY_INTERFACE (obj, intf.__name__, _GetIntfVersion(intf))

	def SCF_QUERY_INTERFACE_SAFE (obj, intf):
		return _SCF_QUERY_INTERFACE_SAFE(obj, intf.__name__,
			_GetIntfVersion(intf))

	def CS_GET_CHILD_OBJECT (obj, intf):
		return _CS_GET_CHILD_OBJECT(obj, intf.__name__, _GetIntfVersion(intf))

	def CS_GET_NAMED_CHILD_OBJECT (obj, intf, name):
		return _CS_GET_NAMED_CHILD_OBJECT(obj, intf.__name__,
			_GetIntfVersion(intf), name)

	def CS_GET_FIRST_NAMED_CHILD_OBJECT (obj, intf, name):
		return CS_GET_FIRST_NAMED_CHILD_OBJECT (obj, intf.__name__,
			_GetIntfVersion(intf), name)

	def CS_QUERY_PLUGIN_CLASS (obj, class_id, intf):
		return _CS_QUERY_PLUGIN_CLASS(obj, class_id, intf.__name__,
			_GetIntfVersion(intf))

	def CS_LOAD_PLUGIN (obj, class_id, intf):
		return _CS_LOAD_PLUGIN(obj, class_id, intf.__name__,
			_GetIntfVersion(intf))

	def CS_REQUEST_PLUGIN (name, intf):
		return (
			name, intf.__name__, cvar.iSCF_SCF.GetInterfaceID(intf.__name__),
			eval('%s_scfGetVersion()' % intf.__name__, locals(), globals())
		)

	def CS_REQUEST_VFS ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.kernel.vfs", iVFS
		)

	def CS_REQUEST_FONTSERVER ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.font.server.default", iFontServer
		)

	def CS_REQUEST_IMAGELOADER ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.graphic.image.io.multiplex", iImageIO
		)

	def CS_REQUEST_NULL3D ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.graphics3d.null", iGraphics3D
		)

	def CS_REQUEST_SOFTWARE3D ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.graphics3d.software", iGraphics3D
		)

	def CS_REQUEST_OPENGL3D ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.graphics3d.opengl", iGraphics3D
		)

	def CS_REQUEST_ENGINE ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.engine.3d", iEngine
		)

	def CS_REQUEST_LEVELLOADER ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.level.loader", iLoader
		)

	def CS_REQUEST_LEVELSAVER ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.level.saver", iSaver
		)

	def CS_REQUEST_REPORTER ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.utilities.reporter", iReporter
		)

	def CS_REQUEST_REPORTERLISTENER ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.utilities.stdrep", iStandardReporterListener
		)

	def CS_REQUEST_CONSOLEOUT ():
		return CS_REQUEST_PLUGIN(
			"crystalspace.console.output.simple", iConsoleOutput
		)

%}

#ifndef CS_MINI_SWIG
%extend iCollideSystem
{
	%rename(_GetCollisionPairs) GetCollisionPairs;

	%pythoncode %{
		def GetCollisionPairs (self):
			num = self.GetCollisionPairCount()
			pairs = []
			for i in range(num):
				pairs.append(self.GetCollisionPairByIndex(i))
			return pairs
	%}

}

%define VECTOR_PYTHON_OBJECT_FUNCTIONS(N)
	csVector ## N __rmul__ (float f) const
		{ return f * *self; }
	float __abs__ () const
		{ return self->Norm(); }
%enddef

%extend csVector2
{
	VECTOR_PYTHON_OBJECT_FUNCTIONS(2)

	float __getitem__ (int i) const
		{ return i ? self->y : self->x; }
	void __setitem__ (int i, float v)
		{ if (i) self->y = v; else self->x = v; }
}

%extend csVector3
{
	VECTOR_PYTHON_OBJECT_FUNCTIONS(3)

	float __getitem__ (int i) const
		{ return self->operator[](i); }
	void __setitem__ (int i, float v)
		{ self->operator[](i) = v; }
	bool __nonzero__ () const
		{ return !self->IsZero(); }
}

%extend csMatrix3
{
	csMatrix3 __rmul__ (float f)
		{ return f * *self; }
}

%define CSPOLY_PYTHON_OBJECT_FUNCTIONS(N)
	csVector ## N & __getitem__ (int i)
		{ return self->operator[](i); }
	%pythoncode %{
		def __setitem__ (self, i, v):
			own_v = self.__getitem__(i)
			for i in range(N):
				own_v[i] = v[i]
	%}
%enddef

%extend csPoly3D
{
	CSPOLY_PYTHON_OBJECT_FUNCTIONS(3)
}

%extend csPoly2D
{
	CSPOLY_PYTHON_OBJECT_FUNCTIONS(2)
}

%extend csTransform
{
	csVector3 __rmul__ (const csVector3 & v) const
		{ return v * *self; } 
	csPlane3 __rmul__ (const csPlane3 & p) const
		{ return p * *self; } 
	csSphere __rmul__ (const csSphere & s) const
		{ return s * *self; } 
}

%extend csColor
{
	csColor __rmul__ (float f) const
		{ return f * *self; }
}

// iutil/string.h
%extend iString
{
	char __getitem__ (size_t i) const
		{ return self->GetAt(i); }
	void __setitem__ (size_t i, char c)
		{ self->SetAt(i, c); }
}
#endif // CS_MINI_SWIG

// csutil/csstring.h
%extend csString
{
	char __getitem__ (size_t i) const
		{ return self->operator[](i); }
	void __setitem__ (size_t i, char c)
		{ self->operator[](i) = c; }
	void __delitem__ (size_t i)
		{ self->DeleteAt(i); }
}

#ifndef CS_MINI_SWIG
%pythoncode %{

	CS_VEC_FORWARD = csVector3(0,0,1)
	CS_VEC_BACKWARD = csVector3(0,0,-1)
	CS_VEC_RIGHT = csVector3(1,0,0)
	CS_VEC_LEFT = csVector3(-1,0,0)
	CS_VEC_UP = csVector3(0,1,0)
	CS_VEC_DOWN = csVector3(0,-1,0)
	CS_VEC_ROT_RIGHT = csVector3(0,1,0)
	CS_VEC_ROT_LEFT = csVector3(0,-1,0)
	CS_VEC_TILT_RIGHT = -csVector3(0,0,1)
	CS_VEC_TILT_LEFT = -csVector3(0,0,-1) 
	CS_VEC_TILT_UP = -csVector3(1,0,0)
	CS_VEC_TILT_DOWN = -csVector3(-1,0,0)

%}

%pythoncode %{
        CS_POLYRANGE_LAST = csPolygonRange (-1, -1)
%}
#endif // CS_MINI_SWIG

/*
 csWrapTypedObject is used to wrap a c++ object and pass it around as a Python 
 object. As an example think of passing the iObjectRegistry* from the main c++
 program to your python code.
*/


%{

#ifdef __cplusplus
extern "C" {
#endif

SWIGRUNTIME(PyObject *) csWrapTypedObject (void* objectptr, const char *typetag, int own)
{
  swig_type_info *ti = SWIG_TypeQuery (typetag);
  PyObject *obj = SWIG_NewPointerObj (objectptr, ti, own);
  return obj;
}
#ifdef __cplusplus
}
#endif

%}

%include "ivaria/pythvarg.i"

#endif // SWIGPYTHON

