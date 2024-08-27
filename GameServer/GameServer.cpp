#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <windows.h>
#include <future>
#include "ThreadManager.h"


#include "SocketUtils.h"
#include "Listener.h"


int main()
{
	// TEMP
	Listener pListener;
	pListener.StartAccept(NetAddress(L"192.168.0.5", 7777));

	for (int32 i = 0; i < 5; ++i)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					GIocpCore.Dispatch();
				}
			});
	}

	GThreadManager->Join();

	return 0;
}
