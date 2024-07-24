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
	int32 iSendBytes = 0;
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

	// WSAEventSelect = WSAEventSelect함수가 핵심이 되는 입출력 모델
	// 소켓과 관련된 네트워크 이벤트를[이벤트 객체]를 통해 감지한다는 특징이있다.

	/*
	이벤트 객체 관련 함수들은 아래와같다
	생성 : WSACreateEvent(수동 리셋 Manual-Reset + Non-Signaled 상태 시작)
	삭제 : WSACloseEvent
	신호 상태 감지 : WSAWaitForMultipleEvents
	구체적인 네트워크 이벤트 확인 : WSAEnumNetworkEvents
	
	WSACreateEvent함수로 이벤트 객체를 생성하고 이벤트 객체와 소켓을 연동시킨다.
	또한, 이벤트 객체와 소켓(세션)은 1대1로 연동시켜야 하기 때문에 소켓의 갯수 만큼 
	이벤트 객체를 생성해줘야 한다는 특징이 있다.

	참고로 위에서 소켓(세션)은 무엇을 의미하는거냐면 위에서 생성해준 Session구조체를
	보면 소켓, 소켓이 데이터를 받아줄 배열, 받아온 데이터의 크기, 전송한 데이터의
	크기를 저장하는 변수가 있는데, 이미 알고 있겠지만 이를 활용해서 송수신이 가능한
	상태인지를 반복 확인하는 불필요한 과정을 방지할 수 있도록 한다.


	이벤트 객체와 소켓을 연동시킬때는 WSAEventSelect(소켓, 이벤트, 네트워크 이벤트)
	이런 식으로 첫 번째 인자값은 관찰 대상으로 지정할 소켓을 넘겨주고 두 번째
	인자값은 관찰 대상인 소켓과 연동시킬 이벤트 객체를 넘겨주고 마지막 세 번째
	인자값은 어떠한 네트워크 이벤트를 감지할 것인지를 넘겨준다.

	관찰할 수 있는 네트워크 이벤트는 여러가지가 있다.
	FD_ACCEPT  : 접속한 클라가 있는지(accept함수가 호출되었는지)
	FD_READ    : 데이터 수신이 가능한지(recv, recvfrom)
	FD_WRITE   : 데이터 송신이 가능한지(send, sendto)
	FD_CLOSE   : 상대방이 접속을 종료했는지
	FD_CONNECT : 통신을 위한 연결 절차가 완료되었는지
	FD_OOB

	WSAEventSelect함수를 사용할 때 몇 가지 주의사항이 있는데, 우선 첫 번째는
	WSAEventSelect를 호출하면 인자값으로 넘겨준 소켓은 자동으로 Non-Blocking
	모드로 전환된다는 것이고, 두 번째는 accept() 함수가 반환하는 클라이언트
	소켓은 ListenSocket과 동일한 속성을 가지고 있으며, 따라서 ClientSocket의
	경우 FD_READ, FD_WRITE 등을 다시 등록할 필요가 있다. 왜냐면 ListenSocket
	은 accept를 이용하기 위해서 사용하기 때문에 가장 먼저 FD_ACCEPT 네트워크
	이벤트를 관찰할 이벤트로 설정할 것이고 따라서 FD_READ, FD_WRITE를 다시
	등록할 필요가 있다. 세 번째로 드물게 WSAEWOULDBLOCK 오류가 발생할 수 
	있기 때문에 예외 처리가 필요할 것이다.

	이벤트 발생 시, 적절한 소켓 함수를 호출해야 한다는 점이다. 자세히 말하자면
	예를 들어서 관찰하고 있는 이벤트 객체가 FD_READ 네트워크 이벤트가 발생했을때
	FD_READ의 경우에는 recv함수를 호출해야 한다는 것이다. 만약 적절하지 않은
	함수를 호출하거나 함수를 호출하지 않는다면 다시는 동일한 네트워크 이벤트가
	발생하지 않는다.
	정리하자면, FD_READ 이벤트 발생 시 recv 함수를 호출해야 하고, 만약 recv
	함수를 호출하지 않는다면 FD_READ 이벤트는 두 번 다시 발생하지 않는다. 
	

	WSAEventSelect 함수는 소켓과 이벤트를 연동시켜주는 역할만 수행하기 때문에
	실질적으로 이벤트가 발생했다는 통지를 받는 것은 WSAWaitForMultipleEvents
	라는 함수를 통해서 별도로 받아줘야한다. 

	WSAWaitForMultipleEvents()함수는 5개의 인자값을 받는 함수이다.
	1) 이벤트의 갯수를 넘겨준다.
	2) 이벤트 배열의 포인터를 넘겨준다.
	3) 모두 기다릴지, 하나만 완료 되어도 반환할지를 설정한다
	4) 대기 시간을 설정한다(무한 대기 방지)
	5) 현재는 사용하지 않은 것이다(우선 false) 
	
	WSAWaitForMultipleEvents 함수를 사용하면 인자값으로 넘겨준 이벤트 
	갯수만큼의 관찰 대상을 등록할 것이며, 반환값으로 가장 먼저 완료가 
	된 이벤트의	인덱스를 DWORD 형태로 반환한다.

	반환값 인덱스를 통해 어떤 이벤트가 완료되었는지는 알 수 있지만 해당
	이벤트가 무엇인지에 대해서는 알 수 없다. 
	그래서 최종적으로 WSAEnumNetworkEvents 함수를 이용할 것이며 인자값은
	아래와 같다.

	1) 반환값 인덱스에 해당 하는 소켓.
	2) 소켓과 연결된 이벤트 객체 핸들을 넘겨주면, 이벤트 객체를 자동으로
	Non-Signaled 상태로 전환해준다.
	3) 관찰하고 있던 네트워크 이벤트 혹은 오류 정보가 저장된다.

	이론적인 설명으로는 많이 헷갈리기 때문에 아래 예시 코드를 참고하자 
	
	*/

	// 위에서 설명했듯 세션과 이벤트는 서로 대응된다
	vector<WSAEVENT> vecWsaEvents;
	vector<Session> vecSessions;
	vecSessions.reserve(100);

	// ListenSocket을 생성할 것이기 때문에 이와 대응되도록 이벤트도 생성
	WSAEVENT ListenEvent = WSACreateEvent();
	vecWsaEvents.push_back(ListenEvent);
	/* 참고로 vecSessions에는 클라이언트의 소켓이 들어가야 하는데 
	ListenSocket을 vecSessions에 저장하는 것이 의아할 수가 있다.
	왜 이렇게 하냐면 소켓과 이벤트가 서로 대응되도록 하여서 동일한
	인덱스를 활용할 수 있도록 해야지 관리하기가 편리하기 때문에 지금
	이렇게 ListenSocket를 넣어주고 있는 것이다 */
	vecSessions.push_back(Session{ ListenSocket });

	
	/* 참고로 세 번째 인자값인 관찰할 네트워크 이벤트의 경우 
	OR 비트연산자를 사용해서 동시에 관찰할 수 있다.
	
	아래와 같은 형태로 WSAEventSelect 함수를 통해 이벤트와 소켓을 서로
	대응되도록 연결을 해주면 된다 */
	if (WSAEventSelect(ListenSocket, ListenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
	{
		
		HandleError("ListenEvent Accept Fail");
		return 0;
	}

	/* 나중에 프로젝트가 커지게 되면 클라이언트가 접속할 때마다
	이벤트와 소켓이 하나씩 늘어나겠지만 현재는 ListenEvent,
	ListenSocket 둘 밖에 없는 상황이다 */

	while (true)
	{
		/* 위에서 설명했든 세 번째 인자값을 false로 설정하여서 
		하나라도 완료가 된 이벤트가 있다면 해당 이벤트에 해당하는 
		인덱스를 반환시킨다 */
		// 참고로 WSAWaitForMultipleEvents를 통해 확인 가능한 
		// 이벤트의 개수는 WSA_MAXIMUM_WAIT_EVENTS(64개)이다.
		int32 iIndex = WSAWaitForMultipleEvents(vecWsaEvents.size(), &vecWsaEvents[0], 
												FALSE, WSA_INFINITE, false);
	
		if (iIndex == WSA_WAIT_FAILED)
		{
			continue;
		}

		/* 반환값 인덱스에 WSA_WAIT_EVENT_0 라는 것을 빼주는 이유는
		WSAWaitForMultipleEvents 함수가 인덱스를 반환할 때 실제로
		WSA_WAIT_EVENT_0 값을 더해준 결과값을 반환하기 때문이다.
		이유는 모르고 문서를 보면 알 수 있음 */
		iIndex -= WSA_WAIT_EVENT_0;

		/* 그리고 WSACreateEvent 함수를 통해 생성한 이벤트는 수동
		으로 리셋을 해줘야 한다. WSAResetEvent 함수를 사용해서 
		수동으로 리셋을 할 수 있는데, WSAEnumNetworkEvents 함수의
		인자값으로 넘겨준 이벤트 객체는 자동으로 Non-Signaled 
		상태로 전환되기 때문에 여기서는 WSAResetEvent 함수를
		사용할 필요가 없다 */
		// WSAResetEvent(vecWsaEvents[iIndex]);

		/* iIndex를 통해서 해당 인덱스의 이벤트가 발생한 대상은 
		알 수 있지만, 어떠한 네트워크 이벤트가 발생했는지는 알
		수 없고, WSAEnumNetworkEvents 함수를 통해서 알 수 있다
		해당 함수의 세 번째 인자값의 경우 WSANETWORKEVENTS 라는
		구조체를 받아주며 해당 인자로 넘겨준 구조체에는 인덱스에
		해당하는 관찰 대상의 정보가 담긴다(까먹었으면 F12) */
		WSANETWORKEVENTS NetworkEvents;
		if (WSAEnumNetworkEvents(vecSessions[iIndex].socket, vecWsaEvents[iIndex],
			&NetworkEvents) == SOCKET_ERROR)
		{
			continue;
		}

		
		/* WSANETWORKEVENTS 구조체로 받아온 정보를 바탕으로 오류
		발생, 발생한 네트워크 이벤트의 종류 등을 확인하면 된다 
		비트플레그 AND 연산자를 통해서 Listenr 소켓이 FD_ACCEPT
		이벤트가 발생한 것인지 확인한다(물론, 발생한 이벤트가 
		여러개가 존재할 수가 있다) */
		if (NetworkEvents.lNetworkEvents & FD_ACCEPT) 
		{
			// 에러 확인
			if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
				continue;


			// 여기까지 왔다면 문제가 없다는 뜻이기에 클라와 accept(연결)한다
			SOCKADDR_IN ClientAddr;
			int32 iAddrLen = sizeof(ClientAddr);
			
			SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &iAddrLen);
			if (ClientSocket != INVALID_SOCKET)
			{
				cout << "Client Connected" << '\n';

				// 받아온 클라이언트 소켓과 대응되는 이벤트 객체 생성
				WSAEVENT ClientEvent = WSACreateEvent();
				vecWsaEvents.push_back(ClientEvent);
				vecSessions.push_back(Session{ ClientSocket });

				// WSAEventSelect 함수를 통해 클라 소켓과 이벤트 연결
				if (WSAEventSelect(ClientSocket, ClientEvent, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
				{
					HandleError("ClientEvent Accept Fail");
					return 0;
				}
			}
		}

		// Client Session 소켓 체크
		if (NetworkEvents.lNetworkEvents & FD_READ ||
			NetworkEvents.lNetworkEvents & FD_WRITE)
		{
			// Read 이벤트가 발생 시 에러 확인
			if ((NetworkEvents.lNetworkEvents & FD_READ) &&
				(NetworkEvents.iErrorCode[FD_READ_BIT] != 0))
			{
				continue;
			}

			if ((NetworkEvents.lNetworkEvents & FD_WRITE) &&
				(NetworkEvents.iErrorCode[FD_WRITE_BIT] != 0))
			{
				continue;
			}

			// 문제가 없다면 네트워크 이벤트가 발생한 세션에 접근
			Session& ClientSession = vecSessions[iIndex];

			// 아직 아무 데이터도 받지 않은 상태
			if (ClientSession.iRecvBytes == 0)
			{
				int32 iRecvLen = recv(ClientSession.socket, ClientSession.recvBuffer,
									  BUFSIZE, 0);
				if (iRecvLen == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
				{
					// TODO : Remove Session
					continue;
				}

				ClientSession.iRecvBytes = iRecvLen;
				cout << "Recv Data = " << iRecvLen << '\n';
			}
			// 아직 보낼 데이터가 남아있는 상태
			if (ClientSession.iRecvBytes > ClientSession.iSendBytes)
			{

				int32 iSendLen = send(ClientSession.socket, &ClientSession.recvBuffer[ClientSession.iSendBytes],
					ClientSession.iRecvBytes - ClientSession.iSendBytes, 0);

				if (iSendLen == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
				{
					// TODO :  Session
					continue;
				}

				ClientSession.iSendBytes += iSendLen;
				if (ClientSession.iSendBytes == ClientSession.iRecvBytes)
				{
					ClientSession.iRecvBytes = 0;
					ClientSession.iSendBytes = 0;
				}
				cout << "Send Data = " << iSendLen<< '\n';
			}
		}

		// FD_CLOSE 처리
		if (NetworkEvents.lNetworkEvents & FD_CLOSE)
		{
			// TODO : Remove Socket
		}


	}

	WSACleanup();

	return 0;
}
