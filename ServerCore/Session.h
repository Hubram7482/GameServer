#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"
#include "RecvBuffer.h"

class Service;

// Listener�� ���������� IOCP�ھ ����� ����̴�
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
		BUFFER_SIZE = 0x10000, // 64KB(�⺻������ ���� ������ ���̶�� �Ѵ�)
	};

public:
	/* �ܺο��� ��� */
	void				Send(BYTE* _pBuffer, int32 _iLen);
	bool				Connect();

	// ������ �����ϰ� �� ������ ���ڰ����� �޴´�
	void				DisConnect(const WCHAR* _Cause);
	shared_ptr<Service> GetService() { return m_Service.lock(); }
	void				SetService(shared_ptr<Service> _Service) { m_Service = _Service; }

public:
	/* ���� ���� */
	void				SetNetAddress(NetAddress _NetAddress) { m_NetAdress = _NetAddress; }
	NetAddress			GetNetAddress() { return m_NetAdress; }

	SOCKET				GetSocket() { return m_Socket; }
	bool				IsConnected() { return m_Connected; }
	// ����� static_pointer_cast�� ����Ʈ �����͸� ����ȯ�ϱ� ���� �����
	SessionRef			GetSessionRef() { return static_pointer_cast<Session>(shared_from_this()); }

private:
	/* �������̽� ���� */
	virtual HANDLE		GetHandle() override;
	// � �ϰ����� IocpEvent�� �޾Ƽ� Dispatch �Լ����� ó���Ѵ�
	virtual void		Dispatch(class IocpEvent* _pIocpEvent, int32 _iNumOfBytes = 0) override;

private:
	/* ���� ���� */
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
	/* ������ �ڵ忡�� �����ε� */
	virtual void		OnConnected() { };
	virtual int32		OnRecv(BYTE* _pBuffer, int32 _iLen) { return _iLen; }
	virtual void		OnSend(int32 _iLen) { };
	virtual void		OnDisConnected() { };

public:
	/* Send �Լ��� ���� ������ �����͸� SendBuffer�� �����ؾ� �ϴµ� ������
	����ؼ� �����͸� �о������ ������ �ִ� ������ ��� ���� ����.
	
	�����͸� �о�ִ� �� ���� ����� �Ʒ��� ����.
	
	1) ��ȯ ���� 
	�����͸� �о�����鼭 ������� ����� ������ ���������� ����س���
	�� ������ �����͸� �о������ �������� ����� �������� �̾ �����͸�
	�߰��ϴ� ����̴�. �̷� ������ ����ؼ� �����͸� �о�ִٺ��� ������
	���� �����ϰ� �� ���ε� �̷� ��� �ٽ� ������ ó������ ������ �����͸�
	�о�ִ� ������� �����ϴ� ����̴�.
	
	1-1) ��ȯ ���۴� ���� ����� ���� ��ٴ� ������ �ִ�. ���߿� ���� ������ 
	��Ŷ�� ������ ���� Session���� �����ϴ� ���� ���� �߻��ϰ� �Ǵµ� �̷�
	������ Send �Լ��� �� Session���� ȣ���ϰ� �Ǹ鼭 ���� ����� ������
	���� �߻��ϰ� �� �� �ֱ� �����̴�. */


private:
	// Ŭ�� ����, Ŭ�� �ּ�, ���� ���� ������
	SOCKET				m_Socket = INVALID_SOCKET;
	Atomic<bool>		m_Connected = { false };
	NetAddress			m_NetAdress = {};
	weak_ptr<Service>	m_Service; // 

private:
	USE_LOCK;
	/* ���� ���� */
	RecvBuffer			m_RecvBuffer;

	/* �۽� ���� */

private:
	/* IocpEvent ���� */
	DisConnectEvent		m_DisConnectEvent;
	ConnectEvent		m_ConnectEvent;
	RecvEvent			m_RecvEvenet;

};