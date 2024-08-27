#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"

// Listener�� ���������� IOCP�ھ ����� ����̴�

class Session : public IocpObject
{
public:
	Session();
	virtual ~Session();

public:
	/* ���� ���� */
	void		SetNetAddress(NetAddress _NetAddress) { m_NetAdress = _NetAddress; }
	NetAddress  GetNetAddress() { return m_NetAdress; }

	SOCKET		GetSocket() { return m_Socket; }

public:
	/* �������̽� ���� */

	virtual HANDLE	 GetHandle() override;
	// � �ϰ����� IocpEvent�� �޾Ƽ� Dispatch �Լ����� ó���Ѵ�
	virtual void	 Dispatch(class IocpEvent* _pIocpEvent, int32 _iNumOfBytes = 0) override;

public:
	// TODO Temp
	char m_chRecvBuffer[1000];


private:
	// Ŭ�� ����, Ŭ�� �ּ�, ���� ���� ������
	SOCKET m_Socket = INVALID_SOCKET;
	NetAddress m_NetAdress = {};
	Atomic<bool> m_Connected = { false };


};