#ifndef __CS_OPENCL_QUEUE_IMPL_H__
#define __CS_OPENCL_QUEUE_IMPL_H__

#include <csutil/clprogram.h>
#include <csutil/clmemory.h>
#include <csutil/clconsts.h>
#include "library.h"

CS_PLUGIN_NAMESPACE_BEGIN(CL)
{
  struct KernelMetaData
  {
    enum // arg type
    {
      ARG_BUFFER    = 1,
      ARG_SAMPLER   = 2,
      ARG_IMAGE2D   = 3,
      ARG_IMAGE3D   = 4,
      ARG_DATA      = 5,
      ARG_TYPE_MASK = 7
    };

    enum // arg storage type
    {
      ARG_PRIVATE      = 0,
      ARG_GLOBAL       = 1 << 3,
      ARG_LOCAL        = 2 << 3,
      ARG_CONST        = 3 << 3,
      ARG_STORAGE_MASK = 3 << 3
    };

    enum // arg access type
    {
      ARG_READ  = 1 << 4,
      ARG_WRITE = 2 << 4,
      ARG_READ_WRITE = ARG_READ|ARG_WRITE
    };

    csString name;
    int* args;
    size_t argCount;

    //@@@todo: add required/preferred extensions/device type/...
  };

  class Kernel : public scfImplementation1<Kernel,iKernel>
  {
  public:
    Kernel(Library*, const KernelMetaData&);
    ~Kernel();

    bool SetArg(size_t id, const void* arg, size_t size);
    bool SetArg(size_t id, iMemoryObject*);

    bool SetDimension(size_t d)
    {
      dimension = d;

      // adjust array sizes
      workSize.SetSize(d);
      workOffset.SetSize(d);
      groupSize.SetSize(d);
    }

    void SetWorkSize(const size_t* sizes);
    void SetWorkOffset(const size_t* offsets);
    void SetGroupSize(const size_t* sizes);

    size_t GetDimension() const
    {
      return dimension;
    }

    size_t* GetWorkSize() const;
    size_t* GetWorkOffset() const;
    size_t* GetGroupSize() const;

    cl_kernel CreateHandle(Context*, csRefArray<Event>& eventList);

  private:
    csRef<Library> parent;
    const KernelMetaData& data;
    csArray<size_t> argsSize;
    csArray<void*> args;
    csRefArray<iMemoryObject> objArgs;
    size_t argCount;

    size_t dimension;
    csArray<size_t> workSize;
    csArray<size_t> workOffset;
    csArray<size_t> groupSize;

    bool SetInstanceArg(cl_kernel kernel, size_t id, const void* arg, size_t size);
    bool SetInstanceArg(cl_kernel kernel, size_t id, iMemoryObject*,
                        Context* c, csRefArray<Event>&);
  };
}
CS_PLUGIN_NAMESPACE_END(CL)

#endif
