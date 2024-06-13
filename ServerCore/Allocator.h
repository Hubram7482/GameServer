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

/* StompAllocator은 아주 작은 사이즈를 할당하더라도 4KB의 배수인
엄청나게 큰 영역이 할당이 되어 낭비가 심하다는 단점이 있지만
그럼에도 불구하고 개발 단계에서 메모리 오염을 감지할 수 있다는
장점이 있기 때문에 어느 정도 감수를 하고 사용을 한다 */
class StompAllocator
{
	// 메모리 할당 시 페이지 단위로 할당해주기 위한 값
	enum { PAGE_SIZE = 0x1000 };
		
public:
	static void* Alloc(int32 _iSize);
	static void  Release(void* _pPtr);


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

	// _Count는 객체의 개수
	T* allocate(size_t _Count)
	{
		// 메모리 사이즈 계산
		const int32 iSize = static_cast<int32>(_Count * sizeof(T));
		return static_cast<T*>(Xalloc(iSize));
	}

	void deallocate(T* _pPtr, size_t _Count)
	{
		Xrelease(_pPtr);
	}

private:


};