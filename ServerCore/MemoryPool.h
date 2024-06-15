#pragma once

enum
{
	SLIST_ALIGNMENT = 16
};

/*---------------------
	  MemoryHeader
-----------------------*/


/* ������ �޸𸮰� 16����Ʈ�� ���ĵǵ��� �� ���ε� 16����Ʈ��
������ �Ѵٴ� ���� �ּ��� ���ϴ� ��Ʈ�� 0000���� ����ٴ� �ǹ��̴� */
DECLSPEC_ALIGN(SLIST_ALIGNMENT) 
struct MemoryHeader : public SLIST_ENTRY
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

	// �޸� ����� ��ȯ���ִ� �Լ�
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

DECLSPEC_ALIGN(SLIST_ALIGNMENT)
class MemoryPool
{
public:
	MemoryPool(int32 _iAllocSize);
	~MemoryPool();

	void			Push(MemoryHeader* _pPtr);
	MemoryHeader*	Pop();


private:
	/* ���������� Lock-Free-Stack�� �����ϴ� 
	����(ù ��° ��带 ����Ų��) */
	SLIST_HEADER			m_Header;
	// �� �޸� Ǯ�� ����ϰ� �ִ� �޸� ũ��
	int32					m_iAllocSize = { 0 };
	// �Ҵ�� �޸��� ����
	Atomic<int32>			m_iUseCount = { 0 };
	Atomic<int32>			m_iReserveCount = { 0 };

};

