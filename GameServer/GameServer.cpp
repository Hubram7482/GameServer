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

	/* 서버의 경우에는 비유하자면 상담을 받아주는 안내원을 
	고용해서 회사의 대표 번호가 적혀있는 휴대폰을 주는 부분
	주소 체계(IP주소), 사용 타입, 프로토콜 총 3개의 인자값을
	받아주는데, 세 번째 인자값은 세팅하지 않아도된다

	주소체계 : Address Family (AF_INET = IPv4, AF_INET6 = IPv6)
	사용타입 : TCP(SOCK_STREAM) vs UDP(SOCK_DGRAM)
	프로토콜 : 0(알아서 지정해달라는 의미)
	참고로 프로토콜은 컴퓨터 내부에서, 또는 컴퓨터 사이에서
	데이터의 교환 방식을 정의하는 규칙 체계를 말한다

	socket함수의 반환값은 SOCKET이라는 타입의 값을 반환하는데
	내부적으로 단순 uint에 불과하다. 그러나 해당 정수값을
	이용하여 나중에 운영체제한테 특정 소켓번호에게 데이터를
	보내고 싶다고 요청하면 운영체제가 알아서 받은 번호에
	해당하는 소켓 리소스를 이용할 수 있도록 해주는 개념이다
	간략하게 SCOKET라는 정수가 사실상 포인터처럼 실제
	리소스를 가르키고 있는 상태라는 느낌으로 이해하면 된다 */
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	// 실패했는지 확인
	if (ListenSocket == INVALID_SOCKET)
	{
		/* 실패한 경우 어떠한 이유로 실패했는지를 알려주는
		에러 메세지를 받아올 수 있다 */
		int32 iErrCode = WSAGetLastError();
		cout << "Socket Error : " << iErrCode << '\n';
		return 0;
	}

	// 서버의 경우에는 비유하자면 전화를 받아줄 회사의
	// 주소를 지정하는 부분(IP주소 + Port)
	SOCKADDR_IN ServerAddR;
	memset(&ServerAddR, false, sizeof(ServerAddR));
	ServerAddR.sin_family = AF_INET;
	// 자동으로 지정해달라는 의미이며, 주소가 여러개 있을 수 있기 때문에 사용
	ServerAddR.sin_addr.s_addr = htonl(INADDR_ANY);

	ServerAddR.sin_port = htons(7777);

	// 비유하자면 안내원에게 건내준 휴대폰 개통(식당의 대표 번호)
	// ListenSocket라는 소켓과 ServerAddR(주소)를 연동한다
	if (::bind(ListenSocket, (SOCKADDR*)&ServerAddR, sizeof(ServerAddR)) == SOCKET_ERROR)
	{
		int32 iErrCode = WSAGetLastError();
		cout << "Bind ErrorCode" << iErrCode << '\n';
		return 0;
	}

	// 영업 시작
	/* listen함수를 사용하게 되는데, 첫 번째 인자값은 소켓이고 두 번째 인자값은
	BackLog라는 정수를 받아주는데 BackLog가 무엇이냐면 예를 들어 식당에 손님은
	많은데 자리가 없다면 대기열이 생길 것이고 재료는 한정되어 있기 때문에 대기열
	인원에 제한이 있을 것이다. BackLog는 이러한 역할을 한다(최대 한도 느낌)	*/
	if (listen(ListenSocket, 10) == SOCKET_ERROR)
	{
		int32 iErrCode = WSAGetLastError();
		cout << "Listen ErrorCode" << iErrCode << '\n';
		return 0;
	}

	// 영업 시작한 상태

	/* 영업을 시작했다는 의미는 누군가가(클라 등) Connect를 
	요청했을 경우 이러한 요청을 처리해줄 준비가 되었다는 뜻이다 */
	while (true)
	{
		/* 무한 루프를 돌면서 송신 요청을 받아서
		처리하기 위해서 accept함수를 사용한다 */
		SOCKADDR_IN ClientAddR;
		memset(&ClientAddR, false, sizeof(ClientAddR));
		/* accept함수의 첫 번째로는 Listen 소켓을 받아주고 두 번째 
		인자값으로는 Listen소켓에 연결한 클라이언트 주소를 받아준다.
		예를 들어서 MMO에서 이러한 ClientAddR를 관리하면서 패킷을 너무
		이상하게 보낸다던가 비정상적인 동작을 하는 경우 해당 
		ClientAddR을 이용해서 IP밴을 하거나 할 수 있을 것이다.
		마지막으로 세 번째 인값으로는 두 번째 인자값으로 받아준
		주소의 크기의 주소를 받아주고 있다 */
		int32 iAddRLen = sizeof(ClientAddR);

		/* 참고로 accept함수의 두 번째 인자와 세 번째 인자값은 필수가
		아니라 선택사항이며, 주소가 궁금해서 추출하고 싶은 경우 소켓을
		기입해주면 되고 그게 아니라면 NULL로 채워줘도 된다.
		중요한건 accept의 반환값인데 SOCKET타입을 반환해주며
		해당 SOCKET값을 통해서 연결된 클라이언트와 통신을 할 수 있다.
		정리하자면 Listen소켓은 그저 안내원의 역할(통신을 받아주는)로써만
		사용하는 소켓이고, accept의 반환값인 SOCKET이 클라이언트와
		연결되어서 패킷을 주고 받을 수 있는 그런 역할이다 */
		SOCKET ClientSocket = ::accept(ListenSocket, (SOCKADDR*)&ClientAddR, &iAddRLen);
		
		if (ClientSocket == INVALID_SOCKET)
		{
			int32 iErrCode = WSAGetLastError();
			cout << "Accept ErrorCode" << iErrCode << '\n';
			return 0;
		}

		/* 손님(연결된 클라이언트) 입장
		아래 코드는 클라이언트의 IP주소를 출력하는 로직인데 
		루키스도 왜 이런 형태로 되어있는지 모른다고 함 */
		char IPAddRes[16];
		inet_ntop(AF_INET, &ClientAddR.sin_addr, IPAddRes, sizeof(IPAddRes));
		cout << "Client Connected IP = " << IPAddRes << '\n';

		// TODO
		while (true)
		{
			char RecvBuffer[1000];

			this_thread::sleep_for(1s);

			/* recv함수의 반환값은 수신받은 데이터의 Byte크기를 의미한다
			따라서, 받아온 데이터 크기가 음수라면 문제가 발생한 것 */
			int32 iRecvLen = recv(ClientSocket, RecvBuffer, sizeof(RecvBuffer), 0);
			if (iRecvLen <= 0)
			{
				int32 iErrCode = WSAGetLastError();
				cout << "Recv ErrorCode" << iErrCode << '\n';
				return 0;
			}

			// 데이터를 정상적으로 수신받았다면 로그 출력
			cout << "Recv Data! Data = " << RecvBuffer << '\n';
			cout << "Recv Data! Len = " << iRecvLen << '\n';

		}
	}

	// 윈속 종료(참고로 WSAStartup함수 호출 횟수 만큼 호출을 해줘야 한다)
	// 호출을 하지 않더라도 문제가 발생하지 않는다(필수사항은 아니고 권장사항이다)
	WSACleanup();

	return 0;
}
