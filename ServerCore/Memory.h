#pragma once
#include "Allocator.h"

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

