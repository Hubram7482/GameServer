#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"

Session::Session()
{
	m_Socket = SocketUtils::CreateSocket();

}

Session::~Session()
{
	SocketUtils::Close(m_Socket);

}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(m_Socket);
}

void Session::Dispatch(IocpEvent* _pIocpEvent, int32 _iNumOfBytes)
{
	// TODO


}
