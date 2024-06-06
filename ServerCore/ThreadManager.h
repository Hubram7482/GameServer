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
	// �ݹ� �Լ��� �޾��ְ�, �����带 �����Ű�� �κ�
	void Launch(function<void(void)> CallBack);
	void Join();

	// �����带 ����/�Ҹ�� ���ÿ� TLS ������ �ʱ�ȭ/����
	static void InitTLS();
	static void DestroyTLS();

private:
	Mutex			m_Mutex;
	vector<thread>  m_vecThreads;

};

