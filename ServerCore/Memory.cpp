#include "pch.h"
#include "Memory.h"
#include "MemoryPool.h"

/*---------------------
		Memory
-----------------------*/

Memory::Memory()
{
	int32 iSize = 0;
	int32 iTableIndex = 0;

	for (iSize = 32; iSize <= 1024; iSize += 32)
	{
		MemoryPool* pPool = new MemoryPool(iSize);
		m_vecPools.push_back(pPool);

		// ex) 0 ~ 32 -> 32 바이트 크기를 관리하는 메모리 풀을 참조
		while (iTableIndex <= iSize)
		{
			m_pPoolTable[iTableIndex] = pPool;
			iTableIndex++;
		}
	}

	for (; iSize <= 2048; iSize += 128)
	{
		MemoryPool* pPool = new MemoryPool(iSize);
		m_vecPools.push_back(pPool);

		while (iTableIndex <= iSize)
		{
			m_pPoolTable[iTableIndex] = pPool;
			iTableIndex++;
		}
	}

	for (; iSize <= 4096; iSize += 256)
	{
		MemoryPool* pPool = new MemoryPool(iSize);
		m_vecPools.push_back(pPool);

		while (iTableIndex <= iSize)
		{
			m_pPoolTable[iTableIndex] = pPool;
			iTableIndex++;
		}
	}
}

Memory::~Memory()
{
	for (MemoryPool* pPool : m_vecPools)
		delete pPool;

	m_vecPools.clear();
}

/* Allocate, Release함수 내부에서 Lock을 잡지 않은 이유는 공용 데이터를 
수정하지 않고 메모리 풀 내부에서 Lock을 잡고 있기 때문이다 */

void* Memory::Allocate(int32 _iSize)
{
	/* _iSize는 헤더를 포함하지 않고 실제로 
	사용하고 싶은 메모리 크기를 의미한다 */

	MemoryHeader* pHeader = { nullptr };

	const int32 iAllocSize = _iSize + sizeof(MemoryHeader);


#ifdef _STOMP
	pHeader = reinterpret_cast<MemoryHeader*>(StompAllocator::Alloc(iAllocSize));

#else
	if (iAllocSize > MAX_ALLOC_SIZE)
	{
		// 메모리 풀링 최대 크기를 넘어서면 일반 할당
		/* 참고로 아래 코드에서 StompAllocator이 아닌 malloc을 사용한 이유가 있는데
		StompAllocator의 방식은 메모리가 필요 없어지면 운영체제를 통해 해당 메모리를
		완전히 해제하여 더 이상 접근을 하지 못 하게 하는 것이었는데 메모리 풀링은
		재사용하기 위해 메모리 공간을 보관하는 방식이다 보니까 조합이 좋지 않다 */
		pHeader = reinterpret_cast<MemoryHeader*>(_aligned_malloc(iAllocSize, SLIST_ALIGNMENT));
	}
	else
	{
		// 메모리 풀에서 꺼내온다
		pHeader = m_pPoolTable[iAllocSize]->Pop();
	}
#endif	
	

	// 현재 할당하는 메모리의 크기를 기입(디버그 용도)
	return MemoryHeader::AttachHeader(pHeader, iAllocSize);
}

void Memory::Release(void* _pPtr)
{
	// _pPtr을 DetachHeader의 인자값으로 넘겨서 MemoryHeader를 받아온다
	// 받아온 MemoryHeader를 통해 메모리 크기를 확인 후 반납한다
	MemoryHeader* pHeader = MemoryHeader::DetachHeader(_pPtr);
	
	const int32 iAllocSize = pHeader->iAllocSize;

	// iAllocSize가 0인 경우라면 Crash
	ASSERT_CRASH(iAllocSize > 0);



#ifdef _STOMP
	StompAllocator::Release(pHeader);

#else
	if (iAllocSize > MAX_ALLOC_SIZE)
	{
		// 메모리 풀링 최대 크기를 벗어나면 일반 해제
		_aligned_free(pHeader);
	}
	else
	{
		// 메모리 풀에 반납한다
		m_pPoolTable[iAllocSize]->Push(pHeader);
	}
#endif
	
}
