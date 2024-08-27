#include "pch.h"
#include "IocpEvent.h"

IocpEvent::IocpEvent(EventType _Type)
	: m_Type(_Type)
{
	/* 생성자에서 Init 함수를 호출하는데 따로 Init 함수를 따로 생성해서
	호출하는 이유는 Init 함수를 외부에서 호출하고 싶은 상황이 발생할 수
	있기 때문이다 */
	Init();

}

void IocpEvent::Init()
{
	// OVERLAPPED 구조체 내부 값들을 초기화한다
	// 해당 값들은 운영체제가 사용할 것이다
	OVERLAPPED::hEvent = 0;
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
	
}
