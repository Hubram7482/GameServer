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
	void WriteLock();
	void WriteUnLock();
	void ReadLock();
	void ReadUnLock();

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
	ReadLockGuard(Lock& _Lock) : m_Lock(_Lock) { m_Lock.ReadLock(); }
	~ReadLockGuard() { m_Lock.ReadUnLock(); }

private:
	Lock& m_Lock;

};

class WriteLockGuard
{
public:
	WriteLockGuard(Lock& _Lock) : m_Lock(_Lock) { m_Lock.WriteLock(); }
	~WriteLockGuard() { m_Lock.WriteUnLock(); }

private:
	Lock& m_Lock;

};

