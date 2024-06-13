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
	// iAllocSize가 0일 경우 사용하지 않는 상태라고 간주
	_pHeader->iAllocSize = 0;

	// Pool에 메모리 반납
	m_quePool.push(_pHeader);
	m_iAllocCount.fetch_add(-1);
}

MemoryHeader* MemoryPool::Pop()
{
	MemoryHeader* pHeader = { nullptr };

	{
		WRITE_LOCK;

		// Pool에 여분이 있는지 확인
		if (m_quePool.size())
		{
			// 여분이 있다면 데이터를 추출한다
			pHeader = m_quePool.front();
			m_quePool.pop();
		}
	}

	// 여분이 없다면 새로 생성한다
	if (pHeader == nullptr)
	{
		pHeader = reinterpret_cast<MemoryHeader*>(malloc(m_iAllocSize));
	}
	else
	{
		ASSERT_CRASH(pHeader->iAllocSize == 0);
	}

	// 데이터를 꺼내왔기 때문에 Count증가
	m_iAllocCount.fetch_add(1);

	return pHeader;
}
