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

		// ex) 0 ~ 32 -> 32 ����Ʈ ũ�⸦ �����ϴ� �޸� Ǯ�� ����
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

/* Allocate, Release�Լ� ���ο��� Lock�� ���� ���� ������ ���� �����͸� 
�������� �ʰ� �޸� Ǯ ���ο��� Lock�� ��� �ֱ� �����̴� */

void* Memory::Allocate(int32 _iSize)
{
	/* _iSize�� ����� �������� �ʰ� ������ 
	����ϰ� ���� �޸� ũ�⸦ �ǹ��Ѵ� */

	MemoryHeader* pHeader = { nullptr };

	const int32 iAllocSize = _iSize + sizeof(MemoryHeader);


#ifdef _STOMP
	pHeader = reinterpret_cast<MemoryHeader*>(StompAllocator::Alloc(iAllocSize));

#else
	if (iAllocSize > MAX_ALLOC_SIZE)
	{
		// �޸� Ǯ�� �ִ� ũ�⸦ �Ѿ�� �Ϲ� �Ҵ�
		/* ����� �Ʒ� �ڵ忡�� StompAllocator�� �ƴ� malloc�� ����� ������ �ִµ�
		StompAllocator�� ����� �޸𸮰� �ʿ� �������� �ü���� ���� �ش� �޸𸮸�
		������ �����Ͽ� �� �̻� ������ ���� �� �ϰ� �ϴ� ���̾��µ� �޸� Ǯ����
		�����ϱ� ���� �޸� ������ �����ϴ� ����̴� ���ϱ� ������ ���� �ʴ� */
		pHeader = reinterpret_cast<MemoryHeader*>(_aligned_malloc(iAllocSize, SLIST_ALIGNMENT));
	}
	else
	{
		// �޸� Ǯ���� �����´�
		pHeader = m_pPoolTable[iAllocSize]->Pop();
	}
#endif	
	

	// ���� �Ҵ��ϴ� �޸��� ũ�⸦ ����(����� �뵵)
	return MemoryHeader::AttachHeader(pHeader, iAllocSize);
}

void Memory::Release(void* _pPtr)
{
	// _pPtr�� DetachHeader�� ���ڰ����� �Ѱܼ� MemoryHeader�� �޾ƿ´�
	// �޾ƿ� MemoryHeader�� ���� �޸� ũ�⸦ Ȯ�� �� �ݳ��Ѵ�
	MemoryHeader* pHeader = MemoryHeader::DetachHeader(_pPtr);
	
	const int32 iAllocSize = pHeader->iAllocSize;

	// iAllocSize�� 0�� ����� Crash
	ASSERT_CRASH(iAllocSize > 0);



#ifdef _STOMP
	StompAllocator::Release(pHeader);

#else
	if (iAllocSize > MAX_ALLOC_SIZE)
	{
		// �޸� Ǯ�� �ִ� ũ�⸦ ����� �Ϲ� ����
		_aligned_free(pHeader);
	}
	else
	{
		// �޸� Ǯ�� �ݳ��Ѵ�
		m_pPoolTable[iAllocSize]->Push(pHeader);
	}
#endif
	
}
