#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"

char SendBuffer[] = "Hello World!";

class ServerSession : public Session
{
public:
	~ServerSession() 
	{
		cout << "~ServerSession" << '\n';
	}

	virtual void OnConnected() override
	{
		cout << "Connected To Server" << '\n';
		Send((BYTE*)SendBuffer, sizeof(SendBuffer));
	}

	virtual int32 OnRecv(BYTE* _pBuffer, int32 _iLen) override
	{
		cout << "OnRecv Len = " << _iLen << '\n';
		/* 에코 서버는 클라이언트가 서버에게 특정 문자열을 보내면 서버가
		다시 그 문자열을 클라이언트에게 전송해 출력해내는 서버를 말한다 */
		this_thread::sleep_for(1s);

		Send((BYTE*)SendBuffer, sizeof(SendBuffer));
		// Len를 반환하는 이유는 나중에 나온다
		return _iLen;
	}

	virtual void OnSend(int32 _iLen) override
	{
		cout << "OnSend Len = " << _iLen << '\n';
	}

	virtual void OnDisConnected() override
	{
		cout << "DisConnected" << '\n';
	}
};

int main()
{
	this_thread::sleep_for(1s);
	
	ClientServiceRef pService = MakeShared<ClientService>(
		NetAddress(L"192.168.0.5", 7777),
		MakeShared<IocpCore>(),
		MakeShared<ServerSession>, // TODO : SessionManager 등등
		1);

	ASSERT_CRASH(pService->Start());

	for (int32 i = 0; i < 2; ++i)
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
