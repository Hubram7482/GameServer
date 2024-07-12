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

	/* Select 모델 = (Select 함수가 핵심이 되는 모델)
	소켓 함수 호출이 성공할 시점을 미리 알 수 있다는 개념의 소켓 모델이다.
	블로킹, 논블로킹 방식에 상관없이 둘 다 응용할 수 있으며, 예를 들어서
	블로킹 소켓을 대상으로 Select 모델을 적용할 경우 조건이 만족되지 않아서
	대기 상태가 되는 상황을 방지할 수 있을 것이고 논블로킹 소켓을 대상으로는
	조건이 만족되지 않아서 불필요하게 반복해서 조건이 성립하는지 확인하는 
	상황을 방지할 수 있을 것이다. 즉 송수신을 하기 전에 송수신이 가능한 
	상태인지를 우선 확인한다는 것이라고 생각하면 된다.
	
	위에서 설명했듯 지금까지 데이터를 송수신 하는 과정에서 발생했던 가장 
	대표적인 문제점이 RecvBuffer에 데이터가 없는데, 데이터를 읽어들이거나
	SendBuffer가 가득 차있는데 데이터를 쓰거나 하는 상황에서 블록킹 방식이라면
	대기를 하게 될 것이고 논블로킹 방식이라면 통과는 되지만 이후에 추가적으로 
	데이터가 제대로 송수신 되었는지 반복해서 확인하는 과정에서 또 다시 대기를
	해야 하는 문제점이 있었으며 이를 방지할 수 있는 방법이라고 생각하면 된다

	Select 모델을 사용하기 위해서는 Socket Set을 만들어줘야 한다.

	Socket Set(사용 방법)
	1) 읽기[ ], 쓰기[ ], 예외(OOB)[ ] 이러한 목록에 관찰 대상을 등록해서 
	확인하는 느낌으로 작동하는데, 예를 들어서 특정 소켓의 읽기, 쓰기, 예외 
	각 시점의성공 시점을 확인하고 싶은 경우 병렬형태로 원하는 관찰 대상에
	등록해서 사용한다. 또한, 예의는 무엇을 의미하냐면 OOB(OutOfBound)는
	send()함수를 사용할 때 마지막 인자값을 따로 설정해준 경우가 없었는데
	상황에 따라서 해당 인자값을 MSG_OOB로 설정할 수 있다. MSG_OOB로 
	설정할 경우 특별한 데이터로 취급되며 데이터를 받는 곳에서도 똑같이
	Recv(수신)을 해야지만 해당 데이터를 받을 수 있게 된다. 간략하게 
	말하자면 긴급 상황을 알리는 특정 상황에서 사용되는데 사용 빈도는 적기
	때문에 읽기, 쓰기 두 가지 정보를 채서 관찰 대상을 등록한다고 생각하자.
	
	2) select(readSet, writeSet, exceptSet) 함수를 사용해서 관찰을 시작한다.
	select 함수의 각 인자값을 모두 넘겨줄 필요는 없고, 사용할 관찰 대상에
	대해서만 인자값을 넘겨주고 나머지는 nullptr을 넘겨주면 된다.

	3) 해당 select 함수를 호출하면 관찰을 시작하며 관찰 대상으로 등록한 
	소켓 중 최소 하나라도 준비가 되었다고 한다면 준비된 소켓의 개수를 반환하고,
	준비가 되지 않은 나머지 소켓은 관찰 대상에서 제거해주게 된다. 왜 이렇게
	동작하냐면 예를 들어서,	A, B, C 라는 세 가지의 소켓을 대상으로 Read를 하고 
	싶다고 가정했을때 읽기[A, B, C] 이 중 B소켓에 데이터가 송신되어서 B소켓만이
	준비가 되었다면 select함수가 반환을 하면서 준비된 소켓의 개수(1)을 반환하고
	나머지 A, B를 제거하는 방식으로 동작하게 된다.
	
	4) 결과적으로 읽기[B]가 남아서 실제로 Read를 진행할 수 있는 소켓만이 남고
	해당 소켓들을 읽어주는 동작을 진행하게 된다 */

#pragma region Select 모델
	/* Socket Set을 다룰 때는 fd_set 데이터 타입을 사용해야 한다
	또한 읽기[ ], 쓰기[ ], 예외[ ]와 같은 관찰 대상 항목에 소켓을
	등록하기 위한 Socket Set을 만들어줘야 하는데, 이때 사용할 수
	있는 4가지의 Flag 타입들이 존재한다.

	fd_set set;
	FD_ZERO  : 비운다		  ex) FD_ZERO(&set);
	FD_SET   : 소켓(s) 추가	  ex) FD_SET(s, &set);
	FD_CLR	 : 소켓(s) 제거	  ex) FD_CLR(s, &set);
	FD_ISSET : 소켓(s)가 set에 들어있다면 0이 아닌 값을 반환한다. */

#pragma endregion

	
	WSACleanup();

	return 0;
}
