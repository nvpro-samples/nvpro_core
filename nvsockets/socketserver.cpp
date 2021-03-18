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

#ifdef WIN32
#include <Windows.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <nvh/nvprint.hpp>
#include "socketserver.hpp"

struct Header {
    short   token;
    int   sz;
};

CBaseServer::CBaseServer(bool asTCP) : CThread(/*startNow*/false, /*Critical*/false)
{
    m_asTCP = asTCP;
    m_initDone=false;
    m_numPendingClients = 0;
    memset(m_pendingClients, 0, sizeof(SOCKET)*FD_SETSIZE);
    m_numTcpClients = 0;
    memset(m_tcpClients, 0, sizeof(SOCKET)*FD_SETSIZE);
}
CBaseServer::~CBaseServer()
{
    Close();
}

void CBaseServer::CThreadProc()
{
    char buffer[100];
    int l;

    while (1)
    {
            //// acknowledge the message, reply w/ the file names
            //if (send(m_current, "OK", 2, 0) == -1) {
            //    LOGE("send error");
            //    break;
            //}
        if ((l=recieve(buffer, 100, NULL)) == 0)
        {
            LOGI("Bad error in listenForMessagesThread");
        }
        else
        {
            switch(buffer[0])
            {
            case 1:
                LOGI("Received: %d", buffer+1);
                break;
            default:
                LOGI("Token %d...", buffer[0]);
                break;
            }
        }
        Sleep(1);
    }
}

bool CBaseServer::Init(int port)
{
    if (!InitPort(port))
        return false;
    if (!ResumeThread())
        return false;
    return true;
}


bool CBaseServer::InitPort(int port)
{
    //
    // DoStartup () initializes the Winsock DLL with Winsock version 1.1
    //
    WSADATA wsaData;
  
    INT iRetVal;

    iRetVal = WSAStartup ( MAKEWORD ( 1,1 ), &wsaData );
    
    if ( 0 != iRetVal)
    {
        LOGI("WSAStartup failled (%d)", iRetVal );
        return false;
    }
    
    ///////////////////// create socket
    if(m_asTCP)
        m_sockServer = socket ( AF_INET, SOCK_STREAM, 0 );
    else
        m_sockServer = socket ( AF_INET, SOCK_DGRAM, 0 );

    if ( INVALID_SOCKET ==  m_sockServer)
    {
        LOGI("socket creation failled (%d)", WSAGetLastError() );
        return false;
    }

    // IP address structures needed to bind to a local port and get the sender's
    // information.
    SOCKADDR_IN saServ;
  
    //
    // bind to the specified port.
    //
    saServ.sin_family = AF_INET;
    saServ.sin_addr.s_addr = htonl ( INADDR_ANY );
    saServ.sin_port = htons ( port );

    INT err = bind ( m_sockServer, (SOCKADDR FAR *)&saServ, sizeof ( SOCKADDR_IN ) );
    if ( SOCKET_ERROR == err )
    {
        LOGI("bind error (%d)\n", WSAGetLastError ( ) );
        // 
        return false;
    }
    //
    // show that we are willing to listen
    //
    if(m_asTCP)
    {
        if (listen(m_sockServer, 5) == -1)
        {
            LOGE("listen error");
            return false;
        }
    }
    m_initDone = true;
    return true;

}

void CBaseServer::removeConnection(SOCKET client)
{
    closesocket(client);
    for(int i=0; i<m_numTcpClients; i++)
    {
        if(m_tcpClients[i] == client)
        {
            LOGI("closing a socket %d...", client);
            for(int j=i+1; j<m_numTcpClients; i++, j++)
                m_tcpClients[i] = m_tcpClients[j];
            m_numTcpClients--;
            break;
        }
    }
}

int CBaseServer::recieve(void *dataBuff, int len, unsigned char** spillingData)
{
    
    if (!m_initDone)
    {
        LOGI("Socket listener not initialized properly, can't recieve\n");
        return 0;
    }
    int nSize = sizeof ( SOCKADDR_IN );

    if(m_asTCP)
    {
        if(m_numPendingClients <= 0)
        {
            // listen for new stuff
            fd_set readfds, exceptfds;
            readfds.fd_count = 1 + m_numTcpClients;
            readfds.fd_array[0] = m_sockServer;
            if(m_numTcpClients>0)
                memcpy(readfds.fd_array+1, m_tcpClients, sizeof(SOCKET)*m_numTcpClients);
            exceptfds.fd_count = 1 + m_numTcpClients;
            exceptfds.fd_array[0] = m_sockServer;
            if(m_numTcpClients>0)
                memcpy(exceptfds.fd_array+1, m_tcpClients, sizeof(SOCKET)*m_numTcpClients);
            int l;
            if((select(0, &readfds, NULL, &exceptfds, NULL)) == 0)
            {
                return 0;
            }
            l = readfds.fd_count;
            int i=0;
            if(readfds.fd_array[i] == m_sockServer)
            {
                SOCKET s;
                i++;
                if ((s = accept(m_sockServer, (struct sockaddr *)  &m_saCli, &nSize)) == -1)
                {
                    LOGE("accept error");
                    return -1;
                }
                LOGI("accepting %s#",inet_ntoa(m_saCli.sin_addr));
                LOGI("from port %d",ntohs(m_saCli.sin_port));
                //register this new client
                m_tcpClients[m_numTcpClients++] = s;
                nSize = 0;
            }
            if(i<l) // we have more to process
            {
                m_numPendingClients = l-i;
                memcpy(m_pendingClients, m_tcpClients+i, sizeof(SOCKET)*m_numPendingClients);
            }
        }
        // check if we still have to process the previous batch
        // this is done to transform CBaseServer::recieve behavior
        if(m_numPendingClients > 0)
        {
            m_numPendingClients--;
            Header *head = (Header *)dataBuff;
            if((nSize = recv(m_pendingClients[m_numPendingClients], (char*)head, sizeof(Header), 0)) <= 0)
            {
                if(nSize == SOCKET_ERROR)
                { LOGE("socket header recv error. Closing it..."); }
                else
                { LOGI("gacefully closing socket..."); }
                removeConnection(m_pendingClients[m_numPendingClients]);
                m_numPendingClients--;
                return -1;
            }
            // we need a protocol to know how much data to expect
            // token + size of data
            int l = ((head->sz > len) ? len:head->sz) - sizeof(Header);
            if(l == 0)
                return sizeof(Header);
            if(l < 0)
            {
                LOGE("socket recv error : didn't find the proper header...");
                return -1;
            }
            if((nSize = recv(m_pendingClients[m_numPendingClients], (char*)dataBuff + sizeof(Header), l, 0)) == -1)
            {
                removeConnection(m_pendingClients[m_numPendingClients]);
                m_numPendingClients--;
                return -1;
            }
            if(head->sz > len)
            {
                unsigned char* spill = (unsigned char*)malloc(head->sz);
                memcpy(spill, dataBuff, len);
                nSize = len;
                do {
                    l = recv(m_pendingClients[m_numPendingClients], (char*)(spill+nSize), head->sz-nSize, 0);
                    if(l == -1)
                    {
                        free(spill);
                        removeConnection(m_pendingClients[m_numPendingClients]);
                        m_numPendingClients--;
                        return -1;
                    }
                    nSize += l;
                } while(nSize < head->sz);
                if(spillingData)
                    *spillingData = spill;
                else
                    free(spill);
            }
            return nSize;
        }

    } else {
        //
        // receive a datagram on the bound port number.
        //
        int err = recvfrom ( m_sockServer,
                 (char*)dataBuff,
                 len,
                 0,
                 (SOCKADDR FAR *) &m_saCli,    // will be set when recieving message and used for reply back
                 &nSize
                 );
        if ( SOCKET_ERROR == err )
        {    
            if ( WSAEMSGSIZE == WSAGetLastError())
            { LOGI("The buffer is too small to hold the recieved message\n."\
            "Be sure to allocate on the slaves enough memory to hold the master buffer\n"); }
            else if (WSAECONNRESET  == WSAGetLastError())
            { LOGI("The Master has closed Socket-listener connection (stopped or crashed ?)\n"); }
            else
            { LOGI("Error in recieve, recvfrom error (%d)\n", WSAGetLastError ( ) ); }

            return -1;
        }
    }
    //ASSERT(strlen(inet_ntoa ( m_saCli.sin_addr )) < MAX_HOSTNAME);
    //sprintf(m_lastClientName,"%s", inet_ntoa ( m_saCli.sin_addr ));
    

    //
    // print the sender's information.
    //
    //LOGI("A  Datagram of length %d bytes received from ", nSize );
    //LOGI("\n\tIP Adress->%s ", inet_ntoa ( m_saCli.sin_addr ) );
    //LOGI("\n\tPort Number->%d\n", ntohs ( m_saCli.sin_port ) );

    
    return nSize;
}

bool CBaseServer::tcpSendToCurrent(const char* pack, int size)
{
    if(m_numPendingClients < 0)
        return false;
    if(!m_initDone)
        return false;

    int retval = send(m_pendingClients[m_numPendingClients], pack, size,0);
    if (retval == SOCKET_ERROR) 
    {
        LOGE("send() failed: error %d\n",WSAGetLastError());
        removeConnection(m_pendingClients[m_numPendingClients]);
        m_numPendingClients--;
        return false;
    }

    return true;
}
bool CBaseServer::tcpSendToCurrent(char c, int i)
{
    struct D : Header {
        int i;
    } data;
    data.sz = sizeof(D);
    data.token = c;
    data.i = i;
    return tcpSendToCurrent((char*)&data, sizeof(D));
}
bool CBaseServer::tcpSendToCurrent(char c, float i)
{
    struct D : Header {
        float i;
    } data;
    data.sz = sizeof(D);
    data.token = c;
    data.i = i;
    return tcpSendToCurrent((char*)&data, sizeof(D));
}

bool CBaseServer::tcpSendToAll(const char* pack, int size)
{
    for(int i=0; i<m_numTcpClients; i++)
    {
        int retval = send(m_tcpClients[i], pack, size,0);
        if (retval == SOCKET_ERROR) 
        {
            LOGE("send() failed: error %d\n",WSAGetLastError());
            removeConnection(m_tcpClients[i]);
            //return false;
        }
    }
    return true;
}
bool CBaseServer::tcpSendToAll(char c, int i)
{
    struct D : Header {
        int i;
    } data;
    data.token = c;
    data.sz = sizeof(D);
    data.i = i;
    return tcpSendToAll((char*)&data, sizeof(D));
}
bool CBaseServer::tcpSendToAll(char c, int i0, int i1, int i2, int i3)
{
    struct D : Header {
        int i[4];
    } data;
    data.token = c;
    data.sz = sizeof(D);
    data.i[0] = i0;
    data.i[1] = i1;
    data.i[2] = i2;
    data.i[3] = i3;
    return tcpSendToAll((char*)&data, sizeof(D));
}
bool CBaseServer::tcpSendToAll(char c, int argtoken, int arg1, int arg2, int arg3, int arg4)
{
    struct D : Header {
        int argtoken;
        int args[4];
    } data;
    data.token = c;
    data.sz = sizeof(D);
    data.argtoken = argtoken;
    data.args[0] = arg1;
    data.args[1] = arg2;
    data.args[2] = arg3;
    data.args[3] = arg4;
    return tcpSendToAll((char*)&data, sizeof(D));
}
bool CBaseServer::tcpSendToAll(char c, float i)
{
    struct D : Header {
        float i;
    } data;
    data.token = c;
    data.sz = sizeof(D);
    data.i = i;
    return tcpSendToAll((char*)&data, sizeof(D));
}
bool CBaseServer::tcpSendToAll(char c, float i0, float i1, float i2, float i3)
{
    struct D : Header {
        float i[4];
    } data;
    data.token = c;
    data.sz = sizeof(D);
    data.i[0] = i0;
    data.i[1] = i1;
    data.i[2] = i2;
    data.i[3] = i3;
    return tcpSendToAll((char*)&data, sizeof(D));
}

void CBaseServer::Close()
{
    DeleteThread();
    WSACleanup();
}


