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
	// �ܺο��� ���
	bool StartAccept(NetAddress _NetAddress);
	void CloseSocket();

public:
	// �������̽� ����
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* _pIocpEvent, int32 _iNumOfBytes = 0) override;

private:
	// ���� ����
	void RegisterAccept(AcceptEvent* _pAcceptEvent); 
	/* ����� �۾��� �Ϸᰡ �Ǿ����� �ش� �۾��� ������ ���Ͽ� ���� ������ 
	������ �޾ƿͼ�	�̿� ���õ� ó���� ProcessAccept �Լ����� �� ���̴�*/
	void ProcessAccept(AcceptEvent* _pAcceptEvent);  

protected:
	SOCKET m_Socket = INVALID_SOCKET;
	Vector<AcceptEvent*> m_vecAcceptEvents;

};

