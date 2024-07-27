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

const int32 BUFSIZE = 1000;

/* 나중에 클라이언트가 서버에 접속을 하게
될 때 클라이언트의 정보를 관리하는 구조체 */
struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char  recvBuffer[BUFSIZE] = {};
	int32 iRecvBytes = 0;
	WSAOVERLAPPED overlapped = {};
};

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

	// Overlapped IO (비동기 + 논블로킹)
	
	
	// 1. Overlapped 함수를 호출한다(WSARecv, WSASend)
	// 2. Overlapped 함수가 성공했는지 확인 후	성공했다면 
	// 결과를 얻어서 처리 하고 실패했다면 사유를 확인한다.
	// 3. 실패 사유가 Pending(보류 중)일 경우 추후 완료가
	// 되었을때 이벤트 방식과 콜백 방식 두 가지 방법 중 
	// 하나로 통지해달라는 요청을 한다 

	/* 우선 이벤트 방식을 사용할 것이며 Overlapped 계열 함수는 아래와같다.
	WSASend, WSARecv, AcceptEx, ConnectEx 참고로 여기서 Ex가 붙은 두 함수는
	나중에 알아볼 것이다.

	WSASend, WSARecv는 함수 인자값이 비슷하며 세부사항은 아래와 같다

	1) 비동기 입출력 소켓
	2) WSABUF 구조체 배열의 시작 주소 + 개수
	WSABUF 배열의 시작 주소를 받는 이유는 WSABUF 구조체를 배열 형태로 
	여러개 만들어서 사용할 경우 한 번에 인자로 넘겨줄 수 있기 때문이다
	이렇게 여러개의 버퍼를 한 번에 넘겨주는 상황으로 Scatter-Gather라는
	쪼개져 있는 버퍼들을 한 번에 보내준다는 개념의 기법이 있다(나중에
	패킷을 만들어서 전달할 때 유용하게 사용할 예정)
	3) 보내고/받은 바이트 수
	4) 상세 옵션(기본 0)
	5) WSAOVERLAPPED 구조체 주소값(사용 예시 중 하나로 해당 구조체에게 
	이벤트 핸들을 넘겨줘서 해당 핸들값을 통해서 이벤트 시그널 상태를 
	탐지해서 완료되었는지 판별하는 것이 있다)
	6) 입출력이 완료되면 OS(운영체제)가 호출할 콜백 함수
	

	Overlapped 모델(이벤트 기반) 사용 방법

	1. 비동기 입출력을 지원하는 소켓과, 통지 받기 위한 이벤트 객체 생성한다
	2. 비동기 입출력 함수(WSASend, WSARecv)을 호출하고 통지 받기 위해 생성한
	이벤트 객체를 같이 넘겨준다(WSAOVERLAPPED 구조체에 담아서 인자값으로 전달)
	3. 비동기 작업이 바로 완료되지 않는다면 WSA_IO_PENDING 오류 코드가 발생하는데
	이러한 경우 요청한 작업이 당장 실행되지 않았고 이후에 완료가 되면 통지가 
	오겠다고 예상할 수 있으며 오류가 발생하지 않았다면 성공적으로 완료가 된 것이다.
	4. WSA_IO_PENDING 오류가 발생해서 나중에 완료가 된다고 했을 경우 나중에 
	운영체제가 백그라운드에서 요청받은 Send, Recv를 처리하다가 완료가 되면
	이벤트 객체를 통해서 Signaled 상태로 전환해서 요청한 대상에게 통지를 하고
	요청을 대상은 이벤트 객체가 Signaled 상태로 되어 있는지를 탐지해서 만약
	Signaled 상태로 되어 있다면 요청한 작업이 완료되었다고 인지를 하고 그 다음
	작업을 진행하는 방식으로 작동한다.
	이전에 사용해봤던 WSAWaitForMultipleEvents 함수를 호출해서 이벤트 객체의
	Signaled 상태를 판별하고, 이후 WSAGetOverlappedResult 함수를 호출해서 
	비동기 입출력 결과 및 데이터를 처리하면 된다

	WSAGetOverlappedResult 함수의 인자값 정보는 아래와같다
	1) 비동기 소켓
	2) 넘겨준 Overlapped 구조체
	3) 전송된 바이트 수 
	4) 비동기 입출력 작업이 끝날때까지 대기할지에 대한 여부 설정(참고로 
	WSAWaitForMultipleEvents 함수를 사용해서 이벤트 객체의 Signaled 상태를
	판별 후 WSAGetOverLappedResult 함수를 호출하는 상황이라면 이미 비동기
	작업이 끝났다는 의미이기 때문에 false로 설정하면 된다)
	5) 비동기 입출력 작업 관련 부가 정보(기본적으로 NULL 사용 빈도 매우 낮음) */

	/* 이번에는 Send, Recv의 경우에만 비동기 방식을 활용할 것이고 
	Accept의 경우에는 사전 작업이 필요하기 때문에 나중에 사용해볼 
	것이며 또한, 기존과는 다르게 에코서버 형태로 사용하지 않고 
	클라에서는 Send만 하고 서버에서는 Recv만 처리하는 형태로 
	작업할 것이다. 
	구조 관련해서는 IOCP에 관한 내용을 다룰때부터 작업할 것이다 */

	while (true)
	{
		SOCKADDR_IN ClientAddr;
		int32 iAddrLen = sizeof(ClientAddr);
		SOCKET ClientSocket;

		while (true)
		{
			/* 현재 소켓 자체가 논블로킹 소켓이기 때문에 accept 함수가 
			완료되지 않더라도 대기하지 않고 빠져나올 것이기 때문에 
			반복문을 통해서 받아온 소켓이 INVALID_SOCKET가 아닐 때까지
			즉, 유효한 소켓을 받아올 때까지 계속해서 확인하는 것이다 */
			ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &iAddrLen);
			if (ClientSocket != INVALID_SOCKET)
				break;

			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				/* 에러가 발생하였으나, WSAEWOULDBLOCK 에러인 경우에는 
				문제상황이 아니라, 아직 연결한 대상이 없다는 의미이기
				때문에 continue 해준다 */
				continue;
			}

			// 여기까지 도달하면 정말로 문제가 발생한 경우이다
			return 0;
		}

		Session  session = Session{ ClientSocket };
		/* 당연한 얘기지만 위 session에 대한 입출력 통지를
		해당 이벤트 객체를 통해서 받을 것이다 */
		WSAEVENT wsaEvent = WSACreateEvent();
		
		// WSAOVERLAPPED에게 이벤트 핸들을 넘겨준다
		session.overlapped.hEvent = wsaEvent;

		cout << "Client Connected !" << '\n';

		while (true)
		{
			/* 참고로 아래 WSABUF 구조체 메모리는 Recv 호출 후 날려버려도 
			상관없으나 session의 RecvBuffer자체는 건들이면 안된다 */
			WSABUF wasBuf;
			wasBuf.buf = session.recvBuffer;
			wasBuf.len = BUFSIZE;

			DWORD recvLen = 0;
			DWORD flags = 0;

			/* 동작 방식을 대략적으로 요약하자면 클라와 Accept(연결)을
			한 다음 클라에서 데이터를 전송하여 해당 데이터를 서버에서
			Recv 함수를 통해서 데이터를 받아줄 때 해당 Recv 함수가 
			당장 완료가 될 수도 있지만, 데이터가 모두 전달이 되지 
			않아서 나중에 데이터가 모두 전송이 끝났을때 데이터를 모두
			받아왔다는 통지를 받을 수 있도록 WSARecv 함수를 걸어놓는
			개념이다. 따라서 무한 루프를 돌면서 계속해서 Recv를 
			해주는 형태로 동작하게 될 것이다 */
			if (WSARecv(ClientSocket, &wasBuf, 1, &recvLen, &flags,
				&session.overlapped, nullptr) == SOCKET_ERROR)
			{
				/* SOCKET_ERROR가 발생했다면 여러가지 상황이 있을 수 
				있는데 클라에서 서버에 Send를 해서 현재 Recv버퍼에
				데이터가 모두 전달된 경우라면 바로 성공을 하겠지만, 
				데이터가 전송되고 있는 중이라면 Pending(보류)상태가
				될 것이며 데이터가 모두 전송이 되지 않았으니 모두
				전송이 되어서 완료가 되는 시점에 통지를 해달라는
				요청을 해놓으면 된다 */

				if (WSAGetLastError() == WSA_IO_PENDING)
				{
					/* 발생한 오류가 WSA_IO_PENDING 이라면 실제로
					문제가 발생한것은 아니고 당장 받을 데이터가 
					없어서 PENDING 상태가 된 것이다 */

					/* WSAWaitForMultipleEvents 함수를 사용해서 완료가
					되었을때 통지를 받을 수 있도록 한다 
					함수 인자값을 보면 알겠지만 모든 이벤트가 완료가
					될 때까지 대기하며 대기 시간은 무한으로 설정했다 */
					WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, false);
					
					/* WSAWaitForMultipleEvents 함수에서 빠져나와서 이벤트객체가
					Signaled 상태로 전환되었다면 WSAGetOverlappedResult 함수를
					사용해서 결과값을 확인할 수 있다 */
					WSAGetOverlappedResult(session.socket, &session.overlapped,
						&recvLen, FALSE, &flags);
					
					/* 결론적으로 WSARecv 함수가 바로 성공했다면 recvLen에 데이터가 
					바로 채워지게 될 것이고, 만약 실패했다면 WSAWaitForMultipleEvents 
					함수를 통해서 데이터를 받을 때까지 대기를 해서 데이터를 받아온 후  
					WSAGetOverlappedResult 함수를 통해서 recvLen, overlapped 구조체에
					값을 채워주는 것이다 */
				}
				else
				{
					// TODO : 문제 상황
					break;
				}
			}
			
			cout << "Data Recv Len = " << recvLen <<'\n';
		}
		
		closesocket(session.socket);
		WSACloseEvent(wsaEvent);
	}

	WSACleanup();

	return 0;
}
