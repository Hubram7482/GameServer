#pragma once


/*---------------------
	  MemoryHeader
-----------------------*/

struct MemoryHeader
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

	// 메모리 사용이 끝났을 경우 반납해주는 함수
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

class MemoryPool
{
public:
	MemoryPool(int32 _iAllocSize);
	~MemoryPool();

	void			Push(MemoryHeader* _pHeader);
	MemoryHeader*	Pop();


private:
	// 각 메모리 풀이 담당하고 있는 메모리 크기
	int32					m_iAllocSize = { 0 };
	// 할당된 메모리의 개수
	Atomic<int32>			m_iAllocCount = { 0 };

	USE_LOCK;
	queue<MemoryHeader*>	m_quePool;

};

