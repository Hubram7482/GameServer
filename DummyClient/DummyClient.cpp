#include "pch.h"
#include <iostream>

//네트워크 프로그래밍을 시작하기 위한 라이브러리 추가
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

	/* 비유하자면 연결을 주고 받을 전화기를 만드는 부분
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
	SOCKET ClientSocket = socket(AF_INET, SOCK_DGRAM, 0); 
	// 실패했는지 확인
	if (ClientSocket == INVALID_SOCKET)
	{
		HandleError("Socket");
		return 0;
	}

	// 비유하자면 연결할 목적지를 지정 후 전화를 거는 부분
	// IPv4을 사용하는 경우 SOCKADDR_IN 사용(IPv6는 다름)
	SOCKADDR_IN ServerAddR; 
	/* SOCKADDR_IN 구조체의 내부를 보면 sin_family, sin_port, sin_addr, sin_zero
	이렇게 4개의 변수가 있는데 sin_zero는 사용하지 않으며 sin_zero는 CHAR 배열이며
	사용하지 않을 것이기 때문에 0으로 밀어준다 */
	memset(&ServerAddR, false, sizeof(ServerAddR));
	/* sin_family는 이름 그대로 주소체계이고, sin_port는 단순 USHORT 타입이다
	sin_addr은 IN_ADDR타입인데 간략하게 정리하면 new long 즉, 4바이트 정수이다 */
	ServerAddR.sin_family = AF_INET;
	/* sin_addr에는 4바이트 정수를 받아줘야 하기 때문에 IP주소를 표현할때 정수로
	표현하면 이해하기 힘들다보니 1바이트씩 끊어서 127, 0, 0, 1 이런식으로 
	통상적으로 많이 표현하는데 이런식으로 변환해주는 함수가 inet_pton이다 
	그런데 IP주소는 환경마다 다르기 때문에 127.0.0.1이라는 값을 사용하면 
	문제가 발생하지 않을까 싶지만, 해당 값은 기본적으로 루프백 주소라고
	해서 현재 컴퓨터에게 다시 요청을 보내는 개념이라고 생각하면 된다.
	강의에서는 클라, 서버를 같은 컴퓨터에서 테스트할 것이기 때문에 해당
	방법을 사용한 것이다(온라인 게임을 배포하는 경우라면 사용자 각자의
	서버를 실행하는 주소로 입력을 해줘야 할 것이다) 또한, 하드코딩으로 
	입력한 것은 우선 테스트를 위해서 입력해준 것이다(이후에 변경할거임) */
	inet_pton(AF_INET, "192.168.0.5", &ServerAddR.sin_addr);
	/* prot라는 단어는 항구라는 뜻을 가지고 있듯이 어떠한 의미를 나타내는 
	번호를 지정해주면 되는데, 아무 번호를 지정하는 것은 아니고, 예를 들어서
	연결할 목적지는 어디인지 요청했을때 IP주소와 포타가 한 쌍이 되는 느낌이다
	묘사를 하자면 (민수의 집이 어디지? -> 아파트(IP주소), 몇호(포트)) 라는
	개념이며, 따라서 IP주소와 포트가 일치해야지만 접근을 할 수 있다(서버
	쪽에서 열어준 포트로 동일한 포트로 접근을 해야 한다) 
	참고로, 모든 번호를 사용할 수 있는 것은 아니고 예를 들어서 80번은 
	보통 웹통신, HTTP통신을 위해서 예약이 되어 있는 등 일부 번호는 
	특정 목적에 따라서 예약이 되어 있는 경우가 있기때문에 정말 겹치지
	않을 것 같은 번호를 사용하는게 좋다.
	
	-----htons & 엔디언 이슈-----
	htons는 무엇을 의미하냐면 h가 호스트를 의미하고 n이 네트워크를 의미한다
	따라서 host to network short라는 것을 의미하는데 왜 prot에 777을 바로
	입력하지 않고 htons를 통해서 입력하는 이유는 엔디언 이슈 때문이다.

	엔디언이라고 부르는 숫자를 저장할 때 사용되는 방식으로
	little-endian, big-endian 이렇게 두 가지 방법이 있다.

	예를 들어서 (1, 2, 3, 4, 5, 6, 7, 8)이러한 16진수 숫자들이 있다고 가정했을때
	low [0x78] [0x56] [0x34] [0x12] high <- little-endian
	low [0x12] [0x34] [0x56] [0x78] high <- big-endian
	참고로 위에서 표현한 예시는 메모리를 확인해보면 동일하다는 것을 알 수 있다
	이런 방식으로 저장하는 방식이며, 딱히 어떤 방식이 더 좋다는 것은 없고
	사용하는 CPU에 따라서 둘 중 하나를 선택해서 사용한다(보통 little방식)
	그렇다면 무조건 little 방식으로 표현하면 되지 않을까 싶지만 그렇지는 않다.
	왜냐하면 데이터를 표현하는 대상과 받아주는 대상이 동일한 방식으로 데이터를 
	해석하지 않을 경우 서로 완전히 다른 데이터로 받아들이게 될 것이기 때문이다.
	따라서 데이터 해석 방식을 통일을 해줘야 하며 네트워크에서 사용하는 표준
	방식은 big 방식이다. 이게 중요한 이유는 나중에 주소에 라우터(아직 모름)
	라거나 다양한 네트워크를 보내주는 역할의 장치들도 마찬가지로 포트를 통해서
	데이터를 넘겨주기 때문에 데이터를 해석하는 방식을 일관되도록 해야 한다
	따라서 htons(host to network short)는 호스트에서 네트워크 방식의 엔디언으로
	맞춰주겠다는 함수를 사용한다고 생각하면 된다(반대 버전도 있다) */
	ServerAddR.sin_port = htons(7777);

	// Connected UDP
	// 연결한다고 해서 TCP와 같은 개념은 아니고, 데이터를 
	// 보낼 대상자를 미리 지정해놓는 방식이다
	connect(ClientSocket, (SOCKADDR*)&ServerAddR, sizeof(ServerAddR));

	/* 클라이언트도 마찬가지로 TCP의 경우에는 connect(연결)작업이
	필요했는데 UDP는 그럴 필요 없이 바로 데이터를 전송하면된다 */
	while (true)
	{
		/* send함수를 사용하여 데이터를 전송한다
		첫 번째 인자값은 데이터를 전송할 소켓을 받아주고,
		두 번째 인자값은 버퍼의 포인터를 받아주는데 여기서 
		버퍼란 데이터를 저장하고 있는 배열을 의미한다
		세 번째 인자값은 전송할 데이터의 크기를 받아준다
		네 번째 인자값은 옵션을 설정하는 부분인데 거의 사용하지
		않기 때문에 그냥 0을 넣으면된다 */
		char SendBuffer[100] = "Hello World";


		/* sendto, recvfrom과 같은 디폴트 상태로 동작하는 함수를 사용하는 방식을
		Unconnected UDP라고 말한다. TCP와는 달리 함수에 추가적인 두 개의 인자값을
		넘겨줘야 하는데 (데이터를 보낼 대상, 데이터 크기)를 넘겨주면 되며 참고로, 
		sendto함수를 호출하는 시점에 클라이언트의 IP주소랑 포트 번호가 설정이 된다
		포트 번호는 현재 사용 가능한 포트 중 아무거나 골라서 자동으로 할당이 된다
		내부적으로 이런 식으로 동작하기 때문에 서버와는 달리 bind를 통해서 자신이
		어떤 주소인지를 연동하는 작업을 할 필요가 없다(TCP, UDP 둘 다 동일하지만
		TCP는 send를 할 때 결정되는것이 아니라 connect를 하는 시점에 결정된다) */
		// int32 iResultCode = sendto(ClientSocket, SendBuffer, sizeof(SendBuffer), 0,
		// 	(SOCKADDR*)&ServerAddR, sizeof(ServerAddR));


		// Connected UDP방식을 사용하면 미리 데이터를 전송할 대상을 지정했기
		// 때문에, 바로 send함수를 통해서 데이터를 전송하면 된다
		int32 iResultCode = send(ClientSocket, SendBuffer, sizeof(SendBuffer), 0);

		if (iResultCode == SOCKET_ERROR)
		{
			HandleError("SendTo");
			return 0;
		}

		// 데이터를 정상적으로 전송되었다면 로그 출력
		cout << "SendTo Data! Len = " << sizeof(SendBuffer) << '\n';

		SOCKADDR_IN RecvAddR;
		memset(&RecvAddR, 0, sizeof(RecvAddR));
		int32 iAddrLen = sizeof(RecvAddR);

		char RecvBuffer[1000];
		// int iRecvLen = recvfrom(ClientSocket, RecvBuffer, sizeof(RecvBuffer), 0,
		// 	(SOCKADDR*)&ServerAddR, &iAddrLen);
		
		// 위 방식은 설명했듯이 Unconnected UDP방식이고 Commected UDP방식을 
		// 사용할 경우 마찬가지로 recv함수를 사용하면 된다
		int iRecvLen = recv(ClientSocket, RecvBuffer, sizeof(RecvBuffer), 0);

		if (iRecvLen <= 0)
		{
			HandleError("SendTo");
			return 0;
		}

		cout << "Recv Data! Data = " << RecvBuffer << '\n';
		cout << "Recv Data! Len = " << iRecvLen << '\n';

		this_thread::sleep_for(1s);
	}


	// 소켓 또한 사용이 끝났다면 정리해주는 것이 좋다(소켓 리소스 반환)
	closesocket(ClientSocket);

	// 윈속 종료(참고로 WSAStartup함수 호출 횟수 만큼 호출을 해줘야 한다)
	// 호출을 하지 않더라도 문제가 발생하지 않는다(필수사항은 아니고 권장사항이다)
	WSACleanup();
}
