#pragma once
#include "Types.h"
#include "MemoryPool.h"

/* �Ʒ��� ���� template����ϸ� �ش� Ŭ������ ��� ������ 
static���� ������ �Ǿ��� ��� �ش� ������ �� �ϳ��� ����
�ϴ� ���� ������, Ŭ�������� �ϳ��� �����ϰ� �Ǵµ�
template�� ���ؼ� �޾��� Type�� � Ÿ������ ������
�Ұ����ϱ� ������ ������ Ŭ������ ������ �Ǳ� �����̴�*/
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
		/* AttachHeader�Լ��� ���ڰ����� s_Pool.Pop()�� ȣ���Ͽ�
		�����͸� �������� �� ��° ���ڰ����� ũ�⸦ �Ѱ��ش�
		�̸� ���ؼ� s_Pool���� �޸𸮸� ���� ����� �ٿ��ش�*/
		Type* pMemory = static_cast<Type*>(MemoryHeader::AttachHeader(s_Pool.Pop(), s_iAllocSize));
#endif 
	
		// placement new�� ���ؼ� �����ڸ� ȣ���� �� ��ȯ�Ѵ�
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
		/* Push�� �ϴ� ���� �޸� ����� ���� ���̱�
		������ �Ҹ��� ȣ�� �� �޸� ����� �����ͼ�
		s_Pool���� �ݳ��Ѵ� */
		s_Pool.Push(MemoryHeader::DetachHeader(_pPtr));
#endif

	}

	template<typename ...Args>
	static shared_ptr<Type> MakeShared(Args&&... _Args)
	{
		// shared_ptr�� ����Ͽ� ��ü�� ����/������ 
		// � ������� ���������� �������ش�
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

