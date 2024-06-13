#pragma once
#include "Allocator.h"

class MemoryPool;


/*---------------------
		Memory
-----------------------*/

// �޸� Ǯ�� �����ϴ� �Ŵ���
class Memory
{
	/* �Ϲ������� �޸� ũ�Ⱑ Ŀ������ ��ġ�� ��찡 
	�������� ������ ���������� ���� �޸𸮴� ���� 
	ū �޸𸮴� �޸� Ǯ ������ �ٿ��� ���̴� */
	enum
	{
		/* ~1024 ������ 32 Byte ������ �޸� Ǯ�� ����
		1024 ~ 2048������ 128 Byte ������ �޸� Ǯ�� ����
		2048 ~ 4096������ 256 Byte ������ �޸� Ǯ�� ���� 
		��, ������ �����ߵ� ũ�Ⱑ Ŭ ���� ������ ���δ� */
		POOL_COUNT = (1024 / 32) + (1024 / 128) + (2048 / 256),
		// 4096�� �ʰ��ϴ� �ſ� ū ũ���� �����Ͷ�� Ǯ��X
		MAX_ALLOC_SIZE = 4096
	};

public:
	Memory();
	~Memory();

	// �޸� �Ҵ� �Լ�
	void* Allocate(int32 _iSize);
	// �޸� ���� �Լ�
	void  Release(void* _pPtr);

private:
	vector<MemoryPool*> m_vecPools;
	
	// �޸� ũ�� <-> �޸� Ǯ
	// O(1)�� �ð����⵵�� ã�� ���� ���̺�(0 ~ 4096)
	MemoryPool* m_pPoolTable[MAX_ALLOC_SIZE + 1]; 

};


// �޸� ������ �Ҵ��� ���� �ش� �޸𸮿� ��ü�� �����ڸ� ȣ���Ѵ�.
template<typename Type, typename... Args>
Type* xnew(Args&&... _Args)
{
	Type* memory = static_cast<Type*>(xalloc(sizeof(Type)));

	// ������ �����ε��� ����Ͽ� ���� ���ڰ��� �Ѱ��ش�
	new(memory)Type(forward<Args>(_Args)...);

	return memory;
}

template<typename Type>
void xdelete(Type* _pObj)
{
	// �޸𸮸� �����ϱ� ���� ��ü�� �Ҹ��ڸ� ȣ���Ѵ�.
	_pObj->~Type();
	xrelease(_pObj);
}

