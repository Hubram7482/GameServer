#include "pch.h"
#include "ThreadManager.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"

/*------------------
	ThreadManager
-------------------*/

ThreadManager::ThreadManager()
{
	InitTLS();
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(function<void(void)> CallBack)
{
	LockGuard Guard(m_Mutex);

	// 각 스레드가 고유의 TLS 영역을 가지도록 하기 위해서
	m_vecThreads.push_back(thread([=]
		{
			InitTLS();
			CallBack();
			DestroyTLS();
		}));
}

void ThreadManager::Join()
{
	LockGuard Guard(m_Mutex);

	// 각 스레드가 고유의 TLS 영역을 가지도록
	for (thread& iterThread : m_vecThreads)
	{
		if (iterThread.joinable())
			iterThread.join();
	}
}

void ThreadManager::InitTLS()
{
	// 스레드가 생성될때 마다 고유의 ID를 가지도록
	static Atomic<uint32> SThreadID = 1;
	LThreadID = SThreadID.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{

}
