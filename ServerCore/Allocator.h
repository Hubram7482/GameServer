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