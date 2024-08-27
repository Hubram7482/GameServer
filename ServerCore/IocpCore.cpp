#include "pch.h"
#include "IocpCore.h"
#include "IocpEvent.h"

// Temp
IocpCore GIocpCore;

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

bool IocpCore::Register(IocpObject* _pIocpObject)
{
	/* IocpObject�� ���������� ����(GetHandle)�� ������ ���� ���̸�
	�ش� ������ Completion Port���� ���� ����̶�� �˷��ش� */
	return CreateIoCompletionPort(_pIocpObject->GetHandle(), m_IocpHandle, 
		reinterpret_cast<ULONG_PTR>(_pIocpObject), 0);
}

bool IocpCore::Dispatch(uint32 TimeOutMs)
{
	/* �۾��� ������ ������(Worker Thread)���� �ش� Dispatch 
	�Լ��� ��� �����Ͽ� ó���� �ϰ��� �ִ��� üũ�Ѵ� */

	DWORD numOfBytes = 0;
	IocpObject* pIocpObject = { nullptr };
	IocpEvent*  pIocpEvent = { nullptr };

	/* �뷫���� �뵵�� �Ʒ��� ����(���� �ٸ� ���� ����)
	
	m_IocpHanle(Completion Port), 
	numOfButes(�Ϸ�� I/O �۾��� ����Ʈ�� ������ ����),
	pIocpObject(Completion Key(�Ϸ�� ����� �۾��� �����ϱ� ���� Ű��),
	pIocpEvent(�Ϸ�� I/O �۾��� ���� OVERLAPPED ����ü ������ ����)) */
	if (GetQueuedCompletionStatus(m_IocpHandle, OUT & numOfBytes, OUT reinterpret_cast<PULONG_PTR>(&pIocpObject),
		OUT reinterpret_cast<LPOVERLAPPED*>(&pIocpEvent), TimeOutMs))
	{
		/* ���������� �����͸� �޾ƿԴٸ� pIocpObject�� Dispatch�Լ��� pIocpEvent�� 
		���ڰ����� �Ѱܼ� EventType�� ���� �Լ� ���ο��� ó���Ѵ� */
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
			pIocpObject->Dispatch(pIocpEvent, numOfBytes);
			break;
		}
	}


	return true;
}
