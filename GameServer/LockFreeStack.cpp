#include "pch.h"
#include "LockFreeStack.h"

/*---------------------
  1�� �õ�(�̱۾�����)
-----------------------*/

//// ��� �ʱ�ȭ �Լ�
//void InitializeHead(SListHeader* _pHeader)
//{
//	_pHeader->pNext = { nullptr };
//
//}
//
//// ���ÿ� �����͸� �߰��ϴ� �Լ�
//void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry)
//{
//	/* [][][] -> [���ο� ������][][][] �̷� ���°� �Ǿ���Ѵ�
//	����� �������� ���� �ּҸ� ����Ű�� �ֱ� ������ Entry��
//	����Ű�� ��带 ������ ���� �ּҷ� �ٲ��ְ� ����� Entry��
//	���� �ּҷ� �ٲ��ָ� ���� �������� �����Ͱ� �߰��� ���̴� */
//	_pEntry->pNext = _pHeader->pNext;
//	_pHeader->pNext = _pEntry;
//}
//
//// ���ÿ��� �����͸� ������ ����ϴ� �Լ�
//SListEntry* PopEntrySList(SListHeader* _pHeader)
//{
//	// FirstNode�� �������� ���� �ּҷ� �ʱ�ȭ
//	SListEntry* pFirstNode = _pHeader->pNext;
//
//	if (pFirstNode != nullptr)
//	{
//		/* ���� �ּҰ� ����Ű�� �ּҸ� ����� ����Ű�� ���� �ּҷ�
//		�����ϴµ� �� ������ �����غ���	���� �ּ��� ���� �����͸�
//		����� ����Ű�� ���� �ּҷ� �ٲ��ְ� ������ ����� ����Ű��
//		���� �ּҸ� ��ȯ�ϴ� ���̴� */
//		_pHeader->pNext = pFirstNode->pNext;
//	}
//
//	return pFirstNode;
//}


/*---------------------
  2�� �õ�(��Ƽ������)
-----------------------*/

//// ��� �ʱ�ȭ �Լ�
//void InitializeHead(SListHeader* _pHeader)
//{
//	_pHeader->pNext = { nullptr };
//
//}
//
//// ���ÿ� �����͸� �߰��ϴ� �Լ�
//void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry)
//{
//	_pEntry->pNext = _pHeader->pNext;
//
//	/* 1. _pHeader->pNext -> �ش� �ּҰ��� �ִ� �����͸� ����
//	   2. ���������� ��ȯ�Ǳ⸦ ����ϴ� �����
//	   3. _pHeader->pNext�� ���� ���
//
//	   ��������� _pHeader->pNext�� _pEntry->pNext�� ���ؼ� 
//	   ���� �� ���� �����ϴٸ� _pEntry���� _pHeader->pNext�� 
//	   ���� �� ���̰� �����ϸ� 0�� ��ȯ�ϴ� ����̴� */
//	while (InterlockedCompareExchange64((int64*)&_pHeader->pNext, 
//		(int64)_pEntry, (int64)_pEntry->pNext) == 0) // ��ȯ���� 0�̶�� �ݺ�
//	{
//
//	}
//}
//
//// ���ÿ��� �����͸� ������ ����ϴ� �Լ�
//SListEntry* PopEntrySList(SListHeader* _pHeader)
//{
//	SListEntry* pExpected = _pHeader->pNext;
//
//	while (pExpected && InterlockedCompareExchange64((int64*)&_pHeader->pNext,
//		(int64)pExpected->pNext, (int64)pExpected) == 0)
//	{
//
//	}
//
//	return pFirstNode;
//}


/*---------------------
  3�� �õ�(��Ƽ������)
-----------------------*/

// ��� �ʱ�ȭ �Լ�
void InitializeHead(SListHeader* _pHeader)
{
	_pHeader->iAlignment = 0;
	_pHeader->iRegion = 0;
}

void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry)
{
	SListHeader pExpected = {};
	SListHeader pDesired = {};

	/* 16����Ʈ�� ���ĵǾ� �ֱ� ������ ���ϴ� ��Ʈ�� 0000�̸�
	�̸� Ȱ���ؼ� iNext(60��Ʈ)�� �ּҸ� �����ϴ� ����̴� 
	���� 60��Ʈ�� �ּҸ� ���������� iReserved(4��Ʈ)����
	�߰����� ������ �����Ͽ� �̸� ���� ���� �� ���� ���̴� */
	pDesired.HeaderX64.iNext = (((uint64)_pEntry) >> 4);

	while (true)
	{
		pExpected = (*_pHeader);

		// 60��Ʈ�� �����س��� �ּҸ� 64��Ʈ�� �����Ͽ� �־��ش�
		_pEntry->pNext = (SListEntry*)((uint64)pExpected.HeaderX64.iNext << 4);

		// 1�� �����ִ� ���� ��ȣǥ�� �߱��ϴ� ����
		pDesired.HeaderX64.iDepth = pExpected.HeaderX64.iDepth + 1;
		pDesired.HeaderX64.iSequence = pExpected.HeaderX64.iSequence + 1;

		
		/* ���� ���߿� �ٸ� �����尡 �����ؼ� �����͸� �����ϰų� �߰��ߴٸ�
		iRegion, iAllignment���� ����Ǿ����״� ���� �����ӿ� �ٲ� ���� ���� 
		�ٽ� �˻縦 �ϴ� ������� �����Ѵ� */
		if (InterlockedCompareExchange128((int64*)_pHeader, pDesired.iRegion,
			pDesired.iAlignment, (int64*)&pExpected) == 1)
		{
			// �����ϸ� ����������
			break;
		}
	}
}

SListEntry* PopEntrySList(SListHeader* _pHeader)
{
	SListHeader pExpected = {};
	SListHeader pDesired  = {};
	SListEntry* pEntry = { nullptr };


	while (true)
	{
		pExpected = (*_pHeader);
		// ����� ���� �ּҸ� ����
		pEntry = (SListEntry*)((uint64)pExpected.HeaderX64.iNext << 4);
		
		if (pEntry == nullptr)
			break;

		// Use-After-Free 
		pDesired.HeaderX64.iNext  = ((uint64)pEntry->pNext) >> 4;
		// Pop�� �ϸ� �����͸� �ϳ� �پ��� ������ 1�� ���ش� 
		pDesired.HeaderX64.iDepth = pExpected.HeaderX64.iDepth - 1;
		pDesired.HeaderX64.iSequence = pExpected.HeaderX64.iSequence + 1;

		if (InterlockedCompareExchange128((int64*)_pHeader, pDesired.iRegion,
			pDesired.iAlignment, (int64*)&pExpected) == 1)
		{
			// �����ϸ� ����������
			break;
		}
	}

	return pEntry;
}
