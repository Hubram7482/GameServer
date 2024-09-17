#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"
#include "Service.h"

/* �򰥸����� ��� ���踦 ���� */

Listener::~Listener()
{
	SocketUtils::Close(m_Socket);

	// Listener�� �Ҹ��ϴ� ��찡 ���� ������ ���������� ������
	for (AcceptEvent* acceptEvents : m_vecAcceptEvents)
	{
		// TODO

		xdelete(acceptEvents);
	}
}

bool Listener::StartAccept(ServerServiceRef _pService)
{
	m_pService = _pService;
	if (m_pService == nullptr)
		return false;

	m_Socket = SocketUtils::CreateSocket();
	if (m_Socket == INVALID_SOCKET)
		return false;

	/* Listen Socket ���� ���������� �����ؾ� �ϴ� 
	��� �ش��ϱ� ������ IocpCore�� ����Ѵ� */
	if (m_pService->GetIocpCore()->Register(shared_from_this()) == false)
		return false;

	/* ���� ���� ���θ� �����Ѵ�. ���� ���θ� �������� �ʾ��� ���
	��Ȥ �ּҰ� ���ļ� ������ ����� �������� �ʴ� ��찡 �ִ� */
	if (SocketUtils::SetReuseAddress(m_Socket, true) == false)
		return false;

	if (SocketUtils::SetLinger(m_Socket, 0, 0) == false)
		return false;

	if (SocketUtils::Bind(m_Socket, m_pService->GetNetAddress()) == false)
		return false;

	if (SocketUtils::Listen(m_Socket) == false)
		return false;

	/* �Ʒ� ������ ���� AcceptCnt��ŭ �ݺ����� ���鼭 ������ Event(�Ϸ�� ����� �۾�)��
	�ɾ��ְ� �ִµ� �̷��� �ϴ� ������ ���߿� �����ڰ� �������ų� �ϴ� ��Ȳ�� �Ǿ�����
	�Ϻ� �ο��� ������ ���� �� �ϴ� ��Ȳ�� �߻��� ���� �ֱ� �����̴�. �׷��� �������
	�������� ���ؼ� �̺�Ʈ�� �ɾ��ִ� ���̴� */
	const int32 AcceptCnt = m_pService->GetMaxSessionCnt();

	for (int32 i = 0; i < AcceptCnt; ++i)
	{
		AcceptEvent* pAcceptEvent = xnew<AcceptEvent>();
		/* shared_from_this �Լ��� ����ϸ� ���۷��� ī��Ʈ�� ������ä 
		�ڱ��ڽſ� ���� shared_ptr�� ������ �� �ִ� */
		pAcceptEvent->m_pOwner = shared_from_this();
		// ���߿� ������ �� �ֵ��� ����
		m_vecAcceptEvents.push_back(pAcceptEvent);
		RegisterAccept(pAcceptEvent);
	}

	return true;
}

void Listener::CloseSocket()
{
	SocketUtils::Close(m_Socket);
}

HANDLE Listener::GetHandle()
{
	return reinterpret_cast<HANDLE>(m_Socket);
}

void Listener::Dispatch(IocpEvent* _pIocpEvent, int32 _iNumOfBytes)
{
	// TODO �ӽ� �ڵ�(�׽�Ʈ�뵵)
	ASSERT_CRASH(_pIocpEvent->m_EventType == EventType::Accept);
	
	AcceptEvent* pAcceptEvent = static_cast<AcceptEvent*>(_pIocpEvent);
	ProcessAccept(pAcceptEvent);
}

void Listener::RegisterAccept(AcceptEvent* _pAcceptEvent)
{
	/* ClientSocket + AcceptEvent�� �޾ƿͼ� accept�� �ɾ��ش�.
	Listener�� AcceptEx�� ȣ���Ѵٴ� ���� �ٽ��̴�. */

	/* AcceptEx()�� accept�� ���̴� AcceptEx()�� �̸� �ΰ��� ����, 
	���� ���ϰ� ������ ���� ������ �̸� �غ��ؾ� �Ѵٴ� ���̴�
	�׷��� �ش� �Լ����� Session�� �����ϴ°��� */

	// Register IOCP(Completion Port)
	SessionRef pSession = m_pService->CreateSession();
	
	_pAcceptEvent->Init();
	_pAcceptEvent->m_pSession = pSession;

	/* �� ��° ���ڰ����� ���۸� �Ѱ��ִµ�, �� �׷��� ������ ã�ƺ���
	ó���� Connection�� �Ҷ� �ʿ��� ������ �޾��ֱ� ���� ���۶�� ������.
	5, 6��° ���ڰ��� �׳� ����ĸ� �ܿ�� �ȴ�.
	��ǻ� ù ��° �� ��° �׸��� ������ ���ڸ� �߿��ϴ� */
	
	DWORD BytesReceived = { 0 };
	
	// AcceptEx�� ���� �������� �򰥸��� �������ִ� �κ� ����
	if (false == SocketUtils::AcceptEx(m_Socket, pSession->GetSocket(), pSession->m_chRecvBuffer, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, OUT & BytesReceived, static_cast<LPOVERLAPPED>(_pAcceptEvent)))
	{
		const int32 iErrorCode = WSAGetLastError();

		if (iErrorCode != WSA_IO_PENDING)
		{
			/* �ش� �Լ��� ȣ��Ǿ��ٴ� ���� AcceptEvent�� �����ϰ� �ش� AcceptEvent��
			IocpCore(Completion Port)�� �ɾ��ְڴٴ� �ǹ��ε� �� �������� Pending(���)
			���°� �ƴ϶�� ���� �翬�ϰԵ� ������ �ִٴ� ���̰� �̷��� �� ��� 
			Completion Packet(�Ϸ�� �۾� ����)�� ���� �߻����� �ʱ� ������ �� �ٽ�
			RegisterAccept�Լ��� ȣ���ؼ� AcceptEx�� �ɸ������� �ݺ��ϴ� ���̴�
			(�� ���� ������ ���� ���� �����ϱ� ���߿� ���� Ʋ���� �����ϼ�) */
		}

	}

}

void Listener::ProcessAccept(AcceptEvent* _pAcceptEvent)
{
	
	/* �Ʒ� ����ó�� �۾��� �Ϸ��� ���� �� �����͸� �޾ƿ��� 
	���� AcceptEvent�� Session�� �����س����Ŵ� */
	SessionRef pSession = _pAcceptEvent->m_pSession;
	
	if (!SocketUtils::SetUpdateAcceptSocket(pSession->GetSocket(), m_Socket))
	{
		/* ���� �ǵ��� �۾��� ���и��ϵ� �������ϵ� ������ RegisterAccept �Լ��� 
		ȣ������� �Ѵ�. ������ �翬�ϰԵ� �ٽ� AcceptEx�� �ɾ��ֱ� ���ؼ��� */
		RegisterAccept(_pAcceptEvent);
		return;
	}
	SOCKADDR_IN SockAddress;
	int32 iSizeOfSockAddress = sizeof(SockAddress);
	if (SOCKET_ERROR == getpeername(pSession->GetSocket(),
		OUT reinterpret_cast<SOCKADDR*>(&SockAddress), &iSizeOfSockAddress))
	{
		RegisterAccept(_pAcceptEvent);
		return;
	}

	// getpeername�� ���ؼ� NetAddress������ ������ ����
	pSession->SetNetAddress(NetAddress(SockAddress));

	cout << "Client Connected" << '\n';

	/* �׸��� _pAcceptEvent�� ����ϰ� ���� ������ �ϴ�	���� �ƴ϶� ��� 
	�����Ѵٴ� ���� �� �� �ִ�(Accept�� �ɾ��ش�)) */
	RegisterAccept(_pAcceptEvent);
}
