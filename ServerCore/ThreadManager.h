#pragma once

#include <thread>
#include <functional>

/*------------------
	ThreadManager
-------------------*/

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

public:
	// 콜백 함수를 받아주고, 스레드를 실행시키는 부분
	void Launch(function<void(void)> CallBack);
	void Join();

	// 스레드를 생성/소멸과 동시에 TLS 영역을 초기화/삭제
	static void InitTLS();
	static void DestroyTLS();

private:
	Mutex			m_Mutex;
	vector<thread>  m_vecThreads;

};

