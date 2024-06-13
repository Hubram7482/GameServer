#include "pch.h"
#include "MemoryPool.h"

MemoryPool::MemoryPool(int32 _iAllocSize)
	: m_iAllocSize(_iAllocSize)
{

}

MemoryPool::~MemoryPool()
{
	while (m_quePool.size())
	{
		MemoryHeader* pHeader = m_quePool.front();
		m_quePool.pop();
		free(pHeader);
	}

}

void MemoryPool::Push(MemoryHeader* _pHeader)
{
	WRITE_LOCK;
	// iAllocSize�� 0�� ��� ������� �ʴ� ���¶�� ����
	_pHeader->iAllocSize = 0;

	// Pool�� �޸� �ݳ�
	m_quePool.push(_pHeader);
	m_iAllocCount.fetch_add(-1);
}

MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* pHeader = { nullptr };

	{
		WRITE_LOCK;

		// Pool�� ������ �ִ��� Ȯ��
		if (m_quePool.size())
		{
			// ������ �ִٸ� �����͸� �����Ѵ�
			pHeader = m_quePool.front();
			m_quePool.pop();
		}
	}

	// ������ ���ٸ� ���� �����Ѵ�
	if (pHeader == nullptr)
	{
		pHeader = reinterpret_cast<MemoryHeader*>(malloc(m_iAllocSize));
	}
	else
	{
		ASSERT_CRASH(pHeader->iAllocSize == 0);
	}

	// �����͸� �����Ա� ������ Count����
	m_iAllocCount.fetch_add(1);

	return pHeader;
}
