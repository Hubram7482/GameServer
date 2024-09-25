#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"

class GameSession : public Session
{
public:
	~GameSession()
	{
		cout << "~GameSession" << '\n';
	}

	virtual int32 OnRecv(BYTE* _pBuffer, int32 _iLen) override
	{
		/* 에코 서버는 클라이언트가 서버에게 특정 문자열을 보내면 서버가 
		다시 그 문자열을 클라이언트에게 전송해 출력해내는 서버를 말한다 */

		cout << "OnRecv Len = " << _iLen << '\n';
		Send(_pBuffer, _iLen);
		// Len를 반환하는 이유는 나중에 나온다
		return _iLen;
	}

	virtual void OnSend(int32 _iLen) override
	{
		cout << "OnSend Len = " << _iLen << '\n';
	}
};

int main()
{
	ServerServiceRef pService = MakeShared<ServerService>(
		NetAddress(L"192.168.0.5", 7777),
		MakeShared<IocpCore>(),
		MakeShared<GameSession>, // TODO : SessionManager 등등
		100);

	ASSERT_CRASH(pService->Start());
	
	for (int32 i = 0; i < 5; ++i)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					pService->GetIocpCore()->Dispatch();
				}
			});
	}
	
	GThreadManager->Join();

	return 0;
}
