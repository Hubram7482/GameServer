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

#pragma region Select 모델

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

	/* Socket Set을 다룰 때는 fd_set 데이터 타입을 사용해야 한다
	또한 읽기[ ], 쓰기[ ], 예외[ ]와 같은 관찰 대상 항목에 소켓을
	등록하기 위한 Socket Set을 만들어줘야 하는데, 이때 사용할 수
	있는 4가지의 Flag 타입들이 존재한다.

	fd_set set;
	FD_ZERO  : 비운다		  ex) FD_ZERO(&set);
	FD_SET   : 소켓(s) 추가	  ex) FD_SET(s, &set);
	FD_CLR	 : 소켓(s) 제거	  ex) FD_CLR(s, &set);
	FD_ISSET : 소켓(s)가 set에 들어있다면 0이 아닌 값을 반환한다.
	
	Select 모델의 장점은 구현하기가 간단하고 낭비되는 부분이 
	없어진다는 것이고 단점으로는 fd_set에 session을 등록할 수
	있는 최대 크기가 생각보다 작다는 것이다(FD_SETSIZE = 64)
	따라서 640명을 등록해야 할 경우 fd_set을 10개를 만들어서
	아래 코드를 10번 반복해야 한다는 것이다 */
	
#pragma endregion

	vector<Session> vecSessions;
	vecSessions.reserve(100);

	fd_set reads;
	fd_set writes;

	while (true)
	{
		/* Socket Set 초기화를 루프마다 해줘야하는데 왜냐하면 당연하게도
		select 함수를 호출하고 나면 준비가 되지 않은 대상은 제거가 되어서
		reads, writes의 데이터가 일부분 날아갔을 수도 있기 때문이다 */
		FD_ZERO(&reads);
		FD_ZERO(&writes);

		// ListenSocket 등록
		FD_SET(ListenSocket, &reads);

		// 소켓 등록
		for (Session& iter : vecSessions)
		{
			/* 클라로부터 받은 데이터를 다시 클라에게 전송할 
			것이다. 따라서 우선 데이터를 받았는지 확인해야 한다.

			클라의 데이터를 받았다면 어느 정도의 데이터를 받았는지
			iter.iRecvBytes에 설정할 것이고 이 점을 활용해서 
			iSendBytes(클라에게 다시 보낼 데이터의 크기)가 
			iRecvBytes(클라에게서 받은 데이터의 크기)보다 
			크거나 같은지를 확인해서 소켓을 등록한다. */
			if (iter.iRecvBytes <= iter.iSendBytes)
			{
				FD_SET(iter.socket, &reads);
			}
			else
			{
				FD_SET(iter.socket, &writes);
			}
		}

		/* 우선 읽기, 쓰기만 사용할 것이기 때문에 4번째 인자값은 nullptr.
		5번째 인자값은 TimeOut을 설정할 수 있는 인자값인데, 이게 무슨
		얘기냐면 예를 들어서 마지막 인자값을 설정해주지 않을 경우에는
		select함수가 하나라도 준비된 소켓이 존재할 때까지 무한 대기를
		하게 될 것인데 다섯 번째 인자값을 넘겨주면 이를 방지할 수 있다.
		
		timeval TimeOut; 이러한 구조체를 인자값으로 넘겨주면 되며, 해당
		구조체는 내부적으로 tv_sec, tv_usec 이러한 long타입의 변수 두 
		개를 가지고 있다. 각각 seconds(초), microseconds를 의미한다.
		이를 통해서 얼마나 기다릴 것인지를 설정하는 방식으로 사용한다.

		첫 번째 인자값은 넣지 않아도된다(Linux와의 통일성을 위한 인자) */
		
		int32 iRetVal = select(0, &reads, &writes, nullptr, nullptr);
		if (iRetVal == SOCKET_ERROR)
		{
			HandleError("SelectFunction");
			break;
		}

		/* select함수가 성공적으로 호출이 되었다고 한다면 select함수는
		조건에 적합하지 않은 대상은 모두 제거해주기 때문에 read, write
		제거되지 않았다면 준비가 된 대상이 남아있다는 의미이며 따라서,
		어느 대상(read, write)이 준비가 되었는지를 확인해야한다 */

		/* FD_ISSET을 사용해서 ListenSocket이 reads에 들어가있는지를
		반환값을 통해서 판별하며 반환값이 true라면 ListenSocket이
		accept(수신)준비가 완료되었다는 뜻이니까 특정 클라이언트가
		connect(연결)요청을 했다는 상황이라는 것을 알 수 있다 */

		// Listener 소켓 확인
		if (FD_ISSET(ListenSocket, &reads))
		{
			SOCKADDR_IN ClientAddr;
			int32 iAddLen = sizeof(ClientAddr);

			SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &iAddLen);
			/* Select모델을 사용하지 않았더라면 받아온 ClientSocket이
			유효한지에 대해서 확인이 필요했는데, 지금은 Select모델을
			사용해서 FD_ISSET을 통해 확인을 했기 때문에 추가적으로
			유효한지에 대한 검사를 하지 않아도 된다 */

			if (ClientSocket != INVALID_SOCKET)
			{
				// 그래도 혹시 모르니까 소켓이 유효할때 진행하도록 했음
				cout << "Client Connected" << '\n';

				/* 클라의 소켓을 정상적으로 받아왔다면 해당 소켓을 
				vecSessions에 추가를 해 줄 것이다 */
				vecSessions.push_back(Session{ClientSocket});
			}
		}

		/* 위 방법을 통해서 vecSessions에게 Listener소켓을 추가했을 경우
		다음 루프에서 해당 Listener소켓 또한 reads(Read set)에 들어가
		있을 것이기에 나머지 소켓도 read 준비가 되었는지 검사를한다 */
		for (Session& iter : vecSessions)
		{
			// Read 확인
			if (FD_ISSET(iter.socket, &reads))
			{
				// Read 준비가 되었을 경우 Recv(수신)을 해준다
				int32 iRecvLen = recv(iter.socket, iter.recvBuffer, BUFSIZE, 0);
				if (iRecvLen <= 0)
				{
					/* TODO 만약 iRecvLen이 0이하의 값이라면 받을 
					데이터가 없다는 뜻이니까 vecSessions에서 제거 */
					continue;
				}

				/* 정상적으로 데이터를 받았다면 RecvBytes에 iRecvLen을 저장하는데
				이렇게 할 경우 다음 루프부터 Send할 데이터가 있다는 의미가 된다 */
				iter.iRecvBytes = iRecvLen;
			}

			// Write 확인
			if (FD_ISSET(iter.socket, &writes))
			{
				/* 송신 커널 버퍼에 비어 있는 공간이 충분히 있다는 상황이기 
				때문에 해당 공간에 데이터를 복사할 준비가 되었다는 뜻이다 */

				/* send 함수의 반환값이 send한 크기를 의미하는데 만약에 기존 
				방식대로 블로킹 모드를 사용했을 경우에는 모든 데이터를 보내야
				하지만 논블로킹 모드를 사용하면 데이터를 받을 상대방의 버퍼
				상황에 따라서 데이터의 일부만을 보낼 수가 있다.
				참고로, 논블로킹 모드에서 데이터의 일부를 보내는 경우는 
				드물지만 send(송신)하려는 데이터의 크기가 보내려던 데이터의
				크기보다 작을 수 있다는 것을 알아야한다 이 때문에 아래 send
				함수를 호출할 때 인자값으로 RecvBytes - SendBytes를 넘겨준
				것이다. 좀 더 자세히 말하자면 예를 들어서 10바이트 만큼의
				데이터를 받았으나 이전 루프에서 6바이트 만큼의 데이터를 
				상대방에게 보냈다고 한다면 남은 4바이트 만큼의 데이터를 
				보내야하기 때문에 이런 방식으로 인자값을 넘겨준 것이다 */
				int32 iSendLen = send(iter.socket, &iter.recvBuffer[iter.iSendBytes],
									  iter.iRecvBytes - iter.iSendBytes, 0);

				if (iSendLen <= 0)
				{
					// TODO Lintener와 마찬가지로 vecSessions에서 제거
					continue;
				}

				/*
				iSendLen의 크기가 0보다 크다면 성공적으로 데이터를 모두
				보냈거나 혹은 일부라도 보냈다는 뜻이기에 send(송신)한 
				만큼의 데이터 크기를 SendBytes에 누적시킨다 */
				iter.iSendBytes += iSendLen;

				/* SendBytes의 크기와 RecvBytes의 크기가 동일하다면 모든 
				데이터를 보냈다는 뜻이니까 SendBytes, RecvBytes의 값을
				0으로 갱신해주며 이렇게 할 경우 다음 루프부터는 다시
				Recv를 할 상태가 준비 되었다는 의미가된다 */
				if (iter.iSendBytes == iter.iRecvBytes)
				{
					iter.iSendBytes = 0;
					iter.iRecvBytes = 0;
				}
			}
		}
	}

	WSACleanup();

	return 0;
}
