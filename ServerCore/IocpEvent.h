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

/* OVERLAPPED를 상속받으면 첫 번째 Offset에는 OVERLAPPED 메모리가 
들어가 있을 것이기 때문에 IocpEvent는 OVERLAPPED 포인터와 동일한 
의미로 사용될 것이다 

결론적으로 IocpEvent는 IocpCore에 네트워크 입출력 함수를 넘겨줄
때 어떤 사유를 의미하는지 나타낸다고 보면 된다. 그래서 각 함수를
호출할 때 해당 IocpEvent를 생성해서 전달하게 될 것이다 */

class IocpEvent : public OVERLAPPED
{
public:
	IocpEvent(EventType _Type);

	void	  Init();
	EventType GetType() { return m_Type; }

protected:
	EventType m_Type; 
	
};

/* EventType을 분류해서 관리하는 이유는 각 타입마다 필요로 하는
인자값이 있을 수 있기 때문이다 필수는 아니고 IocpEvent에서 모두 
처리하는 경우도 있다. 

참고로 해당 방식을 사용할 경우에는 가상 함수를 사용하면 안된다.
가상 함수를 사용하면 가상함수 테이블이 Offset 0번 메모리에 
들어가게 되면서  Offset 0번에 있던 메모리가 OVERLAPPED와 
관련된 메모리가 아닌 다른 것이 들어갈 수 있기 때문이다 */

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
