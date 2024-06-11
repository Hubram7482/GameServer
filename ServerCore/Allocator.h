#pragma once


/*---------------------
	  BaseAllocator
-----------------------*/

class BaseAllocator
{
public:
	// 메모리 할당/해제 함수
	static void* Alloc(int32 _iSize);
	static void  Release(void* _pPtr);


};


/*---------------------
	 StompAllocator
-----------------------*/