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


	// 소켓 또한 사용이 끝났다면 정리해주는 것이 좋다(소켓 리소스 반환)
	// 참고로 closesocket함수를 호출하면 연결중이던 상대방은 해당 소켓에
	// 더 이상 연결을 할 수 없다
	closesocket(ClientSocket);

	// 윈속 종료(참고로 WSAStartup함수 호출 횟수 만큼 호출을 해줘야 한다)
	// 호출을 하지 않더라도 문제가 발생하지 않는다(필수사항은 아니고 권장사항이다)
	WSACleanup();
}
