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
	/* _eType이 Server인 경우 _pAddress는 자기자신의 주소이고 
	Client라면 연결할 대상의 주소를 의미할 것이다 
	
	또한, IocpCore를 하나만 만들고 공유하면서 사용하거나 경우에 따라 
	IocpCore를 분리해서	Queue를 여러개 사용할 수도 있을 것이다.
	여기서 Queue가 뭐를 말하는지 모르겠음. APC 큐를 말하는건가 
	
	_Factory는 Session을 생성해주는 함수이고 _iMaxSessionCnt는 
	이름 그대로 최대 세션(동접자 수)를 받아주는 것이다 */
	Service(ServiceType _eType, NetAddress _pTargetAddress, IocpCoreRef _pCore,
			SessionFactory _Factory, int32 _iMaxSessionCnt = 1);

	virtual ~Service();

	// 
	virtual bool Start() abstract;
	bool		 CanStart() { return (m_SessionFactory != nullptr); }

	virtual void CloseService();
	void		 SetSessionFactory(SessionFactory _Factory) { m_SessionFactory = _Factory; }

	// Session을 생성함과 동시에 IocpCore에 등록하는 역할을 담당
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

	// TEMP 클라이언트 서비스를 시작하면 상대방에게 연결
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
	// 서버 역할을 하기 위해서 Listener관리
	ListenerRef		m_pListener;



};
