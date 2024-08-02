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


/* TODO 현재 테스트 코드는 문제점이 하나 있는데, 뭔지 
기억이 안나면 Completion Port 강의 30:50 부터 보셈 */
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
};

struct OverlappedEx
{
	WSAOVERLAPPED overlapped = {};
	int32 iType = 0; // read, write, accept, connect... IO_TYPE을 의미
};

enum IO_TYPE
{
	READ,
	WRITE,
	ACCEPT,
	CONNECT,
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

void WorkerThreadMain(HANDLE iocpHandle)
{
	/* CP(Completion Port)핸들을 받아와서 GetQueuedCompletionStatus 함수를
	호출하여 완료된 입출력 작업이 있는지를 확인할 것이다 */

	while (true)
	{
		/* 완료된 작업이 있는지 계속해서 확인하는 동작이 성능에 영향을 주지는 않으며
		함수 인자값으로 대기 시간을 넘겨줄 수 있는데 대기 시간을 무한으로 넘겨주면
		이전에 사용했던 이벤트 방식처럼 일감이 발생하기 전 까지 대기 상태가 되어서
		동작하지 않고 있다가 실제로 일감이 발생할 경우 운영체제가 쓰레드 하나를 
		활성화하여 일처리를 명령하게 될 것이다(설명이 모호함) */
		
		/* GetQueuedCompletionStatus 함수의 인자값 정보는 아래와 같다
		1. IOCP 핸들 
		2. 완료된 I/O 작업의 바이트 수를 저장할 변수
		3. Completion Key(완료된 입출력 작업을 구분하기 위한 키값)
		4. 완료된 I/O 작업의 대한 OVERLAPPED 구조체 포인터 변수
		5. 대기 시간 
		
		간략하게 정리하자면 해당 인자값들을 통해서 세팅된 Session, 
		Overlapped 정보를 복원할 수 있다는 것이다 */

		DWORD bytesTransferred = { 0 };
		Session* session = { nullptr };
		OverlappedEx* overlappedEx = { nullptr };

		BOOL bRet = GetQueuedCompletionStatus(iocpHandle, &bytesTransferred, (ULONG_PTR*)&session, 
											 (LPOVERLAPPED*)&overlappedEx, INFINITE);

		/* GetQueuedCompletionStatus 함수에 대기 시간을 INFINITE(무한)을 넘겨서
		실제로 일감(입출력 작업)이 완료가 될 때까지 쓰레드가 대기를 할 것인데
		이후 실질적으로 완료된 일감(Recv, Send 등)이 발생할 경우 커널 모드에서 
		해당 완료 패킷을 IOCP(Completion Port)에 추가할 것이고, IOCP에 완료
		패킷이 추가되면 GetQueuedCompletionStatus 함수는 대기 상태에서 빠져나와
		완료된 입출력 작업에 대한 정보를 반환하게 될 것이다. 이 과정에서 IOCP는
		대기 중인 쓰레드 중 하나를 활성화하여 완료된 작업을 처리하게 될 것이다 
		GetQueuedCompletionStatus 함수의 반환 타입 자체는 BOOL 타입으로 연결이
		끊겼다거나 하는 문제 상황에 대한 여부를 나타낸다 */
		
		if (bRet == FALSE || bytesTransferred == 0)
		{
			// TODO : 연결 끊김 
			// 우선 간략하게 처리
			continue;
		}

		// 테스트 용도로 READ 작업만을 적용하고 있기 때문에 READ 에러를 체크
		ASSERT_CRASH(overlappedEx->iType == IO_TYPE::READ);

		cout << "Recv Data IOCP = " << bytesTransferred << '\n';

		/* 또한, 이런 식으로 쓰레드를 활성화해서 일을 처리한 다음 이어서 데이터를
		또 받아주고 싶은 경우 이전에 했던 것과 마찬가지로 콜백 함수에서도 Recv를
		해줘야 한다. 왜냐하면 단순하게 낚시로 비유를 하자면 낚싯대를 물에 던져놓고
		물고기를 낚은 상황에서 또다시 물고기를 낚고 싶다면 낚싯대를 다시 물에 
		던져놓아야 하는 상황과 유사하다고 생각하면 된다.
		아래 코드에서는 받아온 데이터를 기반으로 Recv를 해주고 있는데 Recv가 아닌
		Send와 같은 다른 작업을 하고 싶다고 한다면 OVERLAPPED 구조체를 동적할당해서
		타입을 변경해서 함수를 호출하면 될 것이다 */

		WSABUF wasBuf;
		wasBuf.buf = session->recvBuffer;
		wasBuf.len = BUFSIZE;

		DWORD recvLen = 0;
		DWORD flag = 0;

		WSARecv(session->socket, &wasBuf, 1, &recvLen, &flag, &overlappedEx->overlapped,
			NULL);

		/* 그래서 결국 Session, OverlappedEx를 분리해서 관리한 이유는 나중에 가면 네트워크
		입출력 함수를 사용할 때 어떤 사유(Read, Writre, 기타 등등)로 사용했는지를 구분하기
		위해서 분리를 해 준 것이다 */
	}
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

	
	/* IOCP (Completion Routine)모델
	Overlapped 모델과는 다르게 APC Queue를 통해서 일감을 처리하는 것이 아니라 
	Completion Port를 통해서 처리하게 된다.
	Completion Port는 쓰레드마다 존재하는 것은 아니고 만들어서 사용하면 되는데
	다수의 Completion Port를 만들 수도 있지만 기본적으로는 1개를 만들어서 사용한다.
	
	Completion Port의 개념을 대략적으로 작성해보자면 중앙에서 관리하는 APC Queue같은 
	느낌인데, 다수의 쓰레드가 하나의 Completion Port를 통해 일감(입출력 비동기 작업)을
	받아서 실행하게 될 것이다. 즉, 일감 자체를 모아놓는 공용 Queue를 만든다는 개념이다.

	Overlapped 모델을 사용해서 일감을 처리하는 방식은 작업이 완료가 되었다면 
	Alertable Wait 상태로 전환 후에 APC Queue에 접근하여 저장되어 있는 모든
	콜백 함수를 하나씩 호출하는 방식으로 처리했었다.
	
	Completion Port의 경우에는 결과 처리를 하기 위해서 GetQueuedCompletionStatus라는
	함수를 호출하게 된다.

	또한, IOCP 방식은 Completion Port 하나를 통해서 일감을 처리하기 때문에 
	멀티쓰레드 환경과 친화적이라는 특징이 있다. 

	위와 같은 특징들을 제외하면 IOCP는 Overlapped 모델과 유사한 코드 흐름을 가진다.

	CreateIoCompletionPort 함수는 Completion Port 생성, 이후에 소켓을 생성해준 
	Completion Port에 등록하는 두 가지 용도로 사용한다.

	GetQueuedCompletionStatus(결과 처리를 감시하는 함수) */

	// 임시로 만들어준 Session을 보관하는 vector
	vector<Session*> vecSessionManager;

	// Completion Port 생성
	/* CreateIoCompletionPort 함수를 Completion Port를 생성하는 용도로 사용할 경우
	첫 번째 인자값으로 INVALID_HANDLE_VALUE를 넘겨주고 나머지 인자값은 모두 NULL
	로 채워주면 되며, 함수 반환값으로 IOCP핸들을 뱉어준다 */
	// 이렇게 생성한 IOCP 핸들을 이용해서 Completion Port에 접근할 수 있다(커널 객체?)
	// 참고로, 해당 코드들은 테스트를 하기 위해서 대충 만든 것들이 있다는 점을 인지하자
	HANDLE IocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, NULL);

	/* Completion Port를 생성한 WorkerThreadMain 쓰레드를 생성해서 Completion Port를 
	계속 관찰하면서 완료된 입출력 함수가 존재하는지 확인해서 완료된 입출력 함수가 있다면 
	완료된 결과물을 받아와서 해당 결과값을 기반으로 어떤 비동기 입출력 작업이 완료가
	되었는지 확인 후 해당 작업에 대한 추가 처리(일반적으로 콜백 함수)를 호출해줄 것이다.
	여기서 말하는 입출력 함수는 WSARecv WSASend와 같은 비동기 함수를 말하는거임 */
	for (int32 i = 0; i < 5; ++i)
	{
		// 람다를 이용해서 복사한 IocpHandle를 이용해서 WorkerThreadMain 함수 호출 유도
		GThreadManager->Launch([=]() { WorkerThreadMain(IocpHandle); });
	}

	/* Main Thread = Accept 담당(참고로 나중에는 Accept 하는 것도 Completion Port를 통해서 
	처리할 것인데 우선은 테스트를 위해서 임시로 Main Thread에서 처리하도록 만든거임

	아래 Main Thread가 하는 역할을 간략하게 쓰자면 새로운 소켓을 받아 와서 한 번 Recv 함수를 
	호출하고 작업 완료 여부는 확인하지 않고 다른 클라이언트을 받아주러 가는 식으로 동작한다.
	Recv 작업 처리는 별도의 쓰레드가 처리해주게 될 것이다 */
	while (true)
	{
		SOCKADDR_IN ClientAddr;
		int32 iAddrLen = sizeof(ClientAddr);

		SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &iAddrLen);
		if (ClientSocket == INVALID_SOCKET)
			return 0;

		// 다수의 소켓과 연결하는 상황을 가정
		Session* session = new Session();
		session->socket = ClientSocket;
		vecSessionManager.push_back(session);
			
		cout << "Client Connected !" << '\n';

		/* 소켓을 CP(Completion Port)에 등록
		소켓을 CP에 등록하여 관찰을 하게 되면 해당 소켓이 Recv, Send 등의 작업이 완료가 되었을
		경우 완료가 되었다는 통지가 IOCP를 통해서 오게 될 것이다. 물론 자동으로 오는 것은 아니고
		직접 관찰을 해서 완료가 되었는지에 대한 여부를 체크해야 한다
		CreateIoCompletionPort 함수는 위에서 설명했듯이 CP를 생성하는 용도와 소켓을 CP에 등록하는 
		두 가지 용도로 사용할 수 있으며, 아래 로직은 소켓을 CP에 등록하는 용도로 사용하는 경우이다 */

		/* 소켓을 CP에 등록하는 용도로 사용하는 경우 첫 번째 인자값으로 HANDLE을 받아주는데
		소켓을 캐스팅해서 넘겨주면되고, 두 번째 인자는 위에서 만들어준 CP을 넘겨주면 된다.
		세 번째 인자값은 나중에 GetQueuedCompletionStatus 함수를 호출해서 일감(입출력 작업)을		
		가지고 올 때 어떤 일감인지를 구별하기 위한 키값을 의미하기 때문에 아무값이나 넘겨주면
		된다. 마지막 네 번째 인자값은 활용할 최대 쓰레드의 개수를 의미하는데 그냥 0을 넘겨주면
		최대 코어 개수만큼 할당이 되기 때문에 0을 넘겨줘도 상관없다. 참고로 해당 함수가 자동으로
		쓰레드를 생성해 주는 것은 아니고 직접 쓰레드를 생성해서 나중에 GetQueuedCompletionStatus
		함수를 통해 Completion Port를 직접 체크하는 방식으로 동작을 하게 될 것이다(설명이 모호함)
		코드를 보면 생각보다 단순하다 */
		CreateIoCompletionPort((HANDLE)ClientSocket, IocpHandle, (ULONG_PTR)session, 0);
		 	
		WSABUF wasBuf;
		wasBuf.buf = session->recvBuffer;
		wasBuf.len = BUFSIZE;

		OverlappedEx* overlappedEx = new OverlappedEx();
		overlappedEx->iType = IO_TYPE::READ;

		DWORD recvLen = 0;
		DWORD flag = 0;
 
		WSARecv(ClientSocket, &wasBuf, 1, &recvLen, &flag, &overlappedEx->overlapped, 
				NULL);

	}

	GThreadManager->Join();
	
	WSACleanup();

	return 0;
}
