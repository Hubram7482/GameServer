#include "pch.h"

#include <atomic>
#include <mutex>
#include <future>
#include <thread>
#include <functional>
#include <queue>
#include "ThreadManager.h"
#include "Allocator.h"
#include "LockFreeStack.h"


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
	// 윈속 라이브러리 초기화(ws2_32 라이브러리 초기화)
	// 관련 정보가 wsaData에 채워진다
	WSAData wsaData;
	// 초기화를 실패하면 0이 아닌 값을 반환한다
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return 0;
	}

	/* TCP서버의 경우에는 10개의 클라이언트와 통신을 할 경우 1개의 Listen소켓과 
	10개의 클라이언트 소켓이 필요했는데 UDP는 1개의 소켓만 있으면 충분하다 */
	SOCKET ServerSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (ServerSocket == INVALID_SOCKET)
	{
		HandleError("Socket");
		return 0;
	}

	SOCKADDR_IN ServerAddR;
	memset(&ServerAddR, false, sizeof(ServerAddR));
	ServerAddR.sin_family = AF_INET;
	ServerAddR.sin_addr.s_addr = htonl(INADDR_ANY);
	// inet_pton(AF_INET, "192.168.0.5", &ServerAddR.sin_addr);
	ServerAddR.sin_port = htons(7777);

	if (::bind(ServerSocket, (SOCKADDR*)&ServerAddR, sizeof(ServerAddR)) == SOCKET_ERROR)
	{
		HandleError("Bind");
		return 0;
	}

	/* TCP는 위 작업들이 끝나면 커넥트 요청을 받아서 연결을 하고 나서 데이터를 
	받아 주는 방식으로 동작하였는데, 반면 UDP는 바로 데이터를 받아 주면 된다 */

	while (true)
	{
		SOCKADDR_IN ClientAddR;
		memset(&ClientAddR, 0, sizeof(ClientAddR));
		int32 iAddrLen = sizeof(ClientAddR);

		char RecvBuffer[1000];
		// UDP는 TCP와는 다르게 recvfrom이라는 함수를 사용한다
		int32 iRecvLen = recvfrom(ServerSocket, RecvBuffer, sizeof(RecvBuffer), 0, (SOCKADDR*)&ClientAddR, &iAddrLen);
		
		if (iRecvLen <= 0)
		{
			HandleError("RecvFrom");
			return 0;
		}

		cout << "Recv Data! Data = " << RecvBuffer << '\n';
		cout << "Recv Data! Len = " << iRecvLen << '\n';

		/* 받아온 데이터를 다시 클라이언트에 전송 해서 출력을 해볼 것인데 
		sendto라는 함수를 사용하며, TCP에서는 UDP의 ServerSocket을 
		ListenSocket용도로만 사용했는데 UDP는 데이터를 보내주거나 하는 등의 
		작업도 ServerSocket을 통해서 처리하는 것을 알 수 있다. 또한,
		connect(연결)을 하지 않고도 데이터를 전송한다는 것도 알 수 있다*/
		int32 iErrorCode = sendto(ServerSocket, RecvBuffer, iRecvLen, 0,
			  (SOCKADDR*)&ClientAddR, sizeof(ClientAddR));
	
		if (iErrorCode == SOCKET_ERROR)
		{
			// 참고로 테스트를 위해서 바로 return을 하는 것이다
			HandleError("SendTo");
			return 0;
		}	

		cout << "SendTo Data! Len = " << iRecvLen << '\n';
	}

	WSACleanup();

	return 0;
}
