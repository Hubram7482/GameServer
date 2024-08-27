#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"

// Listener와 마찬가지로 IOCP코어에 등록할 대상이다

class Session : public IocpObject
{
public:
	Session();
	virtual ~Session();

public:
	/* 정보 관련 */
	void		SetNetAddress(NetAddress _NetAddress) { m_NetAdress = _NetAddress; }
	NetAddress  GetNetAddress() { return m_NetAdress; }

	SOCKET		GetSocket() { return m_Socket; }

public:
	/* 인터페이스 구현 */

	virtual HANDLE	 GetHandle() override;
	// 어떤 일감인지 IocpEvent로 받아서 Dispatch 함수에서 처리한다
	virtual void	 Dispatch(class IocpEvent* _pIocpEvent, int32 _iNumOfBytes = 0) override;

public:
	// TODO Temp
	char m_chRecvBuffer[1000];


private:
	// 클라 소켓, 클라 주소, 접속 상태 정보들
	SOCKET m_Socket = INVALID_SOCKET;
	NetAddress m_NetAdress = {};
	Atomic<bool> m_Connected = { false };


};