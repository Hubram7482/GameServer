#pragma once

#include "Types.h"

/*------------------
	RW SpinLock
-------------------*/

class Lock
{
	enum : uint32
	{
		ACQUIRE_TIMEOUT_TICK = 10000, 
		MAX_SPIN_COUNT = 5000, 
		/* 32��Ʈ�� Ȱ���ؼ� ������ ���̱� ������ 
		   ��Ȯ�� 16��Ʈ�� �����ϱ� ���� ����ũ */
		WRITE_THREAD_MASK = 0xFFFF'0000,
		READ_COUNT_MASK = 0x0000'FFFF,
		EMPTY_FLAG = 0x0000'0000,
	};

public:
	void WriteLock(const char* _pName);
	void WriteUnLock(const char* _pName);
	void ReadLock(const char* _pName);
	void ReadUnLock(const char* _pName);

private:
	Atomic<uint32> m_LockFlag = EMPTY_FLAG;
	uint16 m_iWriteCount = 0;

};

/*------------------
	LockGuard
-------------------*/


// RAII(��ü ���� �� Lock, ��ü �Ҹ� �� Unlock)
class ReadLockGuard
{
public:
	ReadLockGuard(Lock& _Lock, const char* _pName) 
		: m_Lock(_Lock), m_pName(_pName) { m_Lock.ReadLock(_pName); }

	~ReadLockGuard() { m_Lock.ReadUnLock(m_pName); }

private:
	Lock& m_Lock;
	const char* m_pName;

};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& _Lock, const char* _pName) 
		: m_Lock(_Lock), m_pName(_pName) { m_Lock.WriteLock(_pName); }

	~WriteLockGuard() { m_Lock.WriteUnLock(m_pName); }

private:
	Lock& m_Lock;
	const char* m_pName;

};

