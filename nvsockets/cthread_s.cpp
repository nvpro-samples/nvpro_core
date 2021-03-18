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
//#include "comms.h"
#if defined WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#ifndef ANDROID
#   include <sys/sysctl.h>
#endif
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include <assert.h>
#include <stdio.h>
#include "cthread_s.hpp"

namespace sock
{

//----------------------------------------------------------------------------------
// This Function is used as the main callback for all. Then the argument passed will
// be used to jump at the right derived class
//----------------------------------------------------------------------------------
void thread_function(void *pData)
{
    NXPROFILEFUNCCOL(__FUNCTION__, 0xFF800000);
    CThread* pthread = static_cast<CThread*>(pData);
    pthread->CThreadProc();
}

#if defined WIN32
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// WINDOWS WINDOWS WINDOWS WINDOWS WINDOWS WINDOWS WINDOWS WINDOWS WINDOWS WINDOWS //
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
// THREAD THREAD THREAD THREAD THREAD THREAD THREAD THREAD THREAD THREAD THREAD    //
/////////////////////////////////////////////////////////////////////////////////////
CThread::CThread(const bool startNow, const bool Critical)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    m_thread = CreateThread(thread_function, static_cast<CThread*>(this), startNow, Critical);
}

CThread::~CThread()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CancelThread();
    DeleteThread();
}

// CpuCount
int CThread::CpuCount() 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    static int Cpus = -1;
    if(-1 == Cpus) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        Cpus = (int)si.dwNumberOfProcessors > 1 ? (int)si.dwNumberOfProcessors : 1;
        //if(Cpus>4)Cpus=4;
    }
    return Cpus;
}
//int CThread::CpuCount0() 
//{
//    static int Cpus = -1;
//    if(-1 == Cpus) {
//        SYSTEM_INFO si;
//        GetSystemInfo(&si);
//        Cpus = (int)si.dwNumberOfProcessors > 1 ? (int)si.dwNumberOfProcessors : 1;
//    }
//    return Cpus;
//}

// Sleep
void CThread::Sleep(const unsigned long Milliseconds) 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    ::Sleep(Milliseconds);
}

// CreateThread
NThreadHandle CThread::CreateThread(ThreadProc Proc, void *Param, bool startNow, const bool Critical) 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    NThreadHandle hThread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Proc, Param, CREATE_SUSPENDED, NULL);
    if(Critical) {
        SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
    }
    if(startNow)
        ::ResumeThread(hThread);
    return hThread;
}

// CancelThread
void CThread::CancelThread() 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    TerminateThread(m_thread, 0);
}

// DeleteThread
void CThread::DeleteThread() 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CloseHandle(m_thread);
    m_thread = NULL;
}

// WaitThread
void CThread::WaitThread() 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    WaitForSingleObject(m_thread, INFINITE);
}

// WaitThreads
void CThread::WaitThreads(const NThreadHandle *Threads, const int Count) 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    WaitForMultipleObjects(Count, Threads, TRUE, INFINITE);
}

// SuspendThread
bool CThread::SuspendThread()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    return ::SuspendThread(m_thread) == -1 ? false : true;
}

// ResumeThread
bool CThread::ResumeThread()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    return ::ResumeThread(m_thread) == -1 ? false : true;
}

//SetThreadAffinity
void CThread::SetThreadAffinity(unsigned int mask)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    SetThreadAffinityMask(m_thread, mask);
}
/////////////////////////////////////////////////////////////////////////////////////
// MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX   //
/////////////////////////////////////////////////////////////////////////////////////
CMutex::CMutex()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CMutex::CreateMutex(m_mutex);
}

CMutex::~CMutex()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CMutex::DeleteMutex();
}

// CreateMutex
void CMutex::CreateMutex(NMutexHandle &Mutex) 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    Mutex = ::CreateMutex(NULL, FALSE, NULL);
}

// DeleteMutex
void CMutex::DeleteMutex() 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CloseHandle(m_mutex);
    m_mutex = NULL;
}

// LockMutex
bool CMutex::LockMutex(int ms, long *dbg) 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    DWORD res = WaitForSingleObjectEx(m_mutex, ms == -1 ? INFINITE : ms, FALSE);
    if(dbg) *dbg = res;
    return res == WAIT_OBJECT_0 ? true : false;
}

// UnlockMutex
void CMutex::UnlockMutex() 
{    
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    BOOL bRes = ReleaseMutex(m_mutex);
    assert(bRes);
}

/////////////////////////////////////////////////////////////////////////////////////
// SEMAPHORE SEMAPHORE SEMAPHORE SEMAPHORE SEMAPHORE SEMAPHORE SEMAPHORE SEMAPHORE //
/////////////////////////////////////////////////////////////////////////////////////
CSemaphore::CSemaphore()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CSemaphore::CreateSemaphore(m_semaphore, 0, 0xFFFF);
}

CSemaphore::CSemaphore(long initialCnt, long maxCnt)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CSemaphore::CreateSemaphore(m_semaphore, initialCnt, maxCnt);
}

CSemaphore::~CSemaphore()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CSemaphore::DeleteSemaphore();
}

int    CSemaphore::num_Semaphores = 0;
// CreateSemaphore
void CSemaphore::CreateSemaphore(NSemaphoreHandle &Semaphore, long initialCnt, long maxCnt) 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    num_Semaphores++;
    Semaphore = ::CreateSemaphoreA(NULL, initialCnt, maxCnt, NULL);
}

// DeleteSemaphore
void CSemaphore::DeleteSemaphore() 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CloseHandle(m_semaphore);
    m_semaphore = NULL;
    num_Semaphores--;
}

// AcquireSemaphore
bool CSemaphore::AcquireSemaphore(int ms) 
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    return WaitForSingleObject(m_semaphore, ms == -1 ? INFINITE : ms) == WAIT_TIMEOUT ? false : true;
}

// ReleaseSemaphore
void CSemaphore::ReleaseSemaphore(long cnt) 
{    
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    BOOL bRes = ::ReleaseSemaphore(m_semaphore, cnt, NULL);
}

/////////////////////////////////////////////////////////////////////////////////////
// EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT   //
/////////////////////////////////////////////////////////////////////////////////////
int    CEvent::num_events = 0;
CEvent::CEvent(bool manualReset, bool initialState)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CEvent::CreateEvent(m_event, manualReset, initialState);
}
CEvent::~CEvent()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CEvent::DeleteEvent();
}

void CEvent::CreateEvent(NEventHandle &Event, bool manualReset, bool initialState)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    num_events++;
    Event = ::CreateEventA(NULL, manualReset, initialState, NULL);
}
void CEvent::DeleteEvent()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    CloseHandle(m_event);
    m_event = NULL;
    num_events--;
}
bool CEvent::Set()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    return SetEvent(m_event) ? true : false;
}
bool CEvent::Pulse()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    return PulseEvent(m_event) ? true : false;
}
bool CEvent::Reset()
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    return ResetEvent(m_event) ? true : false;
}
bool CEvent::WaitOnEvent(int ms)
{
    NXPROFILEFUNCCOL(__FUNCTION__, COLOR_RED);
    return WaitForSingleObject(m_event, ms == -1 ? INFINITE : ms) == WAIT_TIMEOUT ? false : true;
}


#endif // WINDOWS

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
// UNIX UNIX UNIX UNIX UNIX UNIX UNIX UNIX UNIX UNIX UNIX UNIX UNIX UNIX UNIX UNIX //
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

#if defined IOS || defined ANDROID

/////////////////////////////////////////////////////////////////////////////////////
// CThread CThread CThread CThread CThread CThread CThread CThread CThread CThread //
/////////////////////////////////////////////////////////////////////////////////////
CThread::CThread(const bool startNow, const bool Critical)
{
    m_thread = CreateThread(thread_function, static_cast<CThread*>(this), startNow, Critical);
}

CThread::~CThread()
{
    CancelThread();
    DeleteThread();
}


// CpuCount
int CThread::CpuCount() {
    static int Cpus = -1;
    if(-1 == Cpus) {
#ifdef IOS
        size_t s = sizeof(Cpus);
        sysctlbyname("hw.logicalcpu", &Cpus, &s, NULL, 0);
#endif // IOS
#ifdef ANDROID
        Cpus = sysconf(_SC_NPROCESSORS_ONLN);
#endif // ANDROID
        Cpus = Cpus > 1 ? Cpus : 1;
    }
    return Cpus;
}

// Sleep
void CThread::Sleep(const unsigned long Milliseconds) {
    usleep(1000 * (useconds_t)Milliseconds);
}

// CreateThread
NThreadHandle CThread::CreateThread(ThreadProc Proc, void *Param, bool startNow, const bool Critical) 
{
    pthread_t th;
    //bool startNow, ?
    pthread_create(&th, NULL, (void *(*)(void *))Proc, Param);
    return th;
}

// CancelThread
void CThread::CancelThread() {
#if defined ANDROID
    pthread_kill(m_thread, SIGUSR1);
#else
    pthread_cancel(m_thread);
#endif
}

// DeleteThread
void CThread::DeleteThread() {
    pthread_detach(m_thread);
}

// WaitThread
void CThread::WaitThread() {
    pthread_join(m_thread, NULL);
}

// WaitThreads
void CThread::WaitThreads(const NThreadHandle *Threads, const int Count) {
    int i;
    for(i = 0; i < Count; i++) {
        pthread_join(Threads[i], NULL);
    }
}

///////////////////////////////////////////////////////////////////////////////////
// MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX MUTEX //
///////////////////////////////////////////////////////////////////////////////////
CMutex::CMutex()
{
    CMutex::CreateMutex(m_mutex);
}

CMutex::~CMutex()
{
    CMutex::DeleteMutex();
}
// CreateMutex
void CMutex::CreateMutex(NMutexHandle &Mutex) 
{
    pthread_mutex_init(&Mutex, NULL);
}

// NMutexHandle
void CMutex::DeleteMutex() 
{
    pthread_mutex_destroy(&m_mutex);
}

// LockMutex
bool CMutex::LockMutex(int ms, long *dbg) {
    pthread_mutex_lock(&m_mutex);
    return true;
}

// UnlockMutex
void CMutex::UnlockMutex() {
    pthread_mutex_unlock(&m_mutex);
}

/////////////////////////////////////////////////////////////////////////////////////
// SEMAPHORE SEMAPHORE SEMAPHORE SEMAPHORE SEMAPHORE SEMAPHORE SEMAPHORE SEMAPHORE //
/////////////////////////////////////////////////////////////////////////////////////
CSemaphore::CSemaphore(long initialCnt, long maxCnt)
{
    CSemaphore::CreateSemaphore(m_semaphore, initialCnt, maxCnt);
}

CSemaphore::~CSemaphore()
{
    CSemaphore::DeleteSemaphore();
}

int    CSemaphore::num_Semaphores = 0;
// CreateSemaphore
void CSemaphore::CreateSemaphore(NSemaphoreHandle &Semaphore, long initialCnt, long maxCnt) 
{
    num_Semaphores++;
    sem_init(&Semaphore, 0, (unsigned int)initialCnt);
}

// DeleteSemaphore
void CSemaphore::DeleteSemaphore() 
{
    sem_destroy(&m_semaphore);
    //!@$!#$@#$m_semaphore = NULL;
    num_Semaphores--;
}

// AcquireSemaphore
bool CSemaphore::AcquireSemaphore(int msTimeOut) 
{
    //if(msTimeOut == 0)
        sem_wait(&m_semaphore);
    /*else {
        //convert timeout to a timespec, pthreads wants the final time not the length
#if 0
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += time_t(msTimeOut) / time_t(1000);
        ts.tv_nsec += (long(msTimeOut) % long(1000)) * long(1000*1000);
#else
        struct timeval tv;
        struct timespec ts;
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + time_t(msTimeOut) / time_t(1000);
        ts.tv_nsec = tv.tv_usec*1000 + (long(msTimeOut) % long(1000)) * long(1000*1000);
#endif
        
        if (sem_timedwait(&m_semaphore, &ts))// WTF ?!?!? == ETIMEDOUT)
        {
            //timed out
            return false;
        }
        return true;
    }*/
    return true;
}

//void NCountSemaphore::Post() const
// ReleaseSemaphore
void CSemaphore::ReleaseSemaphore(long cnt) 
{    
    for(;cnt > 0; cnt--)
        sem_post(&m_semaphore);
    
}


/////////////////////////////////////////////////////////////////////////////////////
// EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT EVENT   //
/////////////////////////////////////////////////////////////////////////////////////
int    CEvent::num_events = 0;
CEvent::CEvent(bool manualReset, bool initialState)
{
    CEvent::CreateEvent(m_event, manualReset, initialState);
    m_signaled = initialState;
    m_manualReset = manualReset;
    // TODO: put it in CreateEvent
    pthread_mutex_init(&m_mutex, NULL); // do we need non-default attrs (second arg)?
}
CEvent::~CEvent()
{
    CEvent::DeleteEvent();
}

void CEvent::CreateEvent(NEventHandle &event, bool manualReset, bool initialState)
{
    num_events++;
    pthread_cond_init(&event, NULL);
}
void CEvent::DeleteEvent()
{
    //m_event = NULL;
    num_events--;
    pthread_cond_destroy(&m_event);
    pthread_mutex_destroy(&m_mutex);
}
bool CEvent::Set()
{
    int r = 0;
    pthread_mutex_lock(&m_mutex);
    
    if (m_signaled == false)
    {
        m_signaled = true;
        r = pthread_cond_broadcast(&m_event);
    }
    
    pthread_mutex_unlock(&m_mutex);
    return r ? false : true;
}
bool CEvent::Pulse()
{
    pthread_mutex_lock(&m_mutex);
    
    int r = pthread_cond_broadcast(&m_event);
    
    pthread_mutex_unlock(&m_mutex);
    return r ? false : true;
}
bool CEvent::Reset()
{
    pthread_mutex_lock(&m_mutex);
    m_signaled = false;
    pthread_mutex_unlock(&m_mutex);
    return m_signaled;
}
bool CEvent::WaitOnEvent(int msTimeOut)
{
    pthread_mutex_lock(&m_mutex);
    
    //convert timeout to a timespec, pthreads wants the final time not the length
    struct timeval tv;
    struct timespec ts;
    if(msTimeOut >= 0)
    {
        gettimeofday(&tv, NULL);
        //timespec ts;
        //clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec = tv.tv_sec + time_t(msTimeOut) / time_t(1000);
        ts.tv_nsec = tv.tv_usec*1000 + (long(msTimeOut) % long(1000)) * long(1000*1000);
    }    
    bool ret = true;
    while(m_signaled == false)
    {
        if(msTimeOut >= 0)
        {
            if (::pthread_cond_timedwait(&m_event, &m_mutex, &ts)) // WTF?!?! == ETIMEDOUT)
            {
                //timed out
                ret = false;
                break;
            }
        } else {
            if (::pthread_cond_wait(&m_event, &m_mutex))
            {
                //must be an error, then
                ret = false;
                break;
            }
        }
    }
    
    if (ret && !m_manualReset)
    {
        m_signaled = false;
    }
    
    pthread_mutex_unlock(&m_mutex);
    return ret;
}
#endif // IOS || ANDROID

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
// NThreadLocalNonPODBase NThreadLocalNonPODBase NThreadLocalNonPODBase NThreadLocalNonPODBase
// NThreadLocalNonPODBase NThreadLocalNonPODBase NThreadLocalNonPODBase NThreadLocalNonPODBase
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////


CCriticalSection& NThreadLocalNonPODBase::s_listLock()
{
  static CCriticalSection cs;
  return cs;
}

std::vector<NThreadLocalNonPODBase*>& NThreadLocalNonPODBase::s_tlsList()
{
  static std::vector<NThreadLocalNonPODBase*> v;
  return v;
}

void NThreadLocalNonPODBase::DeleteAllTLSDataInThisThread()
{
  CCriticalSectionHolder h(s_listLock()); 
  std::vector<NThreadLocalNonPODBase*>& l = s_tlsList();
  for (size_t i = 0; i < l.size(); i++)
  {
    l[i]->DeleteMyTLSData();
  }
}


}