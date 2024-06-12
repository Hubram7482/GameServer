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


