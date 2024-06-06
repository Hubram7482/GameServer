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

	// �� �����尡 ������ TLS ������ �������� �ϱ� ���ؼ�
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

	// �� �����尡 ������ TLS ������ ��������
	for (thread& iterThread : m_vecThreads)
	{
		if (iterThread.joinable())
			iterThread.join();
	}
}

void ThreadManager::InitTLS()
{
	// �����尡 �����ɶ� ���� ������ ID�� ��������
	static Atomic<uint32> SThreadID = 1;
	LThreadID = SThreadID.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{

}
