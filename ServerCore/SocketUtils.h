#pragma once
#include "NetAddress.h"

/*---------------------
	   SocketUtils
-----------------------*/

class SocketUtils
{
public:
	// 
	static LPFN_CONNECTEX	 ConnectEx;
	static LPFN_DISCONNECTEX DisConnectEx;
	static LPFN_ACCEPTEX	 AcceptEx;	

public:
	/* Init, Clear 함수는 현재 CoreGlobal 클래스의 생성/소멸 시점에
	호출하고 있음 */
	static void Init();
	static void Clear();

	static bool BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
	static SOCKET CreateSocket();

	/* 밑에 Set 함수들은 소켓 옵션 관련된 함수들인데 깃허브에
	Socket Option 커밋 주석을 보면 세부 설명 작성해놨음 */

	/* 소켓을 close 했을 때 전송되지 않은 데이터를 어떻게 
	처리할 것인지에 대한 옵션이 SO_LINGER 옵션이다 */
	static bool SetLinger(SOCKET socket, uint16 onoff, uint16 linger);
	//  IP주소 및 포트 재사용 여부에 대한 옵션
	static bool SetReuseAddress(SOCKET socket, bool flag);
	// 사이즈 설정
	static bool SetRecvBufferSize(SOCKET socket, int32 size);
	static bool SetSendBufferSize(SOCKET socket, int32 size);
	static bool SetTcpNoDelay(SOCKET socket, bool flag);
	static bool SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket); 


	static bool Bind(SOCKET socket, NetAddress netAddr);
	// 해당 소켓에다가 임의의 IP 주소를 바인딩 하는 함수
	static bool BindAnyAddress(SOCKET socket, uint16 port);
	// 명시적으로 숫자를 입력하지 않고 알아서 최대 숫자로 골라달라는 옵션(SOMAXCONN)
	static bool Listen(SOCKET socket, int32 backlog = SOMAXCONN);
	static void Close(SOCKET& socket);

};

template<typename T>
static inline bool SetSocketOpt(SOCKET socket, int32 level, int32 optName, T optVal)
{
	// 소켓 옵션을 범용적으로 사용하기 위해 템플릿으로 랩핑
	return SOCKET_ERROR != setsockopt(socket, level, optName, 
		reinterpret_cast<char*>(&optVal), sizeof(T));
}
