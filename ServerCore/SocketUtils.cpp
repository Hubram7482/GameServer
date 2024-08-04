#include "pch.h"
#include "SocketUtils.h"

/*---------------------
	   SocketUtils
-----------------------*/

/* Connect, DisConnect, Accept 함수의 포인터를 의미하며 런타임에
실시간으로 함수들의 주소를 저장해 줄 것이다(설명이 모호함) */
LPFN_CONNECTEX	  SocketUtils::ConnectEx = { nullptr };
LPFN_DISCONNECTEX SocketUtils::DisConnectEx = { nullptr };
LPFN_ACCEPTEX	  SocketUtils::AcceptEx = { nullptr };

void SocketUtils::Init()
{
	WSADATA wsaData;
	// WSAStartup 함수 호출을 성공하면 0을 반환
	ASSERT_CRASH(WSAStartup(MAKEWORD(2, 2), OUT &wsaData) == 0);

	SOCKET DummySocket = CreateSocket();
	/* 만들어준 BindWindowsFunction 함수의 첫 번째 인자값으로
	더미 소켓을 넘겨주고 두 번째 인자값으로는 찾아오고 싶은
	함수를 넣어준다. 마지막으로 세 번째 인자값으로는 함수
	포인터를 넘겨준다 */
	ASSERT_CRASH(BindWindowsFunction(DummySocket, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&ConnectEx)));
	ASSERT_CRASH(BindWindowsFunction(DummySocket, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&DisConnectEx)));
	ASSERT_CRASH(BindWindowsFunction(DummySocket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx)));
	
	Close(DummySocket);
}

void SocketUtils::Clear()
{
	WSACleanup();
}

bool SocketUtils::BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn)
{
	/* ConnectEx, AcceptEx 등과 같은 함수 포인터의 주소를 런타임에
	가지고 오는 함수이다. 해당 함수가 정확히 무슨 방식으로 동작하는지
	알 필요는 없고 단순히 ConnectEx, AcceptEx를 런타임에 불러오기 위한
	함수라는 것만 알고 있어도 상관없다 */
	DWORD bytes = { 0 };
	return SOCKET_ERROR != WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, 
					   	   sizeof(guid), fn, sizeof(*fn), OUT & bytes, NULL, NULL);

}

SOCKET SocketUtils::CreateSocket()
{
	/* socket 함수를 사용해서 소켓을 만들어도 되지만 WSASocket 이라는 
	함수를 사용하면 더욱 세분화 되어 있는 옵션을 설정할 수 있다 */
	return WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

bool SocketUtils::SetLinger(SOCKET socket, uint16 onoff, uint16 linger)
{
	LINGER option;
	option.l_onoff = onoff;
	option.l_linger = linger;

	return SetSocketOpt(socket, SOL_SOCKET, SO_LINGER, option);
}

bool SocketUtils::SetReuseAddress(SOCKET socket, bool flag)
{

	return SetSocketOpt(socket, SOL_SOCKET, SO_REUSEADDR, flag);
}

bool SocketUtils::SetRecvBufferSize(SOCKET socket, int32 size)
{

	return SetSocketOpt(socket, SOL_SOCKET, SO_RCVBUF, size);
}

bool SocketUtils::SetSendBufferSize(SOCKET socket, int32 size)
{

	return SetSocketOpt(socket, SOL_SOCKET, SO_SNDBUF, size);
}

bool SocketUtils::SetTcpNoDelay(SOCKET socket, bool flag)
{

	return SetSocketOpt(socket, SOL_SOCKET, TCP_NODELAY, flag);
}

// Listen 소켓의 특성을 첫 번째 인자값으로 넘겨준 클라 소켓에 동일하게 적용하는 옵션
bool SocketUtils::SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket)
{

	return SetSocketOpt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, listenSocket);
}

bool SocketUtils::Bind(SOCKET socket, NetAddress netAddr)
{
	return SOCKET_ERROR != bind(socket, reinterpret_cast<const SOCKADDR*>(&netAddr.GetSockAddr()),
								sizeof(SOCKADDR_IN));

	return false;
}

bool SocketUtils::BindAnyAddress(SOCKET socket, uint16 port)
{
	SOCKADDR_IN MyAddress;
	MyAddress.sin_family = AF_INET;
	MyAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	MyAddress.sin_port = htons(port);
	
	return SOCKET_ERROR != bind(socket, reinterpret_cast<const SOCKADDR*>(&MyAddress), sizeof(MyAddress));
}

bool SocketUtils::Listen(SOCKET socket, int32 backlog)
{
	// 단순 listen 함수 호출
	return SOCKET_ERROR != listen(socket, backlog);
}

void SocketUtils::Close(SOCKET& socket)
{
	if (socket != INVALID_SOCKET)
	{
		// 두 번 close 하지 않도록 하기 위해서 조건 체크
		closesocket(socket);
	}

	// 소켓을 close 하고 나서 INVALID_SOCKET 상태로 변경
	socket = INVALID_SOCKET;
}
