#include "pch.h"
#include "IocpEvent.h"

IocpEvent::IocpEvent(EventType _Type)
	: m_Type(_Type)
{
	/* �����ڿ��� Init �Լ��� ȣ���ϴµ� ���� Init �Լ��� ���� �����ؼ�
	ȣ���ϴ� ������ Init �Լ��� �ܺο��� ȣ���ϰ� ���� ��Ȳ�� �߻��� ��
	�ֱ� �����̴� */
	Init();

}

void IocpEvent::Init()
{
	// OVERLAPPED ����ü ���� ������ �ʱ�ȭ�Ѵ�
	// �ش� ������ �ü���� ����� ���̴�
	OVERLAPPED::hEvent = 0;
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
	
}
