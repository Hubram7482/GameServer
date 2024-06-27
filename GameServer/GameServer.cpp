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

	/* 소켓이 생성되는 순간	커널 버퍼에 만들어지는 SendBuffer, RecvBuffer의 
	크기를 변경하거나 다양한 옵션을 설정할 수 있는데 setsocketopt, getsockopt 
	해당 두 함수를 통해서 설정이 가능하며, 함수의 인자값은 아래와 같다.
	첫 번째 인자 = 설정할 소켓

	두 번째 인자 = 레벨(옵션을 해석하고 처리할 주체를 의미하는데 예를 들어서
	소켓이 처리하도록 설정할 경우 SOL_SOCKET을 입력하면 되고, 그게 아니라 
	IPv4단계에서 처리하도록 설정할 경우에는 IPPROTO_IP를 입력하면 되고, 
	TCP 프로토콜 단계일 경우에는 IPPROTO_TCP 이런 식으로 옵션을 입력한다)

	세 번째 인자 = 설정해준 레벨을 통해 적용할 옵션(SO_KEEPALIVE, SO_LINGER, 
	SO_SNDBUF(송신 버퍼 크기), SO_RCVBUF(수신 버퍼 크기), SO_REUSEADDR, 기타 등등)
	
	SO_KEEPALIVE = 주기적으로 연결 상태를 확인할지에 대한 여부(TCP 전용)
	SO_KEEPALIVE가 필요한 이유는 통신 중인 상대방이 아무런 동작을 하지 않는
	경우가 발생할 수 있는데, 이게 정말로 송수신할 데이터가 없어서 그런 것인지
	연결이 끊어져서 그런 것인지를 알 수가 없기 때문에 주기적으로 TCP프로토콜
	단계에서 연결 상태를 계속 확인하는 것이다.

	SO_LINGER = 지연하다는 뜻인데, 송신 버퍼에 있는 데이터를 보낼 것인지 
	그게 아니면 데이터를 날려버릴지를 설정하는 옵션이며, 이게 무슨 의미냐면
	예를 들어서 Send함수를 호출했다가 바로 이어서 closesocket함수를 사용했을
	경우 send함수를 통해서 이미 커널 버퍼에 SendBuffer에 대한 정보가 저장이
	되어 있을 것인데 해당 데이터를 어떻게 처리할지에 대한 옵션을 의미한다
	SO_LINGER의 내부를 살펴보면 u_short타입의 데이터 두 개를 받아주고 있는데
	각각 onoff, linger라는 이름으로 되어 있으며 onoff의 경우에는 0이면 
	closesocket를 바로 반환하고 1로 설정이 되어 있으면 linger초만큼 대기를
	하게 된다(default는 0) 따라서 linger는 대기 시간을 의미한다

	SO_SNDBUF = 송신 버퍼 크기
	SO_RCVBUF = 수신 버퍼 크기
	위 두개의 옵션은 둘 다 int타입의 데이터를 받아주며, 이름 그대로 버퍼의
	크기를 알고 싶을때 사용하면 된다

	SO_REUSEADDR = 이름 그대로 IP주소 및 포트를 재사용한다는 의미이다
	해당 옵션이 왜 필요한지 간략하게 정리하자면 TCP에서 소켓을 만든 다음
	주소를 만들어서 바인드를 통해 해당 주소에 특정 IP주소와 포트 번호를 
	묶어 줬는데 경우에 따라서 해당 주소가 이미 사용중이거나 서버를 강제종료
	하고 다시 켜거나 하는 상황이라서 정리가 충분히 되지 않은 상황이 있을 수
	있는데, 이런 상황일 경우 바인딩이 실패해서 대략 3분 정도를 기다리지 않으면
	서버를 띄울 수가 없게 된다. 이러한 문제를 방지하고자 강제로 해당 주소를
	재사용하는 방법이 있는데 이게 바로 SO_REUSEADDR 옵션이다(세팅 하는게 좋음)

	네 번째 인자 = 각 옵션 마다 필요한 타입
	다섯 번째 인자 = 옵션 타입의 크기
	여기서 네 번째, 다섯 번째 인자값은 추가 설명이 전혀 없고 그냥 넣어주면
	된다고만 말하는데, 아마도 뭔가 처리 여부를 확인하는게 아닐까 싶음 */

	// SO_KEEPALIVE 예시
	// bool bEnable = true;
	// setsockopt(ServerSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&bEnable, sizeof(bEnable));

	// SO_LINGER 예시
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 5;
	setsockopt(ServerSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));

	/* 참고로 closesocket함수를 바로 호출하면 안되고 연결중인 상대방에게 
	연결을 끊겠다고 미리 알려주고 호출을 해야하며 shutdown함수를 통해서
	해당 작업을 처리하며 함수 인자값은(소켓, 셧다운 방식)이다. 
	셧다운 방식은 SD_SEND(Send를 막는다), SD_RECEIVE(recv를 막는다),
	SD_BOTH(둘 다 막는다)등이 있다(사실은 shutdown을 쓰지 않아도 
	문제는 거의 발생하지 않는다고 한다 */
	//shutdown(ServerSocket, SD_SEND);
	
	int32 iSendBufferSize;
	int32 iSendOptionLen = sizeof(iSendBufferSize);
	// 함수 인자값을 왜 이런 형태로 받아주는지 모른다고함(MS에서 지정한대로 쓰는듯)
	getsockopt(ServerSocket, SOL_SOCKET, SO_SNDBUF, (char*)&iSendBufferSize, &iSendOptionLen);
	cout << "송신 버퍼 크기 : " << iSendBufferSize << '\n';

	int32 iRecvBufferSize;
	int32 iRecvOptionLen = sizeof(iRecvBufferSize);
	getsockopt(ServerSocket, SOL_SOCKET, SO_RCVBUF, (char*)&iRecvBufferSize, &iRecvOptionLen);
	cout << "수신 버퍼 크기 : " << iRecvBufferSize << '\n';

	// SO_REUSEADDR(IP 주소 및 포트 번호 재사용)
	{
		// 해당 옵션에 대한 설명은 위에 작성해놨음
		bool bEnable = true;
		setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&bEnable, sizeof(bEnable));
	}


	// IPPROTO_TCP
	// TCP_NODELAY = Nagle 알고리즘 작동 여부
	/* Nagle 알고리즘을 간략하게 설명하자면 데이터가 충분히 크면 전송하고, 
	그렇지 않다면 데이터가 충분히 쌓일때까지 대기한다는 개념이며, 최대한 
	데이터를 쌓아서 전송함으로써 회선 낭비를 줄이고 효율성을 높이는 방식 
	장점은 작은 패킷이 불필요하게 많이 생성되는 것을 방지한다는 점이지만
	단점으로는 반응 시간의 손해를 보게 된다는 특징이 있다(대기 상황)
	그래서 일반적으로 게임에서는 해당 Nagle을 항상 꺼주는게 기본 상태이고
	정말 필요할 경우에만 켜줘야 한다 */
	{
		/* Nagle 알고리즘을 사용하지 않는다면 작은 패킷들이 너무 많아지지 
		않을까 싶지만, 이걸 방지하기 위해 데이터를 쌓아서 전송하는 작업은 
		유저 레벨에서도 충분히 가능하며 이렇게 관리하는게 더 효율적이다
		그래서 게임에서는 Nagle 알고리즘을 최대한 사용하지 않도록 
		TCP_NODELAY를 true로 설정하는게 일반적인 상황이라고 볼 수 있다 */
		bool bEnable = true;
		setsockopt(ServerSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&bEnable, sizeof(bEnable));
	}

	closesocket(ServerSocket);

	WSACleanup();

	return 0;
}
