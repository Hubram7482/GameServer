#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"

// Temp
IocpCore GIocpCore;

IocpCore::IocpCore()
{
	// 핸들을 생성해준다
	m_IocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	ASSERT_CRASH(m_IocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	CloseHandle(m_IocpHandle);

}

bool IocpCore::Register(IocpObject* _pIocpObject)
{
	/* IocpObject가 내부적으로 소켓(GetHandle)을 가지고 있을 것이며
	해당 소켓을 Completion Port에게 관찰 대상이라고 알려준다 */
	return CreateIoCompletionPort(_pIocpObject->GetHandle(), m_IocpHandle, 
		reinterpret_cast<ULONG_PTR>(_pIocpObject), 0);
}

bool IocpCore::Dispatch(uint32 TimeOutMs)
{
	/* 작업을 수행할 쓰레드(Worker Thread)들이 해당 Dispatch 
	함수를 계속 실행하여 처리할 일감이 있는지 체크한다 */

	DWORD numOfBytes = 0;
	IocpObject* pIocpObject = { nullptr };
	IocpEvent*  pIocpEvent = { nullptr };

	/* 대략적인 용도는 아래와 같다(조금 다를 수도 있음)
	
	m_IocpHanle(Completion Port), 
	numOfButes(완료된 I/O 작업의 바이트를 저장할 변수),
	pIocpObject(Completion Key(완료된 입출력 작업을 구분하기 위한 키값),
	pIocpEvent(완료된 I/O 작업에 대한 OVERLAPPED 구조체 포인터 변수)) */
	if (GetQueuedCompletionStatus(m_IocpHandle, OUT & numOfBytes, OUT reinterpret_cast<PULONG_PTR>(&pIocpObject),
		OUT reinterpret_cast<LPOVERLAPPED*>(&pIocpEvent), TimeOutMs))
	{
		/* 성공적으로 데이터를 받아왔다면 pIocpObject의 Dispatch함수에 pIocpEvent를 
		인자값으로 넘겨서 EventType에 따라 함수 내부에서 처리한다 */
		pIocpObject->Dispatch(pIocpEvent, numOfBytes);
	}
	else
	{
		/* Error 코드가 TimeOut인 경우 인자값으로 받아온 
		TimeOutMs 동안 일감이 없었다는 의미가 된다 */
		int32 iErrorCode = WSAGetLastError();

		switch (iErrorCode)
		{
		case WAIT_TIMEOUT:
			return false;
			break;
		default:
			// TODO : 로그 찍기

			// 일단은 Dispatch
			pIocpObject->Dispatch(pIocpEvent, numOfBytes);
			break;
		}
	}


	return true;
}
