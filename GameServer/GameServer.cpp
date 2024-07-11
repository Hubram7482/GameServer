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
	/* 블로킹(Blocking) 소켓
	accept, connect, send, sendto, recv, recvfrom 이러한 함수들은 모두 특정 조건이
	성립할때까지 대기를 한다는 특징이 있는데, 게임을 제작하는 경우 대기 상황이
	자주 발생하면 여러가지 문제점이 발생할 수 있으며 이러한 문제점을 완화하고자
	논블로킹 방식을 사용해야 하는데, 당연하게도 모든 문제가 해결되지는 않지만
	일부 개선할 수 있다 */


	// 논블로킹(Non-Blocking)
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (ListenSocket == INVALID_SOCKET)
	{
		HandleError("Socket");
		return 0;
	}

	// 논블로킹 방식을 사용하려면 아래와 같이 사용하면 된다는데, 자세한
	// 설명은 따로 없고 그냥 이렇게 사용하면 된다고한다(궁금하면 문서)
	u_long On = 1;
	if (ioctlsocket(ListenSocket, FIONBIO, &On) == INVALID_SOCKET)
	{
		HandleError("Socket");
		return 0;
	}

	SOCKADDR_IN ServerAddr;
	memset(&ServerAddr, false, sizeof(ServerAddr));

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	// inet_pton(AF_INET, "192.168.0.5", &ServerAddr.sin_addr);

	ServerAddr.sin_port = htons(7777);

	if (::bind(ListenSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
	{
		HandleError("Bind");
		return 0;
	}

	// 명시적으로 숫자를 입력하지 않고 알아서 최대 숫자로 골라달라는 옵션(SOMAXCONN)
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		HandleError("Listen");
		return 0;
	}

	cout << "Accept" << '\n';

#pragma region 블로킹 방식
	{
		/* 블로킹 소켓 방식에서의 Accept 방식의 경우에는 아래와 같이 클라이언트의
		주소를 받아오고 있었고 받아온 ClientSocket이 INVAILD_SOCKET이 발생하면
		무조건 문제가 발생했다고 판별하고 있었는데 논블로킹 방식에서는 이러한
		상황에서 무조건 문제 상황이라고 확정지을 수가 없다.왜냐하면 애초에
		논블로킹 방식은 대기 상황을 만들지 않기 때문에 성공적으로 완료되지는
		않았지만 이게 꼭 문제가 있어서 발생한 상황이 아닐 수도 있다는 것이다.
		따라서, 조건처리를 한 번 더 해줘야 한다.

		SOCKADDR_IN ClientAddr;
		int32 iAddrLen = sizeof(ClientAddr);

		SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &iAddrLen);
		if (ClientSocket != INVAILD_SOCKET)
		{
			문제상황 발생(블로킹 방식)
		} */
	}
#pragma endregion

#pragma region 논블로킹 방식

	SOCKADDR_IN ClientAddr;
	int32 iAddrLen = sizeof(ClientAddr);

	while (true)
	{

		SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &iAddrLen);
		if (ClientSocket == INVALID_SOCKET)
		{
			// 또한 WSAGetLastError()함수로 받아온 에러 코드의 타입을 확인해서
			// 어떠한 에러가 발생했는지 확인하는 것도 유용하다(문서 참고)
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				/* 해당 조건문에 들어왔다는 연결 요청한 클라이언트의 데이터가 아직
				연결이 되지 않은 상황이지만, 이게 문제 상황은 아니고 대기 상황을
				방지하고자 강제로 빠져나오게 했는데 빠져나올 때까지 연결이 완료
				되지는 않았다는 의미이다.따라서, 이러한 경우에는 무한 루프를 통해
				계속해서 연결을 시도할 것이라서 continue를 통해서 계속 반복한다 */
				continue;
			}

			// Error발생
			break;
		}

		// 클라이언트 연결 완료
		cout << "Client Connected" << '\n';

		// Recv의 경우에도 Accept와 동일한 문제가 발생한다
		while (true)
		{
			char RecvBuffer[1000];
			int32 iRecvLen = recv(ClientSocket, RecvBuffer, sizeof(RecvBuffer), 0);
			/* 블로킹 방식에서는 iRecvLen의 값이 SCOKET_ERROR과 같다면 문제가
			발생했다고 판단을 했었는데, 이 또한 마찬가지로 Accept와 동일한
			이유로 무조건 문제 상황은 아니고 무한 반복하면서 계속 Recv(수신)
			시도하면서 정말 문제가 발생한건지 추가로 조건 처리를 해줘야한다 */

			if (iRecvLen == SOCKET_ERROR)
			{
				if (WSAGetLastError() == WSAEWOULDBLOCK)
					continue;

				break;
			}
			else if (iRecvLen == 0)
			{
				// 0인 경우 무조건 연결이 끊긴 상황이라서 break
				break;
			}

			cout << "Recv Data Len" << iRecvLen << '\n';

			// Send 또한 동일한 문제가 있다
			while (true)
			{
				if (send(ClientSocket, RecvBuffer, iRecvLen, 0) == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
						continue;

					break;
				}

				cout << "Send Data Len = " << iRecvLen << '\n';

				break;
			}
		}
	}
	
#pragma endregion 

	/* 블로킹 방식에서 논블로킹 방식으로 전환할 경우 오히려 성능이 더욱 저하가 될 것인데
	왜냐하면 현재 테스트하는 환경은 1:1로 데이터를 주고 받는 상황이고, 데이터를 송수신
	하는 경우가 별로 없고 명확한 데이터 송수신 순서가 있디 때문에 계속해서 반복해서 
	검사를 하는 부분에서 CPU사이클을 많이 소요하기 때문에 성능저하가 발생하는 것이다.
	따라서 이러한 상황에서는 대기를 하는 것이 더욱 효울적일수 있으며 상황에 맞게
	적절하게 사용해야 한다는 것이 결론이다.

	블로킹   -> 데이터 송수신 요청이 발생할 때까지 대기
	논블로킹 -> 데이터 송수신이 발생했는지 계속해서 검사 
	
	위와 같은 단점들을 보완하기 위해 사용하는 방식을 소켓 모델이라고 부른다. */
	WSACleanup();

	return 0;
}
