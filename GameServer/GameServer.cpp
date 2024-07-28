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

void CALLBACK RecvCallback(DWORD error, DWORD recvLen, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	cout << "Data Recv CallBack = " << recvLen << '\n';
	// TODO : 에코 서버를 만든다면 WSASend()
	// 참고로 에코 서버란 클라이언트의 데이터를 그대로 되돌려주는 것을 뜻한다

	/* 비동기 함수의 인자값으로 넣어준 콜백 함수이며, 인자값을 통해서 여러 
	정보를 받아오지만 이 중 실질적으로 유용한 정보는 없다. 왜냐하면 첫 번째
	인자는 단순 Error여부이고 두 번째 인자는 받아온	Byte 개수이고 마지막 
	인자값은 단순 0을 의미하기 때문에 결국 운영체제가 알려줄 수 있는 정보는 
	overlapped 뿐인데 해당 overlapped 구조체도 유용한 정보는 이벤트 핸들
	밖에 없으며 이벤트 방식을 사용하지 않을 경우 쓸모가 없어진다.

	이러한 이유들로 인해서 여러 클라이언트 소켓이 존재하고 해당 RecvCallback
	함수가 호출됐을때, 함수를 호출한 클라이언트가 어떤 클라인지를 구별할 수
	없는 문제가 있다. 

	하지만 세 번째 인자값인 WSAOVERLAPPED 포인터를 넘겨줄 때 WSAOVERLAPPED
	포인터만을 넘겨줘야할 필요가 없다는 특징이 있는데, 이게 무슨 말이냐면
	예를 들어서 WSAOVERLAPPED를 포함하고 있는 구조체가 있고 WSAOVERLAPPED
	변수가 가장 먼저 선언되어 있으면 사실상 해당 구조체의 시작 주소가
	WSAOVERLAPPED를 나타내게 될 것이며 이를 통해서 다른 정보들을 담아서
	가지고 올 수 있다. 예시는 아래와 같다.

	struct Test
	{
		WSAOVERLAPPED overlapped;
		int iAtt;
	}
	
	Test test;

	RecvCallback 함수 호출 시 세번째 인자값으로 &test.overlapped를 넘겨줄
	수 있으며, RecvCallback 함수의 인자값으로 받아온 overlapped는 구조체의
	시작 주소를 의미하기 때문에 캐스팅을 통해서 구조체에 담긴 다른 정보를
	사용할 수 있다.

	Test* pTest = (Test*)overlapped;
	pTest->iAtt = 5; */
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

	
	/* WSASend, WSARecv는 함수 인자값이 비슷하며 세부사항은 아래와 같다

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
	
	6번째 인자값으로 넘겨주는 콜백 함수(함수 포인터)를 통해서 받아 올 
	데이터들의 정보는 아래와 같다.

	6-1) 오류 발생시 0이 아닌 값
	6-2) 전송 바이트 수
	6-3) 비동기 입출력 함수 호출 시 넘겨준 WSAOVERLAPPED 구조체의 주소값
	6-4) 사용하지 않을 것이기 때문에 0을 넘겨준다.


	Overlapped 모델(콜백(Completion Routine) 기반) 사용 방법

	1. 비동기 입출력을 지원하는 소켓 생성
	2. 비동기 입출력 함수 호출(비동기 작업 완료 시점에 호출할 콜백 함수의 
	시작 주소를 넘겨준다(함수 포인터))

	3. 비동기 작업이 바로 완료되지 않는다면 WSA_IO_PENDING 오류 코드가 발생하며
	해당 오류 코드가 의미하는 것은 비동기 작업이 진행중이라는 뜻이다. 따라서, 
	해당 오류는 문제상황이 아니다.
	
	4. 비동기 입출력 함수를 호출한 쓰레드를 Alertable Wait 상태로 만든다.
	Alertable Wait 상태가 의미하는 것은 비동기 작업이 완료되었고, 콜백 
	함수를 호출할 수 있도록 대기하고 있다는 것을 의미한다.
	왜 이런 식으로 Alertable Wait 상태를 확인해서 콜백 함수를 호출하는
	것이냐면, 예를 들어서 락을 잡아서 빠르게 처리해야 하는 작업을 
	처리 중인 상황에서 중간에 콜백 함수가 호출이 되면 의도한대로 빠르게
	처리하기 어려울 것이기 때문에 이를 방지하기 위해서 상태를 확인한다.

	Alertable Wait 상태로 만들기 위해 사용되는 함수는 아래와 같다.
	WaitForSingleObjectEx, WaitForMultipleObjectsEx, SleepEx,
	WSAWaitForMultipleEvents 등이 있다.

	5. 콜백 함수(완료 루틴)의 호출이 모두 끝나면 쓰레드는 Alertable Wait
	상태에서 빠져나온다 
	
	APC(Asynchronous Procedure Call) : 비동기적으로 실행되는 함수호출을 의미한다

	APC Queue는 비동기 입출력 결과 저장을 위해 운영체제가 각 쓰레드마다 할당하는 
	메모리 영역이다. 해당 영역은 각 쓰레드마다 독립적이며 비동기적으로 호출해야 
	하는 함수들과 매개변수 정보가 저장된다.
	
	APC Queue에 저장된 콜백 함수들은 쓰레드가 Alertable Wait 상태일때 호출된다.

	쓰레드는 자신의 APC Queue 에 있는 모든 콜백 함수를 하나씩 함수를 호출한다.
	즉 Alertable Wait 상태에 한 번 진입하면 현재 APC Queue에 저장되어 있는
	콜백 함수들을 처리한다. 콜백 함수가 호출이 끝났다면 Alertable Wait 상태에서 
	빠져나와서 이어서 다음 줄부터 실행하는 흐름이다.

	*/

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

			if (WSARecv(ClientSocket, &wasBuf, 1, &recvLen, &flags,
				&session.overlapped, RecvCallback) == SOCKET_ERROR)
			{
				if (WSAGetLastError() == WSA_IO_PENDING)
				{
					// 콜백 방식이기 때문에 Alertable Wait 상태로 바꿔야한다.

					/* WSAWaitForMultipleEvents는 다수의 이벤트를 확인하는 
					방식으로 만들 수 있었는데 해당 함수는 64개의 이벤트만을
					처리할 수 있다는 단점이 있다. 

					반면, CallBack 방식의 경우 SleepEx 함수를 호출하는 순간
					APC Queue에 저장되어 있는 콜백 함수들을 모두 호출하는
					식으로 동작하기 때문에 클라이언트 개수만큼 이벤트를
					할당할 필요가 없다는 장점이 있다 */
					SleepEx(INFINITE, TRUE);
				}
				else
				{
					// TODO : 문제 상황
					break;
				}
			}
			else
			{
				cout << "Data Recv Len = " << recvLen << '\n';
			}
		}
		
		closesocket(session.socket);
	}

	WSACleanup();

	return 0;
}
