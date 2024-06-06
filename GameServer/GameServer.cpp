#include "pch.h"

#include <atomic>
#include <mutex>
#include <future>
#include <thread>
#include <functional>
#include <queue>
#include "ThreadManager.h"

class TestLock
{
	USE_LOCK;

public:
	int32 TestRead()
	{
		READ_LOCK;

		if (m_queue.empty())
			return -1;
			
		return m_queue.front();
	}

	void TestPush()
	{
		WRITE_LOCK;

		m_queue.push(rand() % 100);
	}

	void TestPop()
	{
		WRITE_LOCK;

		if (!m_queue.empty())
		{
			m_queue.pop();
		}
	}

private:
	queue<int32> m_queue;

};

TestLock TestLock1;

void ThreadMain()
{
	while (true)
	{
		TestLock1.TestPush();
		this_thread::sleep_for(1ms);
	}
}

void ThreadRead()
{
	while (true)
	{
		int32 iNum = TestLock1.TestRead();
		cout << iNum << '\n';
		this_thread::sleep_for(1ms);
	}
}

void ThreadPop()
{
	while (true)
	{
		TestLock1.TestPop();
		this_thread::sleep_for(1ms);
	}
}


int main()
{
	for (int32 i = 0; i < 2; ++i)
	{
		GThreadManager->Launch(ThreadMain);
	}

	for (int32 i = 0; i < 2; ++i)
	{
		GThreadManager->Launch(ThreadPop);
	}

	for (int32 i = 0; i < 5; ++i)
	{
		GThreadManager->Launch(ThreadRead);
	}

	GThreadManager->Join();

	return 0;
}
