#pragma once


/*---------------------
  1차 시도(싱글쓰레드)
-----------------------*/
//
//struct SListEntry
//{
//	SListEntry* pNext;
//
//};
//
///* Header는 첫 번째 데이터를 가르키고 해당 데이터는 다음 데이터를
//가르키는 방식으로 계속해서 이어져 있는 구조로 동작하게 될 것이다
//따라서 Header는 첫 번째 데이터 즉, 노드(SListEntry)를 가르키는데
//실질적인 데이터의 크기는 노드가 포함하고 있는 데이터를 포함하고
//있으며 결론적으로 스택이라는 것 자체가 Header만 알고 있다면 
//데이터를 추가하고 꺼내고 하는 작업을 할 수 있다
//Header -> [(노드) 포함하고있는 데이터]->[]->[]*/
//
//struct SListHeader
//{
//	SListEntry* pNext = { nullptr };
//
//};
//
//// 헤더 초기화 함수
//void InitializeHead(SListHeader* _pHeader);
//// 스택에 데이터를 추가하는 함수
//void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry);
//// 스택에서 데이터를 꺼내서 사용하는 함수
//SListEntry* PopEntrySList(SListHeader* _pHeader);


/*---------------------
  2차 시도(멀티쓰레드)
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
//// 헤더 초기화 함수
//void InitializeHead(SListHeader* _pHeader);
//// 스택에 데이터를 추가하는 함수
//void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry);
//// 스택에서 데이터를 꺼내서 사용하는 함수
//SListEntry* PopEntrySList(SListHeader* _pHeader);


/*---------------------
  3차 시도(멀티쓰레드)
-----------------------*/

/* 무조건 메모리가 16바이트로 정렬되도록 할 것인데 16바이트로
정렬을 한다는 것은 주소의 최하단 비트는 0000으로 만든다는 의미이다
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
	/* union->가장 큰 멤버 변수가 union의 크기가되며 모든 멤버 
	변수는 동일한 메모리 공간을 공유한다 또한 다수의 멤버 변수를 
	선언했더라도 하나의 변수만을 사용할 수 있기 때문에 멤버 변수가
	여러 가지 타입으로 사용될때 Union으로 하나의 메모리 공간에 
	선언함으로써 메모리를 효율적으로 사용할 수 있다 */
	union
	{
		// uint64가 2개 들어가있기 때문에 128Bit 크기를 가진다

		struct
		{
			uint64 iAlignment;
			uint64 iRegion;
		} DUMMYSTRUCTNAME;
		/* DUMMYSTRUCTNAME은 내부적으로 살펴보면 define이
		되어있는데 아무것도 하지 않도록 되어있다 */
		struct
		{
			/* 밑에 멤버 변수들은 uint64라고 선언되었다고
			하더라도 실제로는 그렇지 않고, 비트 단위로 
			쪼개서 사용하고 있는 개념이라고 생각하면 된다 
			각 값을 더하면 128 Byte가 나오며 union 특성을 
			활용하여 이런 방식을 사용하는 것 같다 */
			
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

