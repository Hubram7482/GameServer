#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"

#include "Service.h"
#include "Session.h"

class GameSession : public Session
{

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
