#pragma once
#include "IocpCore.h"
#include "NetAddress.h"

class AcceptEvent;

class Listener : public IocpObject
{
public:
	Listener() = default;
	~Listener();

public:
	// 외부에서 사용
	bool StartAccept(NetAddress _NetAddress);
	void CloseSocket();

public:
	// 인터페이스 구현
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* _pIocpEvent, int32 _iNumOfBytes = 0) override;

private:
	// 수신 관련
	void RegisterAccept(AcceptEvent* _pAcceptEvent); 
	/* 입출력 작업이 완료가 되었을때 해당 작업을 진행한 소켓에 대한 데이터 
	정보를 받아와서	이와 관련된 처리를 ProcessAccept 함수에서 할 것이다*/
	void ProcessAccept(AcceptEvent* _pAcceptEvent);  

protected:
	SOCKET m_Socket = INVALID_SOCKET;
	Vector<AcceptEvent*> m_vecAcceptEvents;

};

