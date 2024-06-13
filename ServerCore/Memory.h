#pragma once
#include "Allocator.h"

class MemoryPool;


/*---------------------
		Memory
-----------------------*/

// 메모리 풀을 관리하는 매니저
class Memory
{
	/* 일반적으로 메모리 크기가 커질수록 겹치는 경우가 
	적어지기 때문에 유동적으로 작은 메모리는 많이 
	큰 메모리는 메모리 풀 개수를 줄여줄 것이다 */
	enum
	{
		/* ~1024 까지는 32 Byte 단위로 메모리 풀을 생성
		1024 ~ 2048까지는 128 Byte 단위로 메모리 풀을 생성
		2048 ~ 4096까지는 256 Byte 단위로 메모리 풀을 생성 
		즉, 위에서 설명했듯 크기가 클 수록 개수를 줄인다 */
		POOL_COUNT = (1024 / 32) + (1024 / 128) + (2048 / 256),
		// 4096을 초과하는 매우 큰 크기의 데이터라면 풀링X
		MAX_ALLOC_SIZE = 4096
	};

public:
	Memory();
	~Memory();

	// 메모리 할당 함수
	void* Allocate(int32 _iSize);
	// 메모리 해제 함수
	void  Release(void* _pPtr);

private:
	vector<MemoryPool*> m_vecPools;
	
	// 메모리 크기 <-> 메모리 풀
	// O(1)의 시간복잡도로 찾기 위한 테이블(0 ~ 4096)
	MemoryPool* m_pPoolTable[MAX_ALLOC_SIZE + 1]; 

};


// 메모리 공간을 할당한 다음 해당 메모리에 객체의 생성자를 호출한다.
template<typename Type, typename... Args>
Type* xnew(Args&&... _Args)
{
	Type* memory = static_cast<Type*>(xalloc(sizeof(Type)));

	// 생성자 오버로딩을 고려하여 가변 인자값을 넘겨준다
	new(memory)Type(forward<Args>(_Args)...);

	return memory;
}

template<typename Type>
void xdelete(Type* _pObj)
{
	// 메모리를 해제하기 전에 객체의 소멸자를 호출한다.
	_pObj->~Type();
	xrelease(_pObj);
}

