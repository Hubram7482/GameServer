#pragma once


/*---------------------
	  MemoryHeader
-----------------------*/

struct MemoryHeader
{
	// [MemoryHeader][Data]
	MemoryHeader(int32 _iSize) : iAllocSize(_iSize) {}


	/* �Ҵ�� �޸𸮿� �����͸� �Ҵ��� ���¿��� �����ʹ� 
	������� �ʰ� ù ��° �ּҸ� �Ѱ��ִ� �Լ� */
	
	static void* AttachHeader(MemoryHeader* _pHeader, int32 _iSize)
	{
		/* placement new �޸� ������ �Ҵ��԰� ���ÿ� 
		�ش� �޸𸮿� ��ü�� �����ڸ� ȣ���Ѵ� */
		new(_pHeader)MemoryHeader(_iSize);
		/* �ش� �Լ��� ���� �����͸� ����ϱ� ���� MemoryHeader++ �ؼ�
		C++ ������ ���� Ư���� MemoryHeader��ŭ�� �ǳʶٱ� ������ ��Ȯ��
		�������� ���� ��ġ�� ��ȯ�ϰ� �� ���̴� */
		return reinterpret_cast<void*>(++_pHeader);
	}

	// �޸� ����� ������ ��� �ݳ����ִ� �Լ�
	static MemoryHeader* DetachHeader(void* _pPtr)
	{
		/* �Լ� ���ڰ����� �������� ���� �ּҸ� �޾��ֱ� ������ 
		MemoryHeader�� �����ϱ� ���ؼ� ����ȯ �� 1�� ���� */

		MemoryHeader* pHeader = reinterpret_cast<MemoryHeader*>(_pPtr) - 1;
		return pHeader;
	}

	// ������ ũ��
	int32 iAllocSize;
};

/*---------------------
	  MemoryPool
-----------------------*/

class MemoryPool
{
public:
	MemoryPool(int32 _iAllocSize);
	~MemoryPool();

	void			Push(MemoryHeader* _pHeader);
	MemoryHeader*	Pop();


private:
	// �� �޸� Ǯ�� ����ϰ� �ִ� �޸� ũ��
	int32					m_iAllocSize = { 0 };
	// �Ҵ�� �޸��� ����
	Atomic<int32>			m_iAllocCount = { 0 };

	USE_LOCK;
	queue<MemoryHeader*>	m_quePool;

};

