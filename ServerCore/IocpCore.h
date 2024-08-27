#pragma once

/*-------------------
	 IocpObject
-------------------*/

// �������� ����ߴ� Session ����ü�� ���Ұ� �����ϴ�
class IocpObject
{
public:
	virtual HANDLE GetHandle() abstract;
	// � �ϰ����� IocpEvent�� �޾Ƽ� Dispatch �Լ����� ó���Ѵ�
	virtual void Dispatch(class IocpEvent* _pIocpEvent, int32 _iNumOfBytes = 0) abstract;

};

/*-------------------
	  IocpCore
-------------------*/

/* �뷫���� �帧�� ���ڸ� �����ڿ��� Completion Port�� �������ְ�
Regsiter �Լ��� ���ؼ� ������ ������ ����ϰ� Dispatch �Լ��� 
��� ȣ���ϸ鼭 �ϰ��� �ִ��� üũ�ϰ� ���� �ϰ��� �ִٸ�
�ش� �ϰ��� ó���ϴ� ������� �����Ѵ� 

1. Completion Port ���� 
2. ������ ����(IocpObject) ���
3. Completion Port�� ��ϵ� ���� ������ �Ϸ�� 
�ϰ�(Recv, Send ���� ����� �۾�)�� �ִ��� Ȯ��
4. �Ϸ�� �ϰ��� �ִٸ� ��� ���¿��� �������Ȱ� ���ÿ� 
�Ϸ�� ����� �۾��� ���� ������ ��ȯ�Ѵ�.
5. �� �������� IOCP�� ��� ���� ������(Worker Thread)�ϳ��� 
Ȱ��ȭ�Ͽ� �Ϸ�� �۾��� ó���Ѵ�

�߰� �����
����� ������ ������� �Ϸ�Ǹ� Completion Packet�� ����� 
Completion Packet�� ������� �Ϸ��� ���Ͽ� ���� ����� 
���� ������ ������ �ǹ��ϴµ� �ش� Packet�� Completion Queue��
���� �ְ� ������� ť���� Packet�� ������ ������ ó���Ѵ� */

class IocpCore
{
public:
	IocpCore();
	~IocpCore();

	HANDLE	GetHandle() { return m_IocpHandle; }

	// ������ Iocp�� ����ϴ� �Լ�
	bool	Register(class IocpObject* _pIocpObject);
	// ��������� Iocp�� �ϰ��� �ִ��� Ȯ���ϴ� �Լ�
	bool	Dispatch(uint32 TimeOutMs = INFINITE);

private:
	HANDLE	m_IocpHandle;

};

// Temp
extern IocpCore GIocpCore;