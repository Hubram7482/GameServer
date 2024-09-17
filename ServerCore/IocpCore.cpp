#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"

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

bool IocpCore::Register(IocpObjectRef _pIocpObject)
{
	/* IocpObject가 내부적으로 소켓(GetHandle)을 가지고 있을 것이며
	해당 소켓을 Completion Port에게 관찰 대상이라고 알려준다 */
	return CreateIoCompletionPort(_pIocpObject->GetHandle(), m_IocpHandle, /*key*/0, 0);
}

bool IocpCore::Dispatch(uint32 TimeOutMs)
{
	/* 작업을 수행할 쓰레드(Worker Thread)들이 해당 Dispatch 
	함수를 계속 실행하여 처리할 일감이 있는지 체크한다 */

	ULONG_PTR key = 0;
	DWORD numOfBytes = 0;
	IocpEvent*  pIocpEvent = { nullptr };

	/* 이전까지는 세 번째 인자값으로 키값을 받아오고 있었는데 앞으로는 이런 방식이
	아니라 IocpEvent에서 자기 자신의 주인(세션)의 포인터를 가지고 있도록 할 것이다 
	그래서 세 번째 인자값으로 아무 의미 없는 변수를 넘겨서 함수 호출 조건만 맞춘것 */
	if (GetQueuedCompletionStatus(m_IocpHandle, OUT & numOfBytes, OUT &key,
		OUT reinterpret_cast<LPOVERLAPPED*>(&pIocpEvent), TimeOutMs))
	{
		/* 성공적으로 데이터를 받아왔다면 pIocpObject의 Dispatch함수에 pIocpEvent를 
		인자값으로 넘겨서 EventType에 따라 함수 내부에서 처리한다 */
		IocpObjectRef pIocpObject = pIocpEvent->m_pOwner; 
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
			IocpObjectRef pIocpObject = pIocpEvent->m_pOwner;
			pIocpObject->Dispatch(pIocpEvent, numOfBytes);
			break;
		}
	}


	return true;
}
