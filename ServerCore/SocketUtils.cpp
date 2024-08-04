#include "pch.h"
#include "SocketUtils.h"

/*---------------------
	   SocketUtils
-----------------------*/

/* Connect, DisConnect, Accept �Լ��� �����͸� �ǹ��ϸ� ��Ÿ�ӿ�
�ǽð����� �Լ����� �ּҸ� ������ �� ���̴�(������ ��ȣ��) */
LPFN_CONNECTEX	  SocketUtils::ConnectEx = { nullptr };
LPFN_DISCONNECTEX SocketUtils::DisConnectEx = { nullptr };
LPFN_ACCEPTEX	  SocketUtils::AcceptEx = { nullptr };

void SocketUtils::Init()
{
	WSADATA wsaData;
	// WSAStartup �Լ� ȣ���� �����ϸ� 0�� ��ȯ
	ASSERT_CRASH(WSAStartup(MAKEWORD(2, 2), OUT &wsaData) == 0);

	SOCKET DummySocket = CreateSocket();
	/* ������� BindWindowsFunction �Լ��� ù ��° ���ڰ�����
	���� ������ �Ѱ��ְ� �� ��° ���ڰ����δ� ã�ƿ��� ����
	�Լ��� �־��ش�. ���������� �� ��° ���ڰ����δ� �Լ�
	�����͸� �Ѱ��ش� */
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
	/* ConnectEx, AcceptEx ��� ���� �Լ� �������� �ּҸ� ��Ÿ�ӿ�
	������ ���� �Լ��̴�. �ش� �Լ��� ��Ȯ�� ���� ������� �����ϴ���
	�� �ʿ�� ���� �ܼ��� ConnectEx, AcceptEx�� ��Ÿ�ӿ� �ҷ����� ����
	�Լ���� �͸� �˰� �־ ������� */
	DWORD bytes = { 0 };
	return SOCKET_ERROR != WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, 
					   	   sizeof(guid), fn, sizeof(*fn), OUT & bytes, NULL, NULL);

}

SOCKET SocketUtils::CreateSocket()
{
	/* socket �Լ��� ����ؼ� ������ ���� ������ WSASocket �̶�� 
	�Լ��� ����ϸ� ���� ����ȭ �Ǿ� �ִ� �ɼ��� ������ �� �ִ� */
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

// Listen ������ Ư���� ù ��° ���ڰ����� �Ѱ��� Ŭ�� ���Ͽ� �����ϰ� �����ϴ� �ɼ�
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
	// �ܼ� listen �Լ� ȣ��
	return SOCKET_ERROR != listen(socket, backlog);
}

void SocketUtils::Close(SOCKET& socket)
{
	if (socket != INVALID_SOCKET)
	{
		// �� �� close ���� �ʵ��� �ϱ� ���ؼ� ���� üũ
		closesocket(socket);
	}

	// ������ close �ϰ� ���� INVALID_SOCKET ���·� ����
	socket = INVALID_SOCKET;
}
