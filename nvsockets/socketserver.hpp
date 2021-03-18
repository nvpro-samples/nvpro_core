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


// (some details to check for TCP case here...)
// http://www.pcs.cnu.edu/~dgame/sockets/sockets.html

#ifndef __SOCKETLISTENER_H
#define __SOCKETLISTENER_H
#include "cthread_s.hpp"

class CBaseServer : virtual public sock::CThread
{
public:
    CBaseServer(bool asTCP=true);
    ~CBaseServer();
    bool Init(int port);
    void Close();
    bool tcpSendToCurrent(const char* pack, int size);
    bool tcpSendToCurrent(char c, float i);
    bool tcpSendToCurrent(char c, int i);
    bool tcpSendToAll(const char* pack, int size);
    bool tcpSendToAll(char c, int i);
    bool tcpSendToAll(char c, int i0, int i1, int i2=0, int i3=0);
    bool tcpSendToAll(char c, float i);
    bool tcpSendToAll(char c, float i0, float i1, float i2=0, float i3=0);
    bool tcpSendToAll(char c, int argtoken, int arg1, int arg2, int arg3, int arg4);

    int         m_numPendingClients;
    int         m_pendingClients[FD_SETSIZE];
protected:
    SOCKET      m_sockServer;
    int         m_numTcpClients;
    SOCKET      m_tcpClients[FD_SETSIZE];
    SOCKADDR_IN m_saCli;
    bool        m_initDone;
    bool        m_asTCP;

    bool InitPort(int port);
    void removeConnection(SOCKET client);
    // spillingData to be freed if ever used
    int  recieve(void *dataBuff, int len, unsigned char** spillingData);
    const char* getHostNameOfLastReceivedData() {
        struct hostent *hp = gethostbyaddr((char*)&m_saCli.sin_addr.S_un.S_un_b.s_b1,4,AF_INET);
        return hp->h_name;
    }

    virtual void CThreadProc();

    bool ExecuteProcess(LPSTR dir, LPSTR comm, LPSTR args, PROCESS_INFORMATION &pi);

};


#endif