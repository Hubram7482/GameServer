#pragma once

#define OUT

/*---------------
	  Lock
---------------*/

// typeid(this).name() <- ��Ÿ�ӿ� �ش� ��ü�� �̸��� ���´�
// _##(N) �̷������� �ۼ��� ��� �����Ϸ� �ܰ迡�� Index�� ġȯ�ȴ�
#define USE_MANY_LOCKS(Count) Lock _Locks[Count];
#define USE_LOCK			  USE_MANY_LOCKS(1);
#define READ_LOCK_IDX(Idx)	  ReadLockGuard _ReadLockGuard_##Idx(_Locks[Idx], typeid(this).name());
#define READ_LOCK			  READ_LOCK_IDX(0);
#define WRITE_LOCK_IDX(Idx)	  WriteLockGuard _WriteLockGuard_##Idx(_Locks[Idx], typeid(this).name());
#define WRITE_LOCK			  WRITE_LOCK_IDX(0);

/*---------------
	  Crash
---------------*/

#define CRASH(cause)						\
{											\
	uint32* pCrash = nullptr;				\
	__analysis_assume(pCrash != nullptr);	\
	*pCrash = 0xDEADBEEF;					\
}											

#define ASSERT_CRASH(expr)					\
{											\
	if (!(expr))							\
	{										\
		CRASH("ASSERT_CRASH");				\
		__analysis_assume(expr);			\
	}										\
}