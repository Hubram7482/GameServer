#pragma once

class Session;

enum class EventType : uint8
{
	Connect,
	DisConnect,
	Accept,
	Recv,
	Send
};

/*-------------------
	  IocpEvent
-------------------*/

/* OVERLAPPED�� ��ӹ����� ù ��° Offset���� OVERLAPPED �޸𸮰� 
�� ���� ���̱� ������ IocpEvent�� OVERLAPPED �����Ϳ� ������ 
�ǹ̷� ���� ���̴� 

��������� IocpEvent�� IocpCore�� ��Ʈ��ũ ����� �Լ��� �Ѱ���
�� � ������ �ǹ��ϴ��� ��Ÿ���ٰ� ���� �ȴ�. �׷��� �� �Լ���
ȣ���� �� �ش� IocpEvent�� �����ؼ� �����ϰ� �� ���̴� */

class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType _Type);

	void	  Init();

public:
	EventType m_EventType; 
	IocpObjectRef m_pOwner;
};

/* EventType�� �з��ؼ� �����ϴ� ������ �� Ÿ�Ը��� �ʿ�� �ϴ�
���ڰ��� ���� �� �ֱ� �����̴� �ʼ��� �ƴϰ� IocpEvent���� ��� 
ó���ϴ� ��쵵 �ִ�. 

����� �ش� ����� ����� ��쿡�� ���� �Լ��� ����ϸ� �ȵȴ�.
���� �Լ��� ����ϸ� �����Լ� ���̺��� Offset 0�� �޸𸮿� 
���� �Ǹ鼭  Offset 0���� �ִ� �޸𸮰� OVERLAPPED�� 
���õ� �޸𸮰� �ƴ� �ٸ� ���� �� �� �ֱ� �����̴� */

/*-------------------
	ConnectEvent
-------------------*/

class ConnectEvent : public IocpEvent
{
public:
	ConnectEvent() : IocpEvent(EventType::Connect) { }

};

/*-------------------
   DisConnectEvent
-------------------*/

class DisConnectEvent : public IocpEvent
{
public:
	DisConnectEvent() : IocpEvent(EventType::DisConnect) { }

};

/*-------------------
	AcceptEvent
-------------------*/

class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept) { }


public:
	SessionRef	m_pSession = { nullptr };

};

/*-------------------
	 RecvEvent
-------------------*/


class RecvEvent : public IocpEvent
{
public:
	RecvEvent() : IocpEvent(EventType::Recv) { }

};

/*-------------------
	 SendEvent
-------------------*/

class SendEvent : public IocpEvent
{
public:
	SendEvent() : IocpEvent(EventType::Send) { }

	// TEMP
	vector<BYTE> m_vecBuffer;

};
