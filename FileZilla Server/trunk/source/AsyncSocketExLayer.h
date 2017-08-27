/*CAsyncSocketEx by Tim Kosse (tim.kosse@filezilla-project.org)
            Version 1.1 (2002-11-01)
--------------------------------------------------------

Introduction:
-------------

CAsyncSocketEx is a replacement for the MFC class CAsyncSocket.
This class was written because CAsyncSocket is not the fastest WinSock
wrapper and it's very hard to add new functionality to CAsyncSocket
derived classes. This class offers the same functionality as CAsyncSocket.
Also, CAsyncSocketEx offers some enhancements which were not possible with
CAsyncSocket without some tricks.

How do I use it?
----------------
Basically exactly like CAsyncSocket.
To use CAsyncSocketEx, just replace all occurrences of CAsyncSocket in your
code with CAsyncSocketEx, if you did not enhance CAsyncSocket yourself in
any way, you won't have to change anything else in your code.

Why is CAsyncSocketEx faster?
-----------------------------

CAsyncSocketEx is slightly faster when dispatching notification event messages.
First have a look at the way CAsyncSocket works. For each thread that uses
CAsyncSocket, a window is created. CAsyncSocket calls WSAAsyncSelect with
the handle of that window. Until here, CAsyncSocketEx works the same way.
But CAsyncSocket uses only one window message (WM_SOCKET_NOTIFY) for all
sockets within one thread. When the window recieve WM_SOCKET_NOTIFY, wParam
contains the socket handle and the window looks up an CAsyncSocket instance
using a map. CAsyncSocketEx works differently. It's helper window uses a
wide range of different window messages (WM_USER through 0xBFFF) and passes
a different message to WSAAsyncSelect for each socket. When a message in
the specified range is received, CAsyncSocketEx looks up the pointer to a
CAsyncSocketEx instance in an Array using the index of message - WM_USER.
As you can see, CAsyncSocketEx uses the helper window in a more efficient
way, as it don't have to use the slow maps to lookup it's own instance.
Still, speed increase is not very much, but it may be noticeable when using
a lot of sockets at the same time.
Please note that the changes do not affect the raw data throughput rate,
CAsyncSocketEx only dispatches the notification messages faster.

What else does CAsyncSocketEx offer?
------------------------------------

CAsyncSocketEx offers a flexible layer system. One example is the proxy layer.
Just create an instance of the proxy layer, configure it and add it to the layer
chain of your CAsyncSocketEx instance. After that, you can connect through
proxies.
Benefit: You don't have to change much to use the layer system.
Another layer that is currently in development is the SSL layer to establish
SSL encrypted connections.

License
-------

Feel free to use this class, as long as you don't claim that you wrote it
and this copyright notice stays intact in the source files.
If you use this class in commercial applications, please send a short message
to tim.kosse@filezilla-project.org

*/#if !defined(AFX_ASYNCSOCKETEXLAYER_H__90C7FDB6_F3F1_4CC0_B77B_858458A563F3__INCLUDED_)
#define AFX_ASYNCSOCKETEXLAYER_H__90C7FDB6_F3F1_4CC0_B77B_858458A563F3__INCLUDED_

#include "AsyncSocketEx.h"	// Hinzugefügt von der Klassenansicht
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CAsyncSocketEx;
class CAsyncSocketExLayer
{
	friend CAsyncSocketEx;
	friend CAsyncSocketExHelperWindow;
protected:
	//Protected constructor so that CAsyncSocketExLayer can't be instantiated
	CAsyncSocketExLayer() {};
	virtual ~CAsyncSocketExLayer() = default;

	//Notification event handlers
	virtual void OnAccept(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);

	//Operations
	virtual BOOL Accept( CAsyncSocketEx& rConnectedSocket, SOCKADDR* lpSockAddr = NULL, int* lpSockAddrLen = NULL );
	virtual void Close();
	virtual BOOL Connect(std::wstring const& hostAddress, UINT nHostPort);
	virtual BOOL Connect(const SOCKADDR* lpSockAddr, int nSockAddrLen);
	virtual bool Create(UINT nSocketPort = 0, int nSocketType = SOCK_STREAM,
				long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT |
							FD_CONNECT | FD_CLOSE,
				std::wstring const& socketAddress = std::wstring(), int nFamily = AF_INET,
				bool reusable = false);
	virtual BOOL Listen( int nConnectionBacklog);
	virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	virtual BOOL ShutDown();

	//Functions that will call next layer
	BOOL ShutDownNext();
	BOOL AcceptNext( CAsyncSocketEx& rConnectedSocket, SOCKADDR* lpSockAddr = NULL, int* lpSockAddrLen = NULL );
	void CloseNext();
	BOOL ConnectNext(std::wstring const& hostAddress, UINT nHostPort);
	BOOL ConnectNext(const SOCKADDR* lpSockAddr, int nSockAddrLen);
	bool CreateNext(UINT nSocketPort, int nSocketType, long lEvent, std::wstring const& socketAddress, int nFamily = AF_INET, bool reusable = false);
	BOOL ListenNext(int nConnectionBacklog);
	int ReceiveNext(void *lpBuf, int nBufLen, int nFlags = 0);
	int SendNext(const void *lpBuf, int nBufLen, int nFlags = 0);

	CAsyncSocketEx *m_pOwnerSocket{};

	//Calls OnLayerCallback on owner socket
	int DoLayerCallback(int nType, int nParam1, int nParam2, char* str = 0);

	int GetLayerState();
	BOOL TriggerEvent(long lEvent, int nErrorCode, BOOL bPassThrough = FALSE );

	//Gets the socket family
	int GetFamily() const;

	//Sets the socket family
	bool SetFamily(int nFamily);

	// Iterate through protocols
	bool TryNextProtocol();

private:
	//Layer state can't be set directly from derived classes
	void SetLayerState(int nLayerState);
	int m_nLayerState{notsock};

	int m_nFamily{AF_UNSPEC};
	int m_lEvent{};
	std::wstring m_socketAddress;
	UINT m_nSocketPort{};

	addrinfo *m_addrInfo{}, *m_nextAddr{};

	//Called by helper window, dispatches event notification and updated layer state
	void CallEvent(int nEvent, int nErrorCode);

	int m_nPendingEvents{};
	int m_nCriticalError{};

	void Init(CAsyncSocketExLayer *pPrevLayer, CAsyncSocketEx *pOwnerSocket);
	CAsyncSocketExLayer *AddLayer(CAsyncSocketExLayer *pLayer, CAsyncSocketEx *pOwnerSocket);

	CAsyncSocketExLayer *m_pNextLayer{};
	CAsyncSocketExLayer *m_pPrevLayer{};

	struct t_LayerNotifyMsg {
		SOCKET hSocket{INVALID_SOCKET};
		CAsyncSocketExLayer *pLayer{};
		long lEvent{};
	};
};

#endif // !defined(AFX_ASYNCSOCKETEXLAYER_H__90C7FDB6_F3F1_4CC0_B77B_858458A563F3__INCLUDED_)
