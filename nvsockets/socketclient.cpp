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
#include <nvh/nvprint.hpp>
#include "socketclient.hpp"

struct SockMsgHeader {
    short   token;
    int sz;
};


bool CBaseClient::Init(const char* servername, int port)
{

    if(!InitPort(&m_sock, servername, port))
        return false;

    m_valid = true;

    return true;
}

bool CBaseClient::InitPort(SOCKET *sock, const char* hostname, int port)
{

    LOGI("Openning connection to server %s:%d\n", hostname, port);

    //
    // DoStartup () initializes the Winsock DLL with Winsock version 1.1
    //
    WSADATA wsaData;
  
    INT iRetVal;

    iRetVal = WSAStartup ( MAKEWORD ( 1,1 ), &wsaData );
    
    if ( 0 != iRetVal)
    {
        LOGE("WSAStartup failled (%d)", iRetVal );
        return false;
    }

    
    ///////////////////// create socket

    struct sockaddr_in server;
    struct hostent *hp;
    //unsigned int addr;


    if(m_asTCP)
        *sock = socket ( AF_INET, SOCK_STREAM, 0 );
    else
        *sock = socket ( AF_INET, SOCK_DGRAM, 0 );

    if ( INVALID_SOCKET ==  *sock)
    {
        LOGI("socket creation failled (%d)", WSAGetLastError() );
        WSACleanup();
        // TODO call WSACleanup()
        return false;
    }

    //
    // Attempt to detect if we should call gethostbyname() or
    // gethostbyaddr()

    if (isalpha(hostname[0])) {   /* server address is a name */
        hp = gethostbyname(hostname);
    }
    else  { /* Convert nnn.nnn address to a usable one */
        //addr = inet_addr(hostname);
        hp = gethostbyaddr(hostname,4,AF_INET);
    }
    if (hp == NULL ) {
        LOGI("Client: Cannot resolve address [%s]: Error %d BE SURE TO USE server in hosts\n",
            hostname,WSAGetLastError());
        WSACleanup();
        // TODO call WSACleanup()
        return false;
    }
    m_hostname = std::string(hp->h_name);

    //
    // Copy the resolved information into the sockaddr_in structure
    //
    memset(&server,0,sizeof(server));
    memcpy(&(server.sin_addr),hp->h_addr,hp->h_length);
    server.sin_family = hp->h_addrtype;
    server.sin_port = htons(port);


    //
    // Notice that nothing in this code is specific to whether we 
    // are using UDP or TCP.
    // We achieve this by using a simple trick.
    //    When connect() is called on a datagram socket, it does not 
    //    actually establish the connection as a stream (TCP) socket
    //    would. Instead, TCP/IP establishes the remote half of the
    //    ( LocalIPAddress, LocalPort, RemoteIP, RemotePort) mapping.
    //    This enables us to use send() and recv() on datagram sockets,
    //    instead of recvfrom() and sendto()
    if (connect(*sock,(struct sockaddr*)&server,sizeof(server))
        == SOCKET_ERROR) {
        LOGI("connect() failed: %d\n",WSAGetLastError());
        closesocket(*sock);
        *sock = 0;
        WSACleanup();
        return false;
    }

    return true;
}


bool CBaseClient::Send(const char* pack, unsigned int size)
{
    if(!m_valid)
        return false;

    int retval = send(m_sock, pack, size,0);
    if (retval == SOCKET_ERROR) 
    {
        LOGE("send() failed: error %d\n",WSAGetLastError());
        WSACleanup();
        return false;
    }

    return true;
}

bool CBaseClient::Send(const void* pack, unsigned int size)
{
    if(!m_valid)
        return false;

    int retval = send(m_sock, (const char*)pack, size,0);
    if (retval == SOCKET_ERROR) 
    {
        LOGE("send() failed: error %d\n",WSAGetLastError());
        WSACleanup();
        return false;
    }

    return true;
}

bool CBaseClient::Send(char c)
{
    if(!m_valid)
        return false;
    SockMsgHeader data;
    data.token = c;
    data.sz = sizeof(SockMsgHeader);
    int retval = send(m_sock, (char*)&data, sizeof(SockMsgHeader),0);
    if (retval == SOCKET_ERROR) 
    {
        LOGE("send() failed: error %d\n",WSAGetLastError());
        WSACleanup();
        return false;
    }
    return true;
}

bool CBaseClient::Send(char c, int i)
{
    if(!m_valid)
        return false;
    struct D : SockMsgHeader {
        int i;
    } data;
    data.token = c;
    data.sz = sizeof(D);
    data.i = i;
    int retval = send(m_sock, (char*)&data, sizeof(D),0);
    if (retval == SOCKET_ERROR) 
    {
        LOGE("send() failed: error %d\n",WSAGetLastError());
        WSACleanup();
        return false;
    }
    return true;
}
bool CBaseClient::Send(char c, int i0, int i1, int i2, int i3)
{
    struct D : SockMsgHeader {
        int i[4];
    } data;
    data.token = c;
    data.sz = sizeof(D);
    data.i[0] = i0;
    data.i[1] = i1;
    data.i[2] = i2;
    data.i[3] = i3;
    return Send((char*)&data, sizeof(D));
}
bool CBaseClient::Send(char c, int argtoken, int arg1, int arg2, int arg3, int arg4)
{
    struct D : SockMsgHeader {
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
    return Send((char*)&data, sizeof(D));
}

bool CBaseClient::Send(char c, float i)
{
    if(!m_valid)
        return false;
    struct D : SockMsgHeader {
        float i;
    } data;
    data.token = c;
    data.sz = sizeof(D);
    data.i = i;
    int retval = send(m_sock, (char*)&data, sizeof(D),0);
    if (retval == SOCKET_ERROR) 
    {
        LOGE("send() failed: error %d\n",WSAGetLastError());
        WSACleanup();
        return false;
    }
    return true;
}

bool CBaseClient::Send(char c, float i0, float i1, float i2, float i3)
{
    struct D : SockMsgHeader {
        float i[4];
    } data;
    data.token = c;
    data.sz = sizeof(D);
    data.i[0] = i0;
    data.i[1] = i1;
    data.i[2] = i2;
    data.i[3] = i3;
    return Send((char*)&data, sizeof(D));
}


bool CBaseClient::Recv(char *dstBuf, float szBuf)
{
    if(!m_valid)
        return false;
    if(recv(m_sock, dstBuf, (int)szBuf, 0) == 0)
    {
        perror("recv error");
        return false;
    }
    return true;
}

void CBaseClient::Shutdown(void)
{
    DeleteThread();
    if (m_sock)
    {
        closesocket(m_sock);
        shutdown(m_sock, 2);
        m_sock = 0;
    }
    m_valid = false;
}

int CBaseClient::recieve(void *dataBuff, int len, unsigned char** spillingData)
{
    int nSize = -1;
    if(m_asTCP && m_valid)
    {
        SockMsgHeader *head = (SockMsgHeader *)dataBuff;
        if((nSize = recv(m_sock, (char*)head, sizeof(SockMsgHeader), 0)) <= 0)
        {
            if(nSize == SOCKET_ERROR)
            { LOGE("socket header recv error. Closing it..."); }
            else
            { LOGI("gacefully closing socket..."); }
            closesocket(m_sock);
            shutdown(m_sock, 2);
            m_sock = 0;
            return -1;
        }
        // we need a protocol to know how much data to expect
        // token + size of data
        int l = ((head->sz > len) ? len:head->sz) - sizeof(SockMsgHeader);
        if(l == 0)
            return sizeof(SockMsgHeader);
        if(l < 0)
        {
            LOGE("socket recv error : didn't find the proper header...");
            return -1;
        }
        if((l = recv(m_sock, (char*)dataBuff + sizeof(SockMsgHeader), l, 0)) == -1)
        {
            closesocket(m_sock);
            shutdown(m_sock, 2);
            m_sock = 0;
            return -1;
        }
        nSize += l;
        if(head->sz > len)
        {
            unsigned char* spill = (unsigned char*)malloc(head->sz);
            memcpy(spill, dataBuff, len);
            nSize = len;
            do {
                l = recv(m_sock, (char*)(spill+nSize), head->sz-nSize, 0);
                if(l == -1)
                {
                    free(spill);
                    closesocket(m_sock);
                    shutdown(m_sock, 2);
                    m_sock = 0;
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
    } else
        return -1;
}

