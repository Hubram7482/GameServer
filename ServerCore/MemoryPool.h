#pragma once

enum
{
	SLIST_ALIGNMENT = 16
};

/*---------------------
	  MemoryHeader
-----------------------*/


/* 무조건 메모리가 16바이트로 정렬되도록 할 것인데 16바이트로
정렬을 한다는 것은 주소의 최하단 비트는 0000으로 만든다는 의미이다 */
DECLSPEC_ALIGN(SLIST_ALIGNMENT) 
struct MemoryHeader : public SLIST_ENTRY
{
	// [MemoryHeader][Data]
	MemoryHeader(int32 _iSize) : iAllocSize(_iSize) {}


	/* 할당된 메모리에 데이터를 할당한 상태에서 데이터는 
	사용하지 않고 첫 번째 주소만 넘겨주는 함수 */
	static void* AttachHeader(MemoryHeader* _pHeader, int32 _iSize)
	{
		/* placement new 메모리 공간을 할당함과 동시에 
		해당 메모리에 객체의 생성자를 호출한다 */
		new(_pHeader)MemoryHeader(_iSize);
		/* 해당 함수를 통해 데이터를 사용하기 위해 MemoryHeader++ 해서
		C++ 포인터 연산 특성상 MemoryHeader만큼을 건너뛰기 때문에 정확히
		데이터의 시작 위치를 반환하게 될 것이다 */
		return reinterpret_cast<void*>(++_pHeader);
	}

	// 메모리 헤더를 반환해주는 함수
	static MemoryHeader* DetachHeader(void* _pPtr)
	{
		/* 함수 인자값으로 데이터의 시작 주소를 받아주기 때문에 
		MemoryHeader를 추출하기 위해서 형변환 후 1을 뺀다 */

		MemoryHeader* pHeader = reinterpret_cast<MemoryHeader*>(_pPtr) - 1;
		return pHeader;
	}

	// 데이터 크기
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
	/* 실질적으로 Lock-Free-Stack을 관리하는 
	변수(첫 번째 노드를 가르킨다) */
	SLIST_HEADER			m_Header;
	// 각 메모리 풀이 담당하고 있는 메모리 크기
	int32					m_iAllocSize = { 0 };
	// 할당된 메모리의 개수
	Atomic<int32>			m_iUseCount = { 0 };
	Atomic<int32>			m_iReserveCount = { 0 };

};

