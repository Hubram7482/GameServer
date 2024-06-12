#include "pch.h"
#include "Allocator.h"

/*---------------------
	  BaseAllocator
-----------------------*/

void* BaseAllocator::Alloc(int32 _iSize)
{
	return malloc(_iSize);
}

void BaseAllocator::Release(void* _pPtr)
{
	free(_pPtr);
}

/*---------------------
	 StompAllocator
-----------------------*/

void* StompAllocator::Alloc(int32 _iSize)
{
	/* ���ڰ����� �޾ƿ� ũ��� ���� ������ ������ ũ�⸦ �ݿø��ؼ� ����Ѵ�
	���� ��� 4Byte�� �Ҵ��� ��� ���� ����� 4096���� �Ҵ��� �� ���̴� */

	/* �Ϲ������� �������� 4kB�� ũ��� �Ǿ� �ִµ�, _iSize�� 4Byte��� 
	4Byte + 4096 -> 4099�� �� ���̰� 4099 / 4096 -> 1.0007.. ������ 
	�ݿø��ϸ� ������ ������ �������� �� ���̴�. ����, PAGE_SIZE��
	-1�� ���� ������ ���� _iSize�� ��Ȯ�� 4KB��ŭ�� ũ���� ���� ���
	(4096 + 4095) / 4096 -> 1.99975.. -> 1 �̷��� ��Ȯ�� ������ ũ�Ⱑ
	�������� �ϱ� �����̴� */
	const int64 iPageCount = (_iSize + (PAGE_SIZE - 1)) / PAGE_SIZE;

	/* �׷��� �ش� ����� ������� �ʴ� ū ������ �Ҵ��ϱ� ������ ��� ������
	�Ѿ ������ ����(�����÷ο�)�� �ϴ��� ������ �߻����� �ʴ´ٴ� ������ �ִ�.
	���� ��� Base�� ��� �޴� SubŬ������ �ְ� SubŬ������ �߰������� 
	��� �������� �ִ� ��Ȳ���� Base��ü�� �����ϰ� ����ȯ�� ���� SubŬ������
	��� ������ ������ �ϴ��� ������ �߻����� ������ �ش� ������ �ذ��ϱ� 
	���ؼ��� �����͸� �߰��� �� �� ��ġ�� ��ġ�� �ؾ� �ϴµ� ���� ��
	4Byte�� �����͸� �߰��Ѵٸ� �뷫 4092��ġ�� ��ġ�ؾ� �Ѵٴ� ���̴�
	���� ��κ��� ������ �����÷ο�κ��� �߻��ϱ� ������ �ڿ� ��ġ�Ѵ� */

	// ex) (������ ���� * 4KB) - 4Byte -> ������ ������ 1�� ��� 4092
	const int64 iDataOffset = (iPageCount * PAGE_SIZE) - _iSize;

	/* VirtualAlloc(NULL, �Ҵ��� �޸� ũ��, �޸� Ÿ��, ��å(�б�/���� ��� ��))
	����� ù ��° ���ڰ����� NULL�� ������ �Ҵ��� �޸� ������ �˾Ƽ� ��ƴ޶�� ��
	MEM_RELEASE | MEM_COMMIT <- �޸𸮸� ����� ���ÿ� ����ϰڴٴ� �ǹ� */
	void* pBaseAddress = VirtualAlloc(NULL, (iPageCount * PAGE_SIZE), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	/* ������ ����� ������ Ÿ�Կ� ���� ������� ������ �ֱ� ������ Byte������
	����ϰ��� int8* Ÿ������ ����ȯ�Ͽ� iDataOffset�� �����ִ� ���̴� */
	return static_cast<void*>(static_cast<int8*>(pBaseAddress) + iDataOffset);
}

void StompAllocator::Release(void* _pPtr)
{
	/* Release�� �ϴ� ��� _pPtr�� �����÷ο츦 �����ϱ� ���� �ڿ� ��ġ�� 
	�������� ���̱� ������ �ش� ��ġ�� �ٽ� ������ ��ܼ� ������ �ؾ��Ѵ� */
	
	// iAddRess�� �������� �ּҸ� int64�� ��ȯ���ش�
	const int64 iAddRess = reinterpret_cast<int64>(_pPtr);
	/* iAddRess�� ���� 4092�� ��� 4092 - (4092 % 4096) -> 4088�� �����µ�
	��, �Ҵ��� ������ ũ�� ��ŭ�� ������ ��ܼ� ���ĵ� ���°� �� ���̴� */
	const int64 iBaseAddRess = iAddRess - (iAddRess % PAGE_SIZE);

	// �޸� ������ �����Ѵٸ� �� ��° ���ڰ��� ������ 0
	// iBaseAddRess�� �ٽ� void*�� ��ȯ�ؼ� ���ڰ����� �Ѱ��ش�
	VirtualFree(reinterpret_cast<void*>(iBaseAddRess), 0, MEM_RELEASE);
}
