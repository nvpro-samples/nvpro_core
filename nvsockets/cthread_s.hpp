/*-----------------------------------------------------------------------
    Copyright (c) 2016, NVIDIA. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Neither the name of its contributors may be used to endorse 
       or promote products derived from this software without specific
       prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    feedback to tlorach@nvidia.com (Tristan Lorach)
*/ //--------------------------------------------------------------------
#pragma once

#include <vector>
#include <algorithm>

#define COLOR_RED     0xFFFF0000
#define COLOR_RED2    0xFFAA0000
#define COLOR_ORANGE  0xFFFFA040
#define COLOR_GREEN   0xFF00FF00
#define COLOR_GREEN2  0xFF00AA00
#define COLOR_GREEN3  0xFFB6FF00
#define COLOR_BLUE    0xFF0000FF
#define COLOR_BLUE2   0xFF0000AA
#define COLOR_YELLOW  0xFFFFFF00
#define COLOR_YELLOW2 0xFFAAAA00
#define COLOR_MAGENTA 0xFFFF00FF
#define COLOR_MAGENTA2 0xFFAA00AA
#define COLOR_CYAN    0xFF00FFFF
#define COLOR_CYAN2   0xFF00AAAA

#if defined ANDROID
#define __forceinline inline
#endif

#if defined IOS || defined ANDROID
  #include <stdint.h>
  typedef unsigned char uchar;
  typedef uint16_t  ushort;
  typedef uint32_t  uint;
  typedef uint32_t  uint32;
  typedef uint64_t  uint64;
  typedef int32_t   int32;
  typedef int64_t   int64;
#else
  typedef unsigned char     uchar;
  typedef unsigned short    ushort;
  typedef unsigned int      uint;
  typedef unsigned __int32  uint32;
  typedef unsigned __int64  uint64;
  typedef signed   __int32  int32;
  typedef signed   __int64  int64;
#endif

#if defined IOS || defined ANDROID
#include <pthread.h>
#include <semaphore.h>
#endif
#ifdef WIN32
#include <windows.h>
#ifdef NVP_SUPPORTS_NVTOOLSEXT
//#    include "nvToolsExt.h"
//#    define NX_RANGE nvtxRangeId_t
//#    define NX_MARK(name) nvtxMark(name)
//#    define NX_RANGESTART(name) nvtxRangeStart(name)
//#    define NX_RANGEEND(id) nvtxRangeEnd(id)
//#    define NX_RANGEPUSH(name) nvtxRangePush(name)
//#    define NX_RANGEPUSHCOL(name, c) {\
//            nvtxEventAttributes_t eventAttrib = {0};\
//            eventAttrib.version = NVTX_VERSION;\
//            eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;\
//            eventAttrib.colorType = NVTX_COLOR_ARGB;\
//            eventAttrib.color = c;\
//            eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;\
//            eventAttrib.message.ascii = name;\
//            nvtxRangePushEx(&eventAttrib);\
//            }
//#    define NX_RANGEPOP() nvtxRangePop()
//    struct NXProfileFunc
//    {
//        NXProfileFunc(const char *name, uint32_t c, /*int64_t*/uint32_t p = 0)
//        {
//            nvtxEventAttributes_t eventAttrib = {0};
//            // set the version and the size information
//            eventAttrib.version = NVTX_VERSION;
//            eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
//            // configure the attributes.  0 is the default for all attributes.
//            eventAttrib.colorType = NVTX_COLOR_ARGB;
//            eventAttrib.color = c;
//            eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
//            eventAttrib.message.ascii = name;
//            eventAttrib.payloadType = NVTX_PAYLOAD_TYPE_INT64;
//            eventAttrib.payload.llValue = (int64_t)p;
//            eventAttrib.category = (uint32_t)p;
//            nvtxRangePushEx(&eventAttrib);
//        }
//        ~NXProfileFunc()
//        {
//            nvtxRangePop();
//        }
//    };
//#   ifdef NXPROFILEFUNC
//#   undef NXPROFILEFUNC
//#    undef NXPROFILEFUNCCOL
//#    undef NXPROFILEFUNCCOL2
//#   endif
//#    define NXPROFILEFUNC(name) NXProfileFunc nxProfileMe(name, 0xFF0000FF)
//#    define NXPROFILEFUNCCOL(name, c) NXProfileFunc nxProfileMe(name, c)
//#    define NXPROFILEFUNCCOL2(name, c, p) NXProfileFunc nxProfileMe(name, c, p)
#include "nvh/nsightevents.h"
#else
#    define NX_RANGE int
#    define NX_MARK(name)
#    define NX_RANGESTART(name) 0
#    define NX_RANGEEND(id)
#    define NX_RANGEPUSH(name)
#    define NX_RANGEPUSHCOL(name, c)
#    define NX_RANGEPOP()
#    define NXPROFILEFUNC(name)
#    define NXPROFILEFUNCCOL(name, c)
#    define NXPROFILEFUNCCOL2(name, c, a)
#endif
#endif

namespace sock
{

#if defined IOS || defined ANDROID
typedef pthread_t                         NThreadHandle;
typedef pthread_t                         NThreadID;
typedef pthread_cond_t                    NEventHandle;
typedef sem_t                             NSemaphoreHandle;
#define NCROSS_THREAD_INVOKE_CALL_CONV           
#define NTHREAD_PROC_CALL_CONV                   
typedef long                              NInterlockedValue;
//NOTE: pthread_mutex is much slower than Win32 Critical sections... to see
typedef pthread_mutex_t                   NMutexHandle;

#    define NX_RANGE int
#    define NX_MARK(name)
#    define NX_RANGESTART(name) 0
#    define NX_RANGEEND(id)
#    define NX_RANGEPUSH(name)
#    define NX_RANGEPUSHCOL(name, c)
#    define NX_RANGEPOP()
#    define NXPROFILEFUNC(name)
#    define NXPROFILEFUNCCOL(name, c)
#    define NXPROFILEFUNCCOL2(name, c, a)

#endif
#ifdef WIN32
typedef HANDLE                            NThreadHandle;
typedef DWORD                             NThreadID;
typedef HANDLE                            NEventHandle;
typedef HANDLE                            NSemaphoreHandle;
#define NCROSS_THREAD_INVOKE_CALL_CONV    CALLBACK       
#define NTHREAD_PROC_CALL_CONV            __stdcall       
typedef __declspec(align(4)) LONG         NInterlockedValue;
//NOTE pthread_mutex is much slower than Win32 Critical sections... to see
typedef HANDLE                            NMutexHandle;
#endif // _WIN32


extern void thread_function(void *pData);
/******************************************************************************/
/**
 ** CThread class
 **/
class CThread 
{
public:
    typedef void (*ThreadProc)(void *);

protected:
    NThreadHandle m_thread;
    NThreadHandle CreateThread(ThreadProc Proc, void *Param, bool startNow = true, const bool Critical = false);
public:
    CThread(const bool startNow = true, const bool Critical = false);
    ~CThread();

    // Methods
    void CancelThread();
    void DeleteThread();
    void WaitThread();
    bool SuspendThread();
    bool ResumeThread();
    void SetThreadAffinity(unsigned int mask);
    NThreadHandle GetHandle() { return m_thread; }

    // To implement
    virtual void CThreadProc() = 0;
    //{
    //    LOGW("Ooops\n");
    //};

    // static methods
    static int CpuCount();
    static void Sleep(const unsigned long Milliseconds);
    static void WaitThreads(const NThreadHandle *Threads, const int Count);

}; // Thread

/******************************************************************************/
/**
 ** \brief Mutex Class
 **/
class CMutex
{
public:
    CMutex();
    ~CMutex();

    bool LockMutex(int ms=-1, long *dbg=NULL);
    void UnlockMutex();

protected:
    static void CreateMutex(NMutexHandle &Mutex);
    void DeleteMutex();

    NMutexHandle m_mutex;

};

/******************************************************************************/
/**
 ** \brief Critical section implementation. For now deriving from Mutex
 ** \todo Implement the Windows version (more efficient). For now just inherits from CMutex
 **/
class CCriticalSection : public CMutex
{
public:
    CCriticalSection() : CMutex() {}
    void Enter()
    {
        LockMutex();
    }
    bool TryEnter()
    {
        return LockMutex(0);
    }
    void Exit()
    {
        UnlockMutex();
    }
};
/******************************************************************************/
/**
 ** \brief Critical section holder : construct/destruct used to lock/unlock the section
 **/
class CCriticalSectionHolder
{
private:
    
    CCriticalSection& m_sec;
public:
    CCriticalSectionHolder(CCriticalSectionHolder&); //these are purposely not implemented
    CCriticalSectionHolder& operator= (CCriticalSectionHolder&);
    inline CCriticalSectionHolder(CCriticalSection& s) : m_sec(s) { m_sec.Enter(); }
    inline ~CCriticalSectionHolder() { m_sec.Exit(); }
};

/******************************************************************************/
/**
 ** \brief Semaphore
 ** The semaphore is working this way :
 ** - AcquireSemaphore decrements the semaphore and returns. If ever the semaphore was 0,
 **   the call will not return untill ms exceeded or until some other thread Released it
 ** - ReleaseSemaphore increments the semaphore. This would allow some other threads to get unlocked...
 **  
 **/
class CSemaphore
{
public:
    CSemaphore(long initialCnt, long maxCnt=0xFFFF);
    CSemaphore();
    ~CSemaphore();

    bool AcquireSemaphore(int ms=-1);
    void ReleaseSemaphore(long cnt=1);

protected:
    static int    num_Semaphores;
    static void CreateSemaphore(NSemaphoreHandle &Semaphore, long initialCnt, long maxCnt);
    void DeleteSemaphore();

    NSemaphoreHandle m_semaphore;
};

/******************************************************************************/
/**
 ** \brief Event class
 **/
class CEvent
{
public:
    CEvent(bool manualReset=false, bool initialState=false);
    ~CEvent();

    bool Set();
    bool Pulse();
    bool Reset();
    bool WaitOnEvent(int ms=-1);
    NEventHandle GetHandle() { return m_event; }

protected:
    static int    num_events;
    static void CreateEvent(NEventHandle &Event, bool manualReset, bool initialState);
    void DeleteEvent();

    NEventHandle m_event;
#if defined IOS || defined ANDROID
    // pthread is retarded and their condition signaling involves both a condition variable and a mutex
    mutable pthread_mutex_t m_mutex;
    mutable bool m_signaled;
    mutable bool m_manualReset;
#endif // _WIN32

};

#ifdef WIN32

//for some bizzarre reason MS doesn't define all of these on x86...

/******************************************************************************/
/**
 ** \brief Atomic for integer
 **/
__declspec(align(4))
class NAtomicInt
{
  volatile long m_value;
public:
  __forceinline NAtomicInt(int32 v) : m_value(v) {}
#ifdef WIN64 //TO Check which Define to use for 64bits
  __forceinline int32 Add(int32 v) { return InterlockedAdd(&m_value, v); }
  __forceinline int32 Subtract(int32 v) { return InterlockedAdd(&m_value, -v); }
  __forceinline int32 And(int32 v) { return InterlockedAnd(&m_value, v); }
  __forceinline int32 Or(int32 v) { return InterlockedOr(&m_value, v); }
  __forceinline int32 Xor(int32 v) { return InterlockedXor(&m_value, v); }
#else
  __forceinline int32 Add(int32 v) { return InterlockedExchangeAdd(&m_value, v) + v; }
  __forceinline int32 Subtract(int32 v) { return InterlockedExchangeAdd(&m_value, -v) - v; }
  //__forceinline int32 And(int32 v) { return _InterlockedAnd(&m_value, v); }
  //__forceinline int32 Or(int32 v) { return _InterlockedOr(&m_value, v); }
  //__forceinline int32 Xor(int32 v) { return _InterlockedXor(&m_value, v); }

#endif

  __forceinline int32 Increment() { return InterlockedIncrement(&m_value); }
  __forceinline int32 Decrement() { return InterlockedDecrement(&m_value); }
  __forceinline int32 Exchange(int32 v) { return InterlockedExchange(&m_value, v); }
  //if compareValue == curValue, curValue = v, return prevValue
  __forceinline int32 CmpExchange(int32 v, int32 compareValue) { return InterlockedCompareExchange(&m_value, v, compareValue); }
  //same as add, but returns the previous value
  __forceinline int32 ExchangeAdd(int32 v) { return InterlockedExchangeAdd(&m_value, v); }

  //you can't edit the value using this, only read it
  __forceinline operator int32() { return m_value; }
};

/******************************************************************************/
/**
 ** \brief Atomic for 64 bits int
 **/
__declspec(align(8))
class NAtomicInt64
{
  volatile __int64  m_value;
public:
  __forceinline NAtomicInt64(int64 v) : m_value(v) {}
#ifdef WIN64
  __forceinline int64 Add(int64 v) { return InterlockedAdd64(&m_value, v); }
  __forceinline int64 Subtract(int64 v) { return InterlockedAdd64(&m_value, -v); }
#else
  __forceinline int64 Add(int64 v) { return InterlockedExchangeAdd64(&m_value, v) + v; }
  __forceinline int64 Subtract(int64 v) { return InterlockedExchangeAdd64(&m_value, -v) - v; }
#endif
  __forceinline int64 Increment() { return InterlockedIncrement64(&m_value); }
  __forceinline int64 Decrement() { return InterlockedDecrement64(&m_value); }
  __forceinline int64 And(int64 v) { return InterlockedAnd64(&m_value, v); }
  __forceinline int64 Or(int64 v) { return InterlockedOr64(&m_value, v); }
  __forceinline int64 Xor(int64 v) { return InterlockedXor64(&m_value, v); }
  __forceinline int64 Exchange(int64 v) { return InterlockedExchange64(&m_value, v); }
  //if compareValue == curValue, curValue = v, return prevValue
  __forceinline int64 CmpExchange(int64 v, int64 compareValue) { return InterlockedCompareExchange64(&m_value, v, compareValue); }
  //same as add, but returns the previous value
  __forceinline int64 ExchangeAdd(int64 v) { return InterlockedExchangeAdd64(&m_value, v); }

  //you can't edit the value using this, only read it
  __forceinline operator int64() { return m_value; }
};

#else

/******************************************************************************/
/**
 ** \brief Atomic for Int
 **/
class NAtomicInt
{
  volatile long m_value;
public:
  __forceinline NAtomicInt(int32 v) : m_value(v) {}
  __forceinline int32 Add(int32 v) { return __sync_add_and_fetch(&m_value, v); }
  __forceinline int32 Subtract(int32 v) { return __sync_sub_and_fetch (&m_value, v); }
  __forceinline int32 Increment() { return __sync_add_and_fetch(&m_value, 1); }
  __forceinline int32 Decrement() { return __sync_sub_and_fetch (&m_value, 1); }
  __forceinline int32 And(int32 v) { return __sync_and_and_fetch(&m_value, v); }
  __forceinline int32 Or(int32 v) { return __sync_or_and_fetch(&m_value, v); }
  __forceinline int32 Xor(int32 v) { return __sync_xor_and_fetch(&m_value, v); }
  __forceinline int32 Exchange(int32 v) { return __sync_lock_test_and_set(&m_value, v); }
  //if compareValue == curValue, curValue = v, return prevValue
  __forceinline int32 CmpExchange(int32 v, int32 compareValue) { return __sync_val_compare_and_swap(&m_value, compareValue, v); }
  //same as add, but returns the previous value
  __forceinline int32 ExchangeAdd(int32 v) { return __sync_fetch_and_add(&m_value, v); }

  //you can't edit the value using this, only read it
  __forceinline operator int32() { return m_value; }
} __attribute__ ((aligned (4)));

/******************************************************************************/
/**
 ** \brief Atomic for 64 bits Int
 **/
class NAtomicInt64
{
  volatile long long  m_value;
public:
  __forceinline NAtomicInt64(int64 v) : m_value(v) {}
  __forceinline int64 Add(int64 v) { return __sync_add_and_fetch(&m_value, v); }
  __forceinline int64 Subtract(int64 v) { return __sync_sub_and_fetch (&m_value, v); }
  __forceinline int64 Increment() { return __sync_add_and_fetch(&m_value, 1); }
  __forceinline int64 Decrement() { return __sync_sub_and_fetch (&m_value, 1); }
  __forceinline int64 And(int64 v) { return __sync_and_and_fetch(&m_value, v); }
  __forceinline int64 Or(int64 v) { return __sync_or_and_fetch(&m_value, v); }
  __forceinline int64 Xor(int64 v) { return __sync_xor_and_fetch(&m_value, v); }
  __forceinline int64 Exchange(int64 v) { return __sync_lock_test_and_set(&m_value, v); }
  //if compareValue == curValue, curValue = v, return prevValue
  __forceinline int64 CmpExchange(int64 v, int64 compareValue) { return __sync_val_compare_and_swap(&m_value, compareValue, v); }
  //same as add, but returns the previous value
  __forceinline int64 ExchangeAdd(int64 v) { return __sync_fetch_and_add(&m_value, v); }

  //you can't edit the value using this, only read it
  __forceinline operator int64() { return m_value; }
} __attribute__ ((aligned (8)));

#endif

//this can't be a wrapper function due to the weird semantics of the barrier (it looks at locals for example)
//do a software barrier (asm, MemoryBarrier) to prevent the compiler reordering reads/writes
//and do a hardware barrier
#ifdef WIN32
#define NMemoryBarrier() (MemoryBarrier(), _ReadWriteBarrier())
#define NMemoryBarrierSW() MemoryBarrier()
#define NMemoryBarrierHW() _ReadWriteBarrier()
#else
#define NMemoryBarrier() { asm volatile("" ::: "memory"); __sync_synchronize(); }
#define NMemoryBarrierSW() asm volatile("" ::: "memory")
#define NMemoryBarrierHW() __sync_synchronize()
#endif

//////////////////////////////////////////////////////////////////////////////////////////////

#define NV_NO_DECLSPEC_THREAD


#if defined(_MSC_VER)
  #define NV_DECLSPEC_THREAD __declspec(thread)
#elif defined(__GNUC__)
  #define NV_DECLSPEC_THREAD __thread
#else
  #ifndef NV_NO_DECLSPEC_THREAD
    #error Cannot define NV_DECLSPEC_THREAD
  #endif //NV_NO_DECLSPEC_THREAD
#endif

class NThreadLocalNonPODBase
{
private:
  static CCriticalSection& s_listLock();
  static std::vector<NThreadLocalNonPODBase*>& s_tlsList();
  virtual void DeleteMyTLSData() = 0;
protected:
  __forceinline NThreadLocalNonPODBase() 
  {
    CCriticalSectionHolder h(s_listLock()); 
    std::vector<NThreadLocalNonPODBase*>& l = s_tlsList();
    l.push_back(this);
  }
  __forceinline ~NThreadLocalNonPODBase() 
  { 
    CCriticalSectionHolder h(s_listLock()); 
    std::vector<NThreadLocalNonPODBase*>& l = s_tlsList();
    l.erase(std::remove(l.begin(), l.end(), this), l.end());
  }
public:
   //NWorkerThread's call this on exit, but you need to if you create a thread another way
  static void DeleteAllTLSDataInThisThread();
};

#if !defined(NV_NO_DECLSPEC_THREAD)

  //use the special compiler support (only works for POD types though)
  //we don't need to derive from NThreadLocalNonPODBase since there is no cleanup
  template <class T>
  class NThreadLocalVar
  {
    NV_DECLSPEC_THREAD T m_var;
  public:
    __forceinline NThreadLocalVar() : m_var() {};
    __forceinline T& operator=(const T& v)
    {
      m_var = v;
      return m_var;
    }
    __forceinline operator T&() { return m_var; }
    __forceinline operator const T&() const { return m_var; }
    __forceinline T* operator->() { return &m_var; }
    __forceinline const T* operator-> const () { return &m_var; }
    __forceinline T* operator&() { return &m_var; }
    __forceinline const T* operator& const () { return &m_var; }
  };

#else //!NV_NO_DECLSPEC_THREAD

  //use the slower but more flexible OS API version for everything
  template <class T>
  class NThreadLocalVar : private NThreadLocalNonPODBase
  {
#ifdef WIN32
    DWORD m_varIdx;
    __forceinline T* GetPtr()
    {
      T* v = (T*)TlsGetValue(m_varIdx);
      if (!v)
      {
        v = (T*)malloc(sizeof(T));
        memset(v, 0, sizeof(T));
        TlsSetValue(m_varIdx, v);
      }
      return v;
    }
    void DeleteMyTLSData() 
    {
      free(TlsGetValue(m_varIdx)); 
      TlsSetValue(m_varIdx, NULL);
    }
  public:
    __forceinline NThreadLocalVar() { m_varIdx = TlsAlloc();  };
    __forceinline ~NThreadLocalVar() { TlsFree(m_varIdx); }
#else //WIN32
    pthread_key_t m_varIdx;
    __forceinline T* GetPtr()
    {
      T* v = (T*)pthread_getspecific(m_varIdx);
      if (!v)
      {
        v = (T*)malloc(sizeof(T));
        memset(v, 0, sizeof(T));
        pthread_setspecific(m_varIdx, v);
      }
      return v;
    }
    void DeleteMyTLSData() 
    {
      free(pthread_getspecific(m_varIdx)); 
      pthread_setspecific(m_varIdx, NULL);
    }
  public:
    __forceinline NThreadLocalVar() { pthread_key_create(&m_varIdx, NULL);  }; //we don't use pthread destructors insce they aren't on Win32
    __forceinline ~NThreadLocalVar() { pthread_key_delete(m_varIdx); }
#endif //WIN32

    __forceinline T& operator=(const T& v)
    {
      T* var = GetPtr();
      *var = v;
      return *var;
    }
    __forceinline operator T&() { return *GetPtr(); }
    __forceinline operator const T&() const { return *GetPtr(); }
    __forceinline T* operator->() { return GetPtr(); }
    __forceinline const T* operator-> () const { return GetPtr(); }
    __forceinline T* operator&() { return GetPtr(); }
    __forceinline const T* operator& () const{ return GetPtr(); }
  };

  //for pointers don't double indirect
  template <class T>
  class NThreadLocalVar<T*> : private NThreadLocalNonPODBase
  {
    T* m_fakePtr;
#ifdef WIN32
    DWORD m_varIdx;
    __forceinline T* GetPtr()
    {
      T* v = (T*)TlsGetValue(m_varIdx);
      return v;
    }
    void DeleteMyTLSData() 
    {
      TlsSetValue(m_varIdx, NULL);
    }
  public:
    __forceinline NThreadLocalVar() { m_varIdx = TlsAlloc(); m_fakePtr = NULL;  };
    __forceinline ~NThreadLocalVar() { TlsFree(m_varIdx); }
    __forceinline T*& operator=(T* v)
    {
      TlsSetValue(m_varIdx, v);
      //need to return something so that we chain up assignments
      return m_fakePtr;
    }
#else //WIN32
    pthread_key_t m_varIdx;
    __forceinline T* GetPtr()
    {
      T* v = (T*)pthread_getspecific(m_varIdx);
      return v;
    }
    void DeleteMyTLSData() 
    {
      pthread_setspecific(m_varIdx, NULL);
    }
  public:
    __forceinline NThreadLocalVar() { pthread_key_create(&m_varIdx, NULL); m_fakePtr = NULL;  };
    __forceinline ~NThreadLocalVar() { pthread_key_delete(m_varIdx); }
    __forceinline T*& operator=(T* v)
    {
      pthread_setspecific(m_varIdx, v);
      //need to return something so that we chain up assignments
      return m_fakePtr;
    }
#endif //WIN32

    __forceinline operator T*() { return GetPtr(); }
    __forceinline operator const T*() const { return GetPtr(); }
    __forceinline T* operator->() { return GetPtr(); }
    __forceinline const T* operator-> () const{ return GetPtr(); }

    //you can't make double indirection here
    __forceinline T** operator&() { return NULL; }
    __forceinline const T** operator& () const { return NULL; }
  };



#endif // !defined(NV_NO_DECLSPEC_THREAD)

//nest these so we don't need to make multiple versions
template <typename T>
class NThreadLocalVarNonPOD : private NThreadLocalNonPODBase
{
  NThreadLocalVar<T*> m_ptr;
  void DeleteMyTLSData() 
  {
    delete (T*)m_ptr; 
    m_ptr = NULL;
  }
  __forceinline T* GetPtr()
  {
    T* v = m_ptr;
    if (!v)
    {
      v = new T();
      m_ptr = v;
    }
    return v;
  }
public:
  __forceinline NThreadLocalVarNonPOD() { };
  __forceinline ~NThreadLocalVarNonPOD() { }
  __forceinline T& operator=(const T& v)
  {
    T* var = GetPtr();
    *var = v;
    return *var;
  }
  __forceinline operator T&() { return *GetPtr(); }
  __forceinline operator const T&() const { return *GetPtr(); }

  __forceinline T* operator->() { return GetPtr(); }
  __forceinline const T* operator-> () const { return GetPtr(); }
  __forceinline T* operator&() { return GetPtr(); }
  __forceinline const T* operator& () const { return GetPtr(); }
};

//and for pointer types
template <typename T>
class NThreadLocalVarNonPOD<T*> : private NThreadLocalNonPODBase
{
  //This class is purposely undefined, you should use NThreadLocalVar instead since
  //While T may not be POD, T* is a POD type (a pointer itself has no constructors or destructors etc..)
public:
  NThreadLocalVarNonPOD();
  ~NThreadLocalVarNonPOD();
};


}