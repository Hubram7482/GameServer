#pragma once
#include "Types.h"
#include "MemoryPool.h"

/* 아래와 같이 template사용하면 해당 클래스의 멤버 변수가 
static으로 선언이 되었을 경우 해당 변수가 딱 하나만 존재
하는 것은 맞지만, 클래스별로 하나씩 존재하게 되는데
template을 통해서 받아준 Type은 어떤 타입일지 예측이
불가능하기 때문에 별개의 클래스로 구분이 되기 때문이다*/
template<typename Type>
class ObjectPool
{
public:
	template<typename... Args> 
	static Type* Pop(Args&& ..._Args)
	{
#ifdef _STOMP
		MemoryHeader* pPtr = reinterpret_cast<MemoryHeader*>(StompAllocator::Alloc(s_iAllocSize));
		Type* pMemory = static_cast<Type*>(MemoryHeader::AttachHeader(pPtr, s_iAllocSize));
#else
		/* AttachHeader함수의 인자값으로 s_Pool.Pop()을 호출하여
		데이터를 꺼내오고 두 번째 인자값으로 크기를 넘겨준다
		이를 통해서 s_Pool에서 메모리를 꺼내 헤더를 붙여준다*/
		Type* pMemory = static_cast<Type*>(MemoryHeader::AttachHeader(s_Pool.Pop(), s_iAllocSize));
#endif 
	
		// placement new를 통해서 생성자를 호출한 뒤 반환한다
		new(pMemory)Type(forward<Args>(_Args)...);
		return pMemory;
	}

	template<typename... Args>
	static void Push(Type* _pPtr)
	{
		_pPtr->~Type();

#ifdef _STOMP
		StompAllocator::Release(MemoryHeader::DetachHeader(_pPtr));
#else
		/* Push를 하는 경우는 메모리 사용이 끝난 것이기
		때문에 소멸자 호출 후 메모리 헤더를 꺼내와서
		s_Pool에게 반납한다 */
		s_Pool.Push(MemoryHeader::DetachHeader(_pPtr));
#endif

	}

	template<typename ...Args>
	static shared_ptr<Type> MakeShared(Args&&... _Args)
	{
		// shared_ptr을 사용하여 객체를 생성/삭제시 
		// 어떤 방식으로 동작할지를 지정해준다
		shared_ptr<Type> pPtr = { Pop(forward<Args>(_Args)), Push() };
		return pPtr;
	}

private:
	static int32 s_iAllocSize;
	static MemoryPool s_Pool;

};

template<typename Type>
int32 ObjectPool<Type>::s_iAllocSize = sizeof(Type) + sizeof(MemoryHeader);

template<typename Type>
MemoryPool ObjectPool<Type>::s_Pool{ s_iAllocSize };

