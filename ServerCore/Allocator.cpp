#include "pch.h"
#include "Allocator.h"

/*---------------------
	  BaseAllocator
-----------------------*/

void* BaseAllocator::Alloc(int32 _iSize)
{
	return malloc(_iSize);
}

void BaseAllocator::Release(void* _pPtr)
{
	free(_pPtr);
}

/*---------------------
	 StompAllocator
-----------------------*/

void* StompAllocator::Alloc(int32 _iSize)
{
	/* 인자값으로 받아온 크기와 가장 근접한 페이지 크기를 반올림해서 계산한다
	예를 들어 4Byte를 할당할 경우 가장 가까운 4096으로 할당이 될 것이다 */

	/* 일반적으로 페이지는 4kB의 크기로 되어 있는데, _iSize가 4Byte라면 
	4Byte + 4096 -> 4099가 될 것이고 4099 / 4096 -> 1.0007.. 값으로 
	반올림하면 페이지 개수로 떨어지게 될 것이다. 또한, PAGE_SIZE에
	-1을 해준 이유는 만약 _iSize가 정확히 4KB만큼의 크기라고 했을 경우
	(4096 + 4095) / 4096 -> 1.99975.. -> 1 이렇듯 정확히 페이지 크기가
	나오도록 하기 위함이다 */
	const int64 iPageCount = (_iSize + (PAGE_SIZE - 1)) / PAGE_SIZE;

	/* 그러나 해당 방법은 사용하지 않는 큰 영역을 할당하기 때문에 사용 범위를
	넘어선 공간에 접근(오버플로우)을 하더라도 에러가 발생하지 않는다는 문제가 있다.
	예를 들어 Base를 상속 받는 Sub클래스가 있고 Sub클래스는 추가적으로 
	멤버 변수들이 있는 상황에서 Base객체를 생성하고 형변환을 통해 Sub클래스의
	멤버 변수에 접근을 하더라도 에러가 발생하지 않으며 해당 문제를 해결하기 
	위해서는 데이터를 추가할 때 끝 위치에 배치를 해야 하는데 예를 들어서
	4Byte의 데이터를 추가한다면 대략 4092위치에 배치해야 한다는 것이다
	또한 대부분의 에러는 오버플로우로부터 발생하기 때문에 뒤에 배치한다 */

	// ex) (페이지 개수 * 4KB) - 4Byte -> 페이지 개수가 1인 경우 4092
	const int64 iDataOffset = (iPageCount * PAGE_SIZE) - _iSize;

	/* VirtualAlloc(NULL, 할당할 메모리 크기, 메모리 타입, 정책(읽기/쓰기 허용 등))
	참고로 첫 번째 인자값으로 NULL을 넣으면 할당할 메모리 영역을 알아서 잡아달라는 뜻
	MEM_RELEASE | MEM_COMMIT <- 메모리를 예약과 동시에 사용하겠다는 의미 */
	void* pBaseAddress = VirtualAlloc(NULL, (iPageCount * PAGE_SIZE), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	/* 포인터 계산은 데이터 타입에 따라 결과값에 영향을 주기 때문에 Byte단위로
	계산하고자 int8* 타입으로 형변환하여 iDataOffset을 더해주는 것이다 */
	return static_cast<void*>(static_cast<int8*>(pBaseAddress) + iDataOffset);
}

void StompAllocator::Release(void* _pPtr)
{
	/* Release를 하는 경우 _pPtr은 오버플로우를 방지하기 위해 뒤에 배치된 
	포인터일 것이기 때문에 해당 위치를 다시 앞으로 당겨서 해제를 해야한다 */
	
	// iAddRess에 포인터의 주소를 int64로 변환해준다
	const int64 iAddRess = reinterpret_cast<int64>(_pPtr);
	/* iAddRess가 만약 4092인 경우 4092 - (4092 % 4096) -> 4088이 나오는데
	즉, 할당한 데이터 크기 만큼을 앞으로 당겨서 정렬된 상태가 될 것이다 */
	const int64 iBaseAddRess = iAddRess - (iAddRess % PAGE_SIZE);

	// 메모리 영역을 해제한다면 두 번째 인자값은 무조건 0
	// iBaseAddRess를 다시 void*로 변환해서 인자값으로 넘겨준다
	VirtualFree(reinterpret_cast<void*>(iBaseAddRess), 0, MEM_RELEASE);
}
