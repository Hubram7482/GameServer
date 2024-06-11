#pragma once
#include "Allocator.h"

// �޸� ������ �Ҵ��� ���� �ش� �޸𸮿� ��ü�� �����ڸ� ȣ���Ѵ�.
template<typename Type, typename... Args>
Type* xnew(Args&&... _Args)
{
	Type* memory = static_cast<Type*>(xalloc(sizeof(Type)));

	// ������ �����ε��� ����Ͽ� ���� ���ڰ��� �Ѱ��ش�
	new(memory)Type(forward<Args>(_Args)...);

	return memory;
}

template<typename Type>
void xdelete(Type* _pObj)
{
	// �޸𸮸� �����ϱ� ���� ��ü�� �Ҹ��ڸ� ȣ���Ѵ�.
	_pObj->~Type();
	xrelease(_pObj);
}

