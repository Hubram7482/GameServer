#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"

IocpCore::IocpCore()
{
	// �ڵ��� �������ش�
	m_IocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	ASSERT_CRASH(m_IocpHandle != INVALID_HANDLE_VALUE);
}

IocpCore::~IocpCore()
{
	CloseHandle(m_IocpHandle);

}

bool IocpCore::Register(IocpObjectRef _pIocpObject)
{
	/* IocpObject�� ���������� ����(GetHandle)�� ������ ���� ���̸�
	�ش� ������ Completion Port���� ���� ����̶�� �˷��ش� */
	return CreateIoCompletionPort(_pIocpObject->GetHandle(), m_IocpHandle, /*key*/0, 0);
}

bool IocpCore::Dispatch(uint32 TimeOutMs)
{
	/* �۾��� ������ ������(Worker Thread)���� �ش� Dispatch 
	�Լ��� ��� �����Ͽ� ó���� �ϰ��� �ִ��� üũ�Ѵ� */

	ULONG_PTR key = 0;
	DWORD numOfBytes = 0;
	IocpEvent*  pIocpEvent = { nullptr };

	/* ���������� �� ��° ���ڰ����� Ű���� �޾ƿ��� �־��µ� �����δ� �̷� �����
	�ƴ϶� IocpEvent���� �ڱ� �ڽ��� ����(����)�� �����͸� ������ �ֵ��� �� ���̴� 
	�׷��� �� ��° ���ڰ����� �ƹ� �ǹ� ���� ������ �Ѱܼ� �Լ� ȣ�� ���Ǹ� ����� */
	if (GetQueuedCompletionStatus(m_IocpHandle, OUT & numOfBytes, OUT &key,
		OUT reinterpret_cast<LPOVERLAPPED*>(&pIocpEvent), TimeOutMs))
	{
		/* ���������� �����͸� �޾ƿԴٸ� pIocpObject�� Dispatch�Լ��� pIocpEvent�� 
		���ڰ����� �Ѱܼ� EventType�� ���� �Լ� ���ο��� ó���Ѵ� */
		IocpObjectRef pIocpObject = pIocpEvent->m_pOwner; 
		pIocpObject->Dispatch(pIocpEvent, numOfBytes);
	}
	else
	{
		/* Error �ڵ尡 TimeOut�� ��� ���ڰ����� �޾ƿ� 
		TimeOutMs ���� �ϰ��� �����ٴ� �ǹ̰� �ȴ� */
		int32 iErrorCode = WSAGetLastError();

		switch (iErrorCode)
		{
		case WAIT_TIMEOUT:
			return false;
			break;
		default:
			// TODO : �α� ���
			// �ϴ��� Dispatch
			IocpObjectRef pIocpObject = pIocpEvent->m_pOwner;
			pIocpObject->Dispatch(pIocpEvent, numOfBytes);
			break;
		}
	}


	return true;
}
