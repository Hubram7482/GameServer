#pragma once


/*---------------------
  1�� �õ�(�̱۾�����)
-----------------------*/
//
//struct SListEntry
//{
//	SListEntry* pNext;
//
//};
//
///* Header�� ù ��° �����͸� ����Ű�� �ش� �����ʹ� ���� �����͸�
//����Ű�� ������� ����ؼ� �̾��� �ִ� ������ �����ϰ� �� ���̴�
//���� Header�� ù ��° ������ ��, ���(SListEntry)�� ����Ű�µ�
//�������� �������� ũ��� ��尡 �����ϰ� �ִ� �����͸� �����ϰ�
//������ ��������� �����̶�� �� ��ü�� Header�� �˰� �ִٸ� 
//�����͸� �߰��ϰ� ������ �ϴ� �۾��� �� �� �ִ�
//Header -> [(���) �����ϰ��ִ� ������]->[]->[]*/
//
//struct SListHeader
//{
//	SListEntry* pNext = { nullptr };
//
//};
//
//// ��� �ʱ�ȭ �Լ�
//void InitializeHead(SListHeader* _pHeader);
//// ���ÿ� �����͸� �߰��ϴ� �Լ�
//void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry);
//// ���ÿ��� �����͸� ������ ����ϴ� �Լ�
//SListEntry* PopEntrySList(SListHeader* _pHeader);


/*---------------------
  2�� �õ�(��Ƽ������)
-----------------------*/
//
//struct SListEntry
//{
//	SListEntry* pNext;
//
//};
//
//struct SListHeader
//{
//	SListEntry* pNext = { nullptr };
//
//};
//
//// ��� �ʱ�ȭ �Լ�
//void InitializeHead(SListHeader* _pHeader);
//// ���ÿ� �����͸� �߰��ϴ� �Լ�
//void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry);
//// ���ÿ��� �����͸� ������ ����ϴ� �Լ�
//SListEntry* PopEntrySList(SListHeader* _pHeader);


/*---------------------
  3�� �õ�(��Ƽ������)
-----------------------*/

/* ������ �޸𸮰� 16����Ʈ�� ���ĵǵ��� �� ���ε� 16����Ʈ��
������ �Ѵٴ� ���� �ּ��� ���ϴ� ��Ʈ�� 0000���� ����ٴ� �ǹ��̴�
*/
DECLSPEC_ALIGN(16)
struct SListEntry
{
	SListEntry* pNext;

};

DECLSPEC_ALIGN(16) 
struct SListHeader
{
	SListHeader()
	{
		iAlignment = 0;
		iRegion = 0;
	}	
	/* union->���� ū ��� ������ union�� ũ�Ⱑ�Ǹ� ��� ��� 
	������ ������ �޸� ������ �����Ѵ� ���� �ټ��� ��� ������ 
	�����ߴ��� �ϳ��� �������� ����� �� �ֱ� ������ ��� ������
	���� ���� Ÿ������ ���ɶ� Union���� �ϳ��� �޸� ������ 
	���������ν� �޸𸮸� ȿ�������� ����� �� �ִ� */
	union
	{
		// uint64�� 2�� ���ֱ� ������ 128Bit ũ�⸦ ������

		struct
		{
			uint64 iAlignment;
			uint64 iRegion;
		} DUMMYSTRUCTNAME;
		/* DUMMYSTRUCTNAME�� ���������� ���캸�� define��
		�Ǿ��ִµ� �ƹ��͵� ���� �ʵ��� �Ǿ��ִ� */
		struct
		{
			/* �ؿ� ��� �������� uint64��� ����Ǿ��ٰ�
			�ϴ��� �����δ� �׷��� �ʰ�, ��Ʈ ������ 
			�ɰ��� ����ϰ� �ִ� �����̶�� �����ϸ� �ȴ� 
			�� ���� ���ϸ� 128 Byte�� ������ union Ư���� 
			Ȱ���Ͽ� �̷� ����� ����ϴ� �� ���� */
			
			// iDepth + iSequence -> iAllignment
			// iNext  + iReserved -> iRegion
			uint64 iDepth	 : 16;
			uint64 iSequence : 48;
			uint64 iReserved : 4;
			uint64 iNext	 : 60;
		} HeaderX64;
	};
};

void InitializeHead(SListHeader* _pHeader);
void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry);
SListEntry* PopEntrySList(SListHeader* _pHeader);

