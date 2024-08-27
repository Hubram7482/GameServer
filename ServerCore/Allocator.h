#pragma once


/*---------------------
	  BaseAllocator
-----------------------*/

class BaseAllocator
{
public:
	// �޸� �Ҵ�/���� �Լ�
	static void* Alloc(int32 _iSize);
	static void  Release(void* _pPtr);

};

/*---------------------
	 StompAllocator
-----------------------*/

/* StompAllocator�� ���� ���� ����� �Ҵ��ϴ��� 4KB�� �����
��û���� ū ������ �Ҵ��� �Ǿ� ���� ���ϴٴ� ������ ������
�׷����� �ұ��ϰ� ���� �ܰ迡�� �޸� ������ ������ �� �ִٴ�
������ �ֱ� ������ ��� ���� ������ �ϰ� ����� �Ѵ� */
class StompAllocator
{
	// �޸� �Ҵ� �� ������ ������ �Ҵ����ֱ� ���� ��
	enum { PAGE_SIZE = 0x1000 };
		
public:
	static void* Alloc(int32 _iSize);
	static void  Release(void* _pPtr);


};

/*-------------------
	PoolAllocator
-------------------*/

class PoolAllocator
{
public:
	static void*	Alloc(int32 size);
	static void		Release(void* ptr);
};


/*---------------------
	 STLAllocator
-----------------------*/

template<typename T>
class STLAllocator
{
public:
	using value_type = T;

	STLAllocator() {}

	template<typename Other>
	STLAllocator(const STLAllocator<Other>&) {}

	// _Count�� ��ü�� ����
	T* allocate(size_t _Count)
	{
		// �޸� ������ ���
		const int32 iSize = static_cast<int32>(_Count * sizeof(T));
		return static_cast<T*>(PoolAllocator::Alloc((iSize)));
	}

	void deallocate(T* _pPtr, size_t _Count)
	{
		PoolAllocator::Release(_pPtr);
	}

private:


};

