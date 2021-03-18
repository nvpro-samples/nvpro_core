/*-----------------------------------------------------------------------
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ //--------------------------------------------------------------------

#ifndef __CLIENT__
#define __CLIENT__

#include <string>
#include "cthread_s.hpp"

/**
 ** Handles sock creation and message transportation
 **/
class CBaseClient: virtual public sock::CThread
{
public:
    CBaseClient(bool asTCP=true) : sock::CThread(/*startNow*/false, /*Critical*/false), m_valid(false), m_sock(0), m_asTCP(asTCP) {};
    virtual bool Init(const char* servername, int port);
    virtual void Shutdown(void);
    virtual bool Send(const char* pack, unsigned int size);
    virtual bool Send(const void* pack, unsigned int size);
    virtual bool Send(char c);
    virtual bool Send(char c, int i);
    virtual bool Send(char c, int i0, int i1, int i2=0, int i3=0);
    virtual bool Send(char c, float i);
    virtual bool Send(char c, float i0, float i1, float i2=0, float i3=0);
    virtual bool Send(char c, int argtoken, int arg1, int arg2, int arg3, int arg4);
    virtual bool Recv(char *dstBuf, float szBuf);
    virtual ~CBaseClient() {Shutdown();};

    bool isValid() {return m_valid; }

    std::string m_hostname;
protected:
    bool InitPort(SOCKET *sock, const char* hostname, int port);
    int  recieve(void *dataBuff, int len, unsigned char** spillingData);

    SOCKET m_sock;
    bool m_valid;
    bool m_asTCP;
};

#endif