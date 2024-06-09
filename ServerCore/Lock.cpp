#include "pch.h"
#include "Lock.h"
#include "CoreTLS.h"
#include "DeadLockProfiler.h"

// W -> W (O)
// W -> R (O)
// R -> W (X)
// R -> R (O) <- ReadCount 증가

void Lock::WriteLock(const char* _pName)
{
#if _DEBUG // 디버그 모드에서 데드락 탐지
	GDeadLockProfiler->PushLock(_pName);

#endif

	// 동일한 쓰레드가 소유하고 있다면 무조건 성공
	// 현재 Lock을 걸어놓고 있는 쓰레드의 ID를 확인한다.
	const uint32 iLockThreadID = (m_LockFlag.load() & WRITE_THREAD_MASK) >> 16;
	
	
	/* 동일한 쓰레드가 Lock을 소유하고 있다면 바로 반환한다.
	   또한, Lock을 소유하고 있는 쓰레드만 독단적으로 
	   증가시키는 값이기 때문에 Atomic 만들지 않아도 된다 */
	if (LThreadID == iLockThreadID)
	{
		m_iWriteCount++;
		return;
	}

	/* 아무도 소유(Write) 및 공유(Read)하고 있지 않을 때, 경합해서 소유권을 얻는다.
	   현재 스레드 ID에서 16비트를 이동시킨뒤 WRITE_THREAD_MASK와 
	   AND 연산을 해준 값을 m_LockFlag에 저장해준다 */
	const uint32 iDesired = ((LThreadID << 16) & WRITE_THREAD_MASK);

	// 너무 오랫동안 Spin을 할 경우를 방지하기 위한 시작 Tick값
	const int64 iBeginTick = GetTickCount64();
	// Compare Swap 방식으로 Lock 가능 여부 체크
	while (true)
	{
		// MAX_SPIN_COUNT 만큼 반복하며 Lock을 걸어놓을수 있는지 확인
		for (uint32 iSpinCnt = 0; iSpinCnt < MAX_SPIN_COUNT; ++iSpinCnt)
		{
			// EMPTY_FLAG상태 즉, 아무도 소유하고 있지 않는 상태를 전재로 확인한다
			uint32 iExpected = EMPTY_FLAG;

			// 참고로 인자값으로 넣어준 OUT은 값이 변경될 수도 있다는 것을 암시하기 위해 사용했다. 
			if (m_LockFlag.compare_exchange_strong(OUT iExpected, iDesired))
			{
				/* 해당 조건문에 들어왔다면 현재 LockFlag가 EMPTY_FLAG인 경우
				   iDesired(스레드 ID)를 저장하여 Lock의 소유권을 가지게 된다 */

				
				/* m_iWriteCount를 따로 관리하는 이유는 WriteLock()함수를 재귀적으로 
				호출할 수도 있기 때문에, 이러한 경우 Crash를 발생시키는 것이 아니라
				WriteCount를 늘려서 Lock을 재차 잡아주도록 하기 위함이다 */
				m_iWriteCount++;
				return;
			}
		}

		if ((GetTickCount64() - iBeginTick) >= ACQUIRE_TIMEOUT_TICK)
		{
			CRASH("LOCK_TIMEOUT");
		}

		// Sleep 메모에 정리해놨음
		this_thread::yield();
	}
}

void Lock::WriteUnLock(const char* _pName)
{
#if _DEBUG // 디버그 모드에서 데드락 탐지
	GDeadLockProfiler->PopLock(_pName);

#endif

	/* ReadLock을 모두 풀어주기 전까지 WriteUnlock은 불가능하다 ReadLock을
	걸어놓은 상태라는 것은 WriteLock을 걸어놓을 수 없는 상태였다는 뜻 */
	if ((m_LockFlag.load() & READ_COUNT_MASK) != 0) 
	{
		CRASH("INVALID_UNLOCK_ORDER");
	}

	/* Lock을 해제하는 경우 WriteCount에서 1을 
	뺀 값이 0일 경우 EMPTY상태로 변경해준다	*/
	const int32 iLockCount = --m_iWriteCount;
	if (iLockCount == 0)
	{
		m_LockFlag.store(EMPTY_FLAG);
	}

}

void Lock::ReadLock(const char* _pName)
{
#if _DEBUG // 디버그 모드에서 데드락 탐지
	GDeadLockProfiler->PushLock(_pName);

#endif


	// 동일한 쓰레드가 소유하고 있다면 무조건 성공
	const uint32 iLockThreadID = (m_LockFlag.load() & WRITE_THREAD_MASK) >> 16;
	if (LThreadID == iLockThreadID)
	{
		// 다른 쓰레드는 접근을 못 하는 상태가 확실하기 때문에 단순히 1을 증가시킨다
		m_LockFlag.fetch_add(1);
		return;
	}

	// 아무도 소유하고 있지 않는 상태라면 경합해서 공유 카운트를 증가시킨다.
	// 너무 오랫동안 Spin을 할 경우를 방지하기 위한 시작 Tick값
	const int64 iBeginTick = GetTickCount64();
	while (true)
	{
		for (uint32 iSpinCnt = 0; iSpinCnt < MAX_SPIN_COUNT; ++iSpinCnt)
		{
			/* READ_COUNT_MASK의 초기화 값이 0x0000'FFFF이다 보니 현재 Lock을 소유하고 
			있는 쓰레드가 없는 경우 iExpected의 값에는 ReadCount가 들어가게 된다 */
			uint32 iExpected = (m_LockFlag.load() & READ_COUNT_MASK); 
			
			/* 즉, Lock을 소유하고 있는 쓰레드가 없다면 ReadCount만이 남으니까 iExpected,
			iExpected를 1 증가시킨 값을 인자값으로 넘기면 ReadCount를 증가시킬 수 있다 */
			if (m_LockFlag.compare_exchange_strong(OUT iExpected, iExpected + 1))
			{
				return;
			}
		}

		if ((GetTickCount64() - iBeginTick) >= ACQUIRE_TIMEOUT_TICK)
		{
			// 그냥 Crash를 내버리고 문제를 찾는 것이 깔끔하다
			CRASH("LOCK_TIMEOUT");
		}

		/* 실패했다면 이미 Lock을 소유하고 있는 쓰레드가 존재하거나 중간에
		다른 쓰레드에서 ReadCount의 값을 변경하는 두 가지 경우이다 */
		this_thread::yield();
	}

}

void Lock::ReadUnLock(const char* _pName)
{
#if _DEBUG // 디버그 모드에서 데드락 탐지
	GDeadLockProfiler->PopLock(_pName);

#endif


	/* fetch_sub함수는 인자값으로 받아준 값을 뺀 결과값의 이전 값을 반환하기 
	때문에 만약 값이 0이라면 Lock, UnLock의 짝이 맞지 않다는 뜻이다 */
	if ((m_LockFlag.fetch_sub(1) & READ_COUNT_MASK) == 0)
	{
		CRASH("MULTIPLE_UNLOCK");
	}


}
