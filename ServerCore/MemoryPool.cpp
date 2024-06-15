#include "pch.h"
#include "MemoryPool.h"

MemoryPool::MemoryPool(int32 _iAllocSize)
	: m_iAllocSize(_iAllocSize)
{
	InitializeSListHead(&m_Header);

}

MemoryPool::~MemoryPool()
{
	// pMemory의 값이 NULL이 아닌 경우 반복
	while (MemoryHeader* pMemory = static_cast<MemoryHeader*>(InterlockedPopEntrySList(&m_Header)))
	{
		_aligned_free(pMemory);
	}

}

void MemoryPool::Push(MemoryHeader* _pPtr)
{
	// iAllocSize가 0일 경우 사용하지 않는 상태라고 간주
	_pPtr->iAllocSize = 0;

	// Pool에 메모리 반납
	InterlockedPushEntrySList(&m_Header, static_cast<PSLIST_ENTRY>(_pPtr));

	m_iUseCount.fetch_add(-1);
	m_iReserveCount.fetch_add(-1);
}

MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* pMemory = static_cast<MemoryHeader*>(InterlockedPopEntrySList(&m_Header));

	// 여분이 없다면 새로 생성한다
	if (pMemory == nullptr)
	{
		pMemory = reinterpret_cast<MemoryHeader*>(_aligned_malloc(m_iAllocSize, SLIST_ALIGNMENT));
	}
	else
	{
		ASSERT_CRASH(pMemory->iAllocSize == 0);
		m_iReserveCount.fetch_add(1);
	}

	// 데이터를 꺼내왔기 때문에 Count증가
	m_iUseCount.fetch_add(1);

	return pMemory;
}
