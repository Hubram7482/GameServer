#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

class Service;

// Listener와 마찬가지로 IOCP코어에 등록할 대상이다
class Session : public IocpObject
{
	friend class Listener;
	friend class IocpCore;
	friend class Service;

public:
	Session();
	virtual ~Session();

	// TEMP
	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB(기본적으로 가장 적당한 값이라고 한다)
	};

public:
	/* 외부에서 사용 */
	void				Send(BYTE* _pBuffer, int32 _iLen);
	bool				Connect();

	// 연결을 해제하게 된 사유를 인자값으로 받는다
	void				DisConnect(const WCHAR* _Cause);
	shared_ptr<Service> GetService() { return m_Service.lock(); }
	void				SetService(shared_ptr<Service> _Service) { m_Service = _Service; }

public:
	/* 정보 관련 */
	void				SetNetAddress(NetAddress _NetAddress) { m_NetAdress = _NetAddress; }
	NetAddress			GetNetAddress() { return m_NetAdress; }

	SOCKET				GetSocket() { return m_Socket; }
	bool				IsConnected() { return m_Connected; }
	// 참고로 static_pointer_cast는 스마트 포인터를 형변환하기 위해 사용함
	SessionRef			GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

private:
	/* 인터페이스 구현 */
	virtual HANDLE		GetHandle() override;
	// 어떤 일감인지 IocpEvent로 받아서 Dispatch 함수에서 처리한다
	virtual void		Dispatch(class IocpEvent* _pIocpEvent, int32 _iNumOfBytes = 0) override;

private:
	/* 전송 관련 */
	bool				RegisterConnect();
	bool				RegisterDisConnect();
	void				RegisterRecv();
	void				RegisterSend(SendEvent* _pSendEvent);

	void				ProcessConnect();
	void				ProcessDisConnect();
	void				ProcessRecv(int32 _iNumOfBytes);
	void				ProcessSend(SendEvent* _pSendEvent, int32 _iNumOfBytes);

	void				HandleError(int32 _iErrorCode);

protected:
	/* 컨텐츠 코드에서 오버로딩 */
	virtual void		OnConnected() { };
	virtual int32		OnRecv(BYTE* _pBuffer, int32 _iLen) { return _iLen; }
	virtual void		OnSend(int32 _iLen) { };
	virtual void		OnDisConnected() { };

public:
	/* Send 함수를 통해 전송할 데이터를 SendBuffer에 보관해야 하는데 무작정
	계속해서 데이터를 밀어넣으면 기존에 있던 영역을 덮어쓸 수는 없다.
	
	데이터를 밀어넣는 몇 가지 방법은 아래와 같다.
	
	1) 순환 버퍼 
	데이터를 밀어넣으면서 현재까지 사용한 영역이 어디까지인지 기록해놓고
	그 다음에 데이터를 밀어넣을때 이전까지 사용한 영역부터 이어서 데이터를
	추가하는 방식이다. 이런 식으로 계속해서 데이터를 밀어넣다보면 영역의
	끝에 도달하게 될 것인데 이럴 경우 다시 영역의 처음부터 시작해 데이터를
	밀어넣는 방식으로 동작하는 방법이다.
	
	1-1) 순환 버퍼는 복사 비용이 많이 든다는 단점이 있다. 나중에 가면 동일한 
	패킷을 굉장히 많은 Session에게 전달하는 일이 자주 발생하게 되는데 이럴
	때마다 Send 함수를 각 Session마다 호출하게 되면서 복사 비용이 굉장히
	많이 발생하게 될 수 있기 때문이다. */


private:
	// 클라 소켓, 클라 주소, 접속 상태 정보들
	SOCKET				m_Socket = INVALID_SOCKET;
	Atomic<bool>		m_Connected = { false };
	NetAddress			m_NetAdress = {};
	weak_ptr<Service>	m_Service; // 

private:
	USE_LOCK;
	/* 수신 관련 */
	RecvBuffer			m_RecvBuffer;

	/* 송신 관련 */

private:
	/* IocpEvent 재사용 */
	DisConnectEvent		m_DisConnectEvent;
	ConnectEvent		m_ConnectEvent;
	RecvEvent			m_RecvEvenet;

};