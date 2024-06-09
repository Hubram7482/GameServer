#include "pch.h"
#include "Lock.h"
#include "CoreTLS.h"
#include "DeadLockProfiler.h"

// W -> W (O)
// W -> R (O)
// R -> W (X)
// R -> R (O) <- ReadCount ����

void Lock::WriteLock(const char* _pName)
{
#if _DEBUG // ����� ��忡�� ����� Ž��
	GDeadLockProfiler->PushLock(_pName);

#endif

	// ������ �����尡 �����ϰ� �ִٸ� ������ ����
	// ���� Lock�� �ɾ���� �ִ� �������� ID�� Ȯ���Ѵ�.
	const uint32 iLockThreadID = (m_LockFlag.load() & WRITE_THREAD_MASK) >> 16;
	
	
	/* ������ �����尡 Lock�� �����ϰ� �ִٸ� �ٷ� ��ȯ�Ѵ�.
	   ����, Lock�� �����ϰ� �ִ� �����常 ���������� 
	   ������Ű�� ���̱� ������ Atomic ������ �ʾƵ� �ȴ� */
	if (LThreadID == iLockThreadID)
	{
		m_iWriteCount++;
		return;
	}

	/* �ƹ��� ����(Write) �� ����(Read)�ϰ� ���� ���� ��, �����ؼ� �������� ��´�.
	   ���� ������ ID���� 16��Ʈ�� �̵���Ų�� WRITE_THREAD_MASK�� 
	   AND ������ ���� ���� m_LockFlag�� �������ش� */
	const uint32 iDesired = ((LThreadID << 16) & WRITE_THREAD_MASK);

	// �ʹ� �������� Spin�� �� ��츦 �����ϱ� ���� ���� Tick��
	const int64 iBeginTick = GetTickCount64();
	// Compare Swap ������� Lock ���� ���� üũ
	while (true)
	{
		// MAX_SPIN_COUNT ��ŭ �ݺ��ϸ� Lock�� �ɾ������ �ִ��� Ȯ��
		for (uint32 iSpinCnt = 0; iSpinCnt < MAX_SPIN_COUNT; ++iSpinCnt)
		{
			// EMPTY_FLAG���� ��, �ƹ��� �����ϰ� ���� �ʴ� ���¸� ����� Ȯ���Ѵ�
			uint32 iExpected = EMPTY_FLAG;

			// ����� ���ڰ����� �־��� OUT�� ���� ����� ���� �ִٴ� ���� �Ͻ��ϱ� ���� ����ߴ�. 
			if (m_LockFlag.compare_exchange_strong(OUT iExpected, iDesired))
			{
				/* �ش� ���ǹ��� ���Դٸ� ���� LockFlag�� EMPTY_FLAG�� ���
				   iDesired(������ ID)�� �����Ͽ� Lock�� �������� ������ �ȴ� */

				
				/* m_iWriteCount�� ���� �����ϴ� ������ WriteLock()�Լ��� ��������� 
				ȣ���� ���� �ֱ� ������, �̷��� ��� Crash�� �߻���Ű�� ���� �ƴ϶�
				WriteCount�� �÷��� Lock�� ���� ����ֵ��� �ϱ� �����̴� */
				m_iWriteCount++;
				return;
			}
		}

		if ((GetTickCount64() - iBeginTick) >= ACQUIRE_TIMEOUT_TICK)
		{
			CRASH("LOCK_TIMEOUT");
		}

		// Sleep �޸� �����س���
		this_thread::yield();
	}
}

void Lock::WriteUnLock(const char* _pName)
{
#if _DEBUG // ����� ��忡�� ����� Ž��
	GDeadLockProfiler->PopLock(_pName);

#endif

	/* ReadLock�� ��� Ǯ���ֱ� ������ WriteUnlock�� �Ұ����ϴ� ReadLock��
	�ɾ���� ���¶�� ���� WriteLock�� �ɾ���� �� ���� ���¿��ٴ� �� */
	if ((m_LockFlag.load() & READ_COUNT_MASK) != 0) 
	{
		CRASH("INVALID_UNLOCK_ORDER");
	}

	/* Lock�� �����ϴ� ��� WriteCount���� 1�� 
	�� ���� 0�� ��� EMPTY���·� �������ش�	*/
	const int32 iLockCount = --m_iWriteCount;
	if (iLockCount == 0)
	{
		m_LockFlag.store(EMPTY_FLAG);
	}

}

void Lock::ReadLock(const char* _pName)
{
#if _DEBUG // ����� ��忡�� ����� Ž��
	GDeadLockProfiler->PushLock(_pName);

#endif


	// ������ �����尡 �����ϰ� �ִٸ� ������ ����
	const uint32 iLockThreadID = (m_LockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (LThreadID == iLockThreadID)
	{
		// �ٸ� ������� ������ �� �ϴ� ���°� Ȯ���ϱ� ������ �ܼ��� 1�� ������Ų��
		m_LockFlag.fetch_add(1);
		return;
	}

	// �ƹ��� �����ϰ� ���� �ʴ� ���¶�� �����ؼ� ���� ī��Ʈ�� ������Ų��.
	// �ʹ� �������� Spin�� �� ��츦 �����ϱ� ���� ���� Tick��
	const int64 iBeginTick = GetTickCount64();
	while (true)
	{
		for (uint32 iSpinCnt = 0; iSpinCnt < MAX_SPIN_COUNT; ++iSpinCnt)
		{
			/* READ_COUNT_MASK�� �ʱ�ȭ ���� 0x0000'FFFF�̴� ���� ���� Lock�� �����ϰ� 
			�ִ� �����尡 ���� ��� iExpected�� ������ ReadCount�� ���� �ȴ� */
			uint32 iExpected = (m_LockFlag.load() & READ_COUNT_MASK); 
			
			/* ��, Lock�� �����ϰ� �ִ� �����尡 ���ٸ� ReadCount���� �����ϱ� iExpected,
			iExpected�� 1 ������Ų ���� ���ڰ����� �ѱ�� ReadCount�� ������ų �� �ִ� */
			if (m_LockFlag.compare_exchange_strong(OUT iExpected, iExpected + 1))
			{
				return;
			}
		}

		if ((GetTickCount64() - iBeginTick) >= ACQUIRE_TIMEOUT_TICK)
		{
			// �׳� Crash�� �������� ������ ã�� ���� ����ϴ�
			CRASH("LOCK_TIMEOUT");
		}

		/* �����ߴٸ� �̹� Lock�� �����ϰ� �ִ� �����尡 �����ϰų� �߰���
		�ٸ� �����忡�� ReadCount�� ���� �����ϴ� �� ���� ����̴� */
		this_thread::yield();
	}

}

void Lock::ReadUnLock(const char* _pName)
{
#if _DEBUG // ����� ��忡�� ����� Ž��
	GDeadLockProfiler->PopLock(_pName);

#endif


	/* fetch_sub�Լ��� ���ڰ����� �޾��� ���� �� ������� ���� ���� ��ȯ�ϱ� 
	������ ���� ���� 0�̶�� Lock, UnLock�� ¦�� ���� �ʴٴ� ���̴� */
	if ((m_LockFlag.fetch_sub(1) & READ_COUNT_MASK) == 0)
	{
		CRASH("MULTIPLE_UNLOCK");
	}


}
