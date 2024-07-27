#include "pch.h"
#include <iostream>

//네트워크 프로그래밍을 시작하기 위한 라이브러리 추가
#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


void HandleError(const char* _Cause)
{
	int32 iErrCode = WSAGetLastError();
	cout << _Cause << " ErrorCode : " << iErrCode << '\n';
}

int main()
{
	this_thread::sleep_for(3s);

	// 윈속 라이브러리 초기화(ws2_32 라이브러리 초기화)
	// 관련 정보가 wsaData에 채워진다
	WSAData wsaData;
	// 초기화를 실패하면 0이 아닌 값을 반환한다
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return 0;
	}
	
	// 논블로킹(Non-Blocking)
	SOCKET ClientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ClientSocket == INVALID_SOCKET)
	{
		HandleError("Socket");
		return 0;
	}

	u_long On = 1;
	if (ioctlsocket(ClientSocket, FIONBIO, &On) == INVALID_SOCKET)
	{
		HandleError("Socket");
		return 0;
	}

	SOCKADDR_IN ServerAddr;
	memset(&ServerAddr, false, sizeof(ServerAddr));
	ServerAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "192.168.0.5", &ServerAddr.sin_addr);
	ServerAddr.sin_port = htons(7777);


	while (true)
	{
		if (::connect(ClientSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				/* 해당 조건문에 들어왔다면 Connect의 경우에는 연결을 진행하고
				있다는 의미가 된다(Server의 경우에는 연결 여부를 의미)
				여기서 그냥 break을 해도 되기는 하다 */
				continue;
			}

			// 따라서 위와 같은 이유로 인해서 Connect를 시도를 하였고,
			// 연결을 진행중에 있기 때문에 계속해서 연결을 시도 하지 
			// 않도록 한 번 더 조건처리를 해서 이미 연결되었다면 break
			if (WSAGetLastError() == WSAEISCONN)
				break;

			break;
		}
	}

	cout << "Connect To Server" << '\n';

	// 데이터 전송
	char SendBuffer[100] = "Hello World";
	WSAEVENT wsaEvent = WSACreateEvent();
	WSAOVERLAPPED overlapped = {};
	overlapped.hEvent = wsaEvent;

	while (true)
	{
		WSABUF wsaBuf;
		wsaBuf.buf = SendBuffer;
		wsaBuf.len = sizeof(SendBuffer);

		DWORD sendLen = 0;
		DWORD flags = 0;

		if (WSASend(ClientSocket, &wsaBuf, 1, &sendLen, flags, 
			&overlapped, nullptr) == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSA_IO_PENDING)
			{
				/* 마찬가지로 상대방이 데이터를 받지 않아서 커널 버퍼의 
				SendBuffer가 가득 차 있는 상태이고, 때문에 SendBuffer에
				전송할 데이터를 복사할 수 없기 때문에 PENDING 상태가
				되었다는 것을 의미한다 */
				
				
				WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, false);
				WSAGetOverlappedResult(ClientSocket, &overlapped, &sendLen,
					FALSE, &flags);

			}
			else
			{
				// TODO : 문제 상황
				break;
			}
		}

		cout << "Send Data Len = " << sizeof(SendBuffer) << '\n';
		
		this_thread::sleep_for(1s);
	}

	closesocket(ClientSocket);
	WSACloseEvent(wsaEvent);

	// 윈속 종료(참고로 WSAStartup함수 호출 횟수 만큼 호출을 해줘야 한다)
	// 호출을 하지 않더라도 문제가 발생하지 않는다(필수사항은 아니고 권장사항이다)
	WSACleanup();
}
