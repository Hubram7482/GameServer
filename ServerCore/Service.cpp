#include "pch.h"
#include "Service.h"
#include "Session.h"

Service::Service(ServiceType _eType, NetAddress _pTargetAddress, IocpCoreRef _pCore, SessionFactory _Factory, int32 _iMaxSessionCnt)
	: m_eType(_eType), m_pNetAddress(_pTargetAddress), m_pIocpCore(_pCore), m_SessionFactory(_Factory), m_iMaxSessionCnt(_iMaxSessionCnt)
{

}

Service::~Service()
{

}

void Service::CloseService()
{
	// TODO 

}

SessionRef Service::CreateSession()
{
	SessionRef pSession = m_SessionFactory();

	if (m_pIocpCore->Register(pSession) == false)
	{
		return nullptr;
	}

	return pSession;
}

void Service::AddSession(SessionRef _pSessionRef)
{
	// Lock걸고 세션 추가 
	WRITE_LOCK;

	m_iSessionCnt++;
	m_Sessions.insert(_pSessionRef);
}

void Service::ReleaseSession(SessionRef _pSessionRef)
{
	WRITE_LOCK;
	
	ASSERT_CRASH(m_Sessions.erase(_pSessionRef) != 0);
	m_iSessionCnt--;
}

/*---------------------
	 ClientService
-----------------------*/

ClientService::ClientService(NetAddress _pTargetAddress, IocpCoreRef _pIocpCore, SessionFactory _Factory, int32 _iMaxSessionCnt)
	: Service(ServiceType::Client, _pTargetAddress, _pIocpCore, _Factory, _iMaxSessionCnt)
{

}

ClientService::~ClientService()
{

}

bool ClientService::Start()
{
	// TODO

	return true;
}

/*---------------------
	 ServerService
-----------------------*/

ServerService::ServerService(NetAddress _pAddress, IocpCoreRef _pIocpCore, SessionFactory _Factory, int32 _iMaxSessionCnt)
	: Service(ServiceType::Server, _pAddress, _pIocpCore, _Factory, _iMaxSessionCnt)
{

}

ServerService::~ServerService()
{

}

bool ServerService::Start()
{
	// TODO
	if (CanStart() == false)
	{
		return false;
	}

	m_pListener = MakeShared<Listener>();
	if (m_pListener == nullptr)
		return false;

	ServerServiceRef pService = static_pointer_cast<ServerService>(shared_from_this());
	if (m_pListener->StartAccept(pService) == false)
		return false;


	return true;
}

void ServerService::CloseService()
{
	// TODO

	Service::CloseService();
}
