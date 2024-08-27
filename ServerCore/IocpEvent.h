#pragma once

class Session;

enum class EventType : uint8
{
	Connect,
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
	EventType GetType() { return m_Type; }

protected:
	EventType m_Type; 
	
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
	AcceptEvent
-------------------*/

class AcceptEvent : public IocpEvent
{
public:
	AcceptEvent() : IocpEvent(EventType::Accept) { }

	void		SetSession(Session* _pSession) { m_pSession = _pSession; }
	Session*	GetSession() { return m_pSession; }

private:
	Session*	m_pSession = { nullptr };

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

};
