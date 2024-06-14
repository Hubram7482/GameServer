#include "pch.h"
#include "LockFreeStack.h"

/*---------------------
  1차 시도(싱글쓰레드)
-----------------------*/

//// 헤더 초기화 함수
//void InitializeHead(SListHeader* _pHeader)
//{
//	_pHeader->pNext = { nullptr };
//
//}
//
//// 스택에 데이터를 추가하는 함수
//void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry)
//{
//	/* [][][] -> [새로운 데이터][][][] 이런 형태가 되어야한다
//	헤더는 데이터의 시작 주소를 가르키고 있기 때문에 Entry가
//	가르키는 노드를 기존의 시작 주소로 바꿔주고 헤더는 Entry를
//	시작 주소로 바꿔주면 스택 형식으로 데이터가 추가될 것이다 */
//	_pEntry->pNext = _pHeader->pNext;
//	_pHeader->pNext = _pEntry;
//}
//
//// 스택에서 데이터를 꺼내서 사용하는 함수
//SListEntry* PopEntrySList(SListHeader* _pHeader)
//{
//	// FirstNode를 데이터의 시작 주소로 초기화
//	SListEntry* pFirstNode = _pHeader->pNext;
//
//	if (pFirstNode != nullptr)
//	{
//		/* 시작 주소가 가르키는 주소를 헤더가 가르키는 시작 주소로
//		변경하는데 이 과정을 정리해보면	시작 주소의 다음 데이터를
//		헤더가 가르키는 시작 주소로 바꿔주고 기존의 헤더가 가르키던
//		시작 주소를 반환하는 것이다 */
//		_pHeader->pNext = pFirstNode->pNext;
//	}
//
//	return pFirstNode;
//}


/*---------------------
  2차 시도(멀티쓰레드)
-----------------------*/

//// 헤더 초기화 함수
//void InitializeHead(SListHeader* _pHeader)
//{
//	_pHeader->pNext = { nullptr };
//
//}
//
//// 스택에 데이터를 추가하는 함수
//void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry)
//{
//	_pEntry->pNext = _pHeader->pNext;
//
//	/* 1. _pHeader->pNext -> 해당 주소값에 있는 데이터를 참조
//	   2. 최종적으로 반환되기를 기대하는 결과값
//	   3. _pHeader->pNext와 비교할 대상
//
//	   결론적으로 _pHeader->pNext와 _pEntry->pNext를 비교해서 
//	   만약 두 값이 동일하다면 _pEntry값이 _pHeader->pNext에 
//	   들어가게 될 것이고 실패하면 0을 반환하는 방식이다 */
//	while (InterlockedCompareExchange64((int64*)&_pHeader->pNext, 
//		(int64)_pEntry, (int64)_pEntry->pNext) == 0) // 반환값이 0이라면 반복
//	{
//
//	}
//}
//
//// 스택에서 데이터를 꺼내서 사용하는 함수
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
  3차 시도(멀티쓰레드)
-----------------------*/

// 헤더 초기화 함수
void InitializeHead(SListHeader* _pHeader)
{
	_pHeader->iAlignment = 0;
	_pHeader->iRegion = 0;
}

void PushEntrySList(SListHeader* _pHeader, SListEntry* _pEntry)
{
	SListHeader pExpected = {};
	SListHeader pDesired = {};

	/* 16바이트로 정렬되어 있기 때문에 최하단 비트는 0000이며
	이를 활용해서 iNext(60비트)에 주소를 저장하는 방법이다 
	따라서 60비트에 주소를 저장했으니 iReserved(4비트)에는
	추가적인 정보를 저장하여 이를 통해 비교할 수 있을 것이다 */
	pDesired.HeaderX64.iNext = (((uint64)_pEntry) >> 4);

	while (true)
	{
		pExpected = (*_pHeader);

		// 60비트로 저장해놓은 주소를 64비트로 변경하여 넣어준다
		_pEntry->pNext = (SListEntry*)((uint64)pExpected.HeaderX64.iNext << 4);

		// 1을 더해주는 것은 번호표를 발급하는 개념
		pDesired.HeaderX64.iDepth = pExpected.HeaderX64.iDepth + 1;
		pDesired.HeaderX64.iSequence = pExpected.HeaderX64.iSequence + 1;

		
		/* 만약 도중에 다른 쓰레드가 접근해서 데이터를 삭제하거나 추가했다면
		iRegion, iAllignment값이 변경되었을테니 다음 프레임에 바뀐 값을 통해 
		다시 검사를 하는 방식으로 동작한다 */
		if (InterlockedCompareExchange128((int64*)_pHeader, pDesired.iRegion,
			pDesired.iAlignment, (int64*)&pExpected) == 1)
		{
			// 성공하면 빠져나간다
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
		// 헤더의 시작 주소를 추출
		pEntry = (SListEntry*)((uint64)pExpected.HeaderX64.iNext << 4);
		
		if (pEntry == nullptr)
			break;

		// Use-After-Free 
		pDesired.HeaderX64.iNext  = ((uint64)pEntry->pNext) >> 4;
		// Pop을 하면 데이터를 하나 줄어들기 때문에 1을 빼준다 
		pDesired.HeaderX64.iDepth = pExpected.HeaderX64.iDepth - 1;
		pDesired.HeaderX64.iSequence = pExpected.HeaderX64.iSequence + 1;

		if (InterlockedCompareExchange128((int64*)_pHeader, pDesired.iRegion,
			pDesired.iAlignment, (int64*)&pExpected) == 1)
		{
			// 성공하면 빠져나간다
			break;
		}
	}

	return pEntry;
}
