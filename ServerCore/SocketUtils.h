#pragma once
#include "NetAddress.h"

/*---------------------
	   SocketUtils
-----------------------*/

class SocketUtils
{
public:
	// 
	static LPFN_CONNECTEX	 ConnectEx;
	static LPFN_DISCONNECTEX DisConnectEx;
	static LPFN_ACCEPTEX	 AcceptEx;	

public:
	/* Init, Clear �Լ��� ���� CoreGlobal Ŭ������ ����/�Ҹ� ������
	ȣ���ϰ� ���� */
	static void Init();
	static void Clear();

	static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
	static SOCKET CreateSocket();

	/* �ؿ� Set �Լ����� ���� �ɼ� ���õ� �Լ����ε� ����꿡
	Socket Option Ŀ�� �ּ��� ���� ���� ���� �ۼ��س��� */

	/* ������ close ���� �� ���۵��� ���� �����͸� ��� 
	ó���� �������� ���� �ɼ��� SO_LINGER �ɼ��̴� */
	static bool SetLinger(SOCKET socket, uint16 onoff, uint16 linger);
	//  IP�ּ� �� ��Ʈ ���� ���ο� ���� �ɼ�
	static bool SetReuseAddress(SOCKET socket, bool flag);
	// ������ ����
	static bool SetRecvBufferSize(SOCKET socket, int32 size);
	static bool SetSendBufferSize(SOCKET socket, int32 size);
	static bool SetTcpNoDelay(SOCKET socket, bool flag);
	static bool SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket); 


	static bool Bind(SOCKET socket, NetAddress netAddr);
	// �ش� ���Ͽ��ٰ� ������ IP �ּҸ� ���ε� �ϴ� �Լ�
	static bool BindAnyAddress(SOCKET socket, uint16 port);
	// ��������� ���ڸ� �Է����� �ʰ� �˾Ƽ� �ִ� ���ڷ� ���޶�� �ɼ�(SOMAXCONN)
	static bool Listen(SOCKET socket, int32 backlog = SOMAXCONN);
	static void Close(SOCKET& socket);

};

template<typename T>
static inline bool SetSocketOpt(SOCKET socket, int32 level, int32 optName, T optVal)
{
	// ���� �ɼ��� ���������� ����ϱ� ���� ���ø����� ����
	return SOCKET_ERROR != setsockopt(socket, level, optName, 
		reinterpret_cast<char*>(&optVal), sizeof(T));
}
