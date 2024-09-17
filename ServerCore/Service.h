#pragma once
#include "NetAddress.h"
#include "IocpCore.h"
#include "Listener.h"
#include <functional>

enum class ServiceType : uint8
{
	Server,
	Client
};

using SessionFactory = function<SessionRef(void)>;

class Service : public enable_shared_from_this<Service>
{
public:
	/* _eType�� Server�� ��� _pAddress�� �ڱ��ڽ��� �ּ��̰� 
	Client��� ������ ����� �ּҸ� �ǹ��� ���̴� 
	
	����, IocpCore�� �ϳ��� ����� �����ϸ鼭 ����ϰų� ��쿡 ���� 
	IocpCore�� �и��ؼ�	Queue�� ������ ����� ���� ���� ���̴�.
	���⼭ Queue�� ���� ���ϴ��� �𸣰���. APC ť�� ���ϴ°ǰ� 
	
	_Factory�� Session�� �������ִ� �Լ��̰� _iMaxSessionCnt�� 
	�̸� �״�� �ִ� ����(������ ��)�� �޾��ִ� ���̴� */
	Service(ServiceType _eType, NetAddress _pTargetAddress, IocpCoreRef _pCore,
			SessionFactory _Factory, int32 _iMaxSessionCnt = 1);

	virtual ~Service();

	// 
	virtual bool Start() abstract;
	bool		 CanStart() { return (m_SessionFactory != nullptr); }

	virtual void CloseService();
	void		 SetSessionFactory(SessionFactory _Factory) { m_SessionFactory = _Factory; }

	// Session�� �����԰� ���ÿ� IocpCore�� ����ϴ� ������ ���
	SessionRef	 CreateSession();
	void		 AddSession(SessionRef _pSessionRef);
	void		 ReleaseSession(SessionRef _pSessionRef);
	int32		 GetCurrentSessionCnt() { return m_iSessionCnt; }
	int32		 GetMaxSessionCnt() { return m_iMaxSessionCnt; }

public:
	// Temp
	ServiceType	 GetServiceType() { return m_eType; }
	NetAddress	 GetNetAddress() { return m_pNetAddress; }
	IocpCoreRef& GetIocpCore() { return m_pIocpCore; }

protected:
	USE_LOCK;

	ServiceType			m_eType;
	NetAddress			m_pNetAddress = { };
	IocpCoreRef			m_pIocpCore; 

	Set<SessionRef>		m_Sessions;
	SessionFactory		m_SessionFactory;
	
	int32			    m_iSessionCnt = { 0 };
	int32			    m_iMaxSessionCnt = { 0 };

};

/*---------------------
	 ClientService
-----------------------*/

class ClientService : public Service
{
public:
	ClientService(NetAddress _pTargetAddress, IocpCoreRef _pIocpCore, SessionFactory _Factory, int32 _iMaxSessionCnt = 1);
	virtual ~ClientService();

	// TEMP Ŭ���̾�Ʈ ���񽺸� �����ϸ� ���濡�� ����
	virtual bool Start() override;

};

/*---------------------
	 ServerService
-----------------------*/

class ServerService : public Service
{
public:
	ServerService(NetAddress _pAddress, IocpCoreRef _pIocpCore, SessionFactory _Factory, int32 _iMaxSessionCnt = 1);
	virtual ~ServerService();


	virtual bool	Start() override;
	virtual void	CloseService();


private:
	// ���� ������ �ϱ� ���ؼ� Listener����
	ListenerRef		m_pListener;



};
