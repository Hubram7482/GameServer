#include "pch.h"
#include "MemoryPool.h"

MemoryPool::MemoryPool(int32 _iAllocSize)
	: m_iAllocSize(_iAllocSize)
{
	InitializeSListHead(&m_Header);

}

MemoryPool::~MemoryPool()
{
	// pMemory�� ���� NULL�� �ƴ� ��� �ݺ�
	while (MemoryHeader* pMemory = static_cast<MemoryHeader*>(InterlockedPopEntrySList(&m_Header)))
	{
		_aligned_free(pMemory);
	}

}

void MemoryPool::Push(MemoryHeader* _pPtr)
{
	// iAllocSize�� 0�� ��� ������� �ʴ� ���¶�� ����
	_pPtr->iAllocSize = 0;

	// Pool�� �޸� �ݳ�
	InterlockedPushEntrySList(&m_Header, static_cast<PSLIST_ENTRY>(_pPtr));

	m_iUseCount.fetch_add(-1);
	m_iReserveCount.fetch_add(-1);
}

MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* pMemory = static_cast<MemoryHeader*>(InterlockedPopEntrySList(&m_Header));

	// ������ ���ٸ� ���� �����Ѵ�
	if (pMemory == nullptr)
	{
		pMemory = reinterpret_cast<MemoryHeader*>(_aligned_malloc(m_iAllocSize, SLIST_ALIGNMENT));
	}
	else
	{
		ASSERT_CRASH(pMemory->iAllocSize == 0);
		m_iReserveCount.fetch_add(1);
	}

	// �����͸� �����Ա� ������ Count����
	m_iUseCount.fetch_add(1);

	return pMemory;
}
