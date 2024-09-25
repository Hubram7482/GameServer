#include "pch.h"
#include "Session.h"
#include "SocketUtils.h"
#include "Service.h"

Session::Session() : m_RecvBuffer(BUFFER_SIZE)
{
	m_Socket = SocketUtils::CreateSocket();

}

Session::~Session()
{
	SocketUtils::Close(m_Socket);

}

void Session::Send(BYTE* _pBuffer, int32 _iLen)
{
	/* 1) ���� ����
	   2) SendEvent�� ��� ��������, ���Ϸ� ���� ���� ���� ���� �׸���
	WSASend�� ��ø�ؼ� ȣ���� �� ������ ����� ������ �����ؾ� �Ѵ� 
	
	���� ���̳ĸ� ���� �� MMORPG ������ ����� �ִٰ� ���������� 
	�÷��̾ �ʵ忡�� ���͸� óġ�ؼ� ������ ������ Ŭ�� �������� 
	�ִ� ��Ȳ���� ���ÿ� ģ�����Լ� �ӼӸ��� ���� �ش� �ӼӸ��� ���� ������
	�÷��̾�� ������� �Ǵµ� �̷��� �Ǹ� Send�� ���� �� ȣ���ϰ� �ǰ�
	�÷��̾�� �����͸� ���ôٹ������� �޾���� �ϴµ� ���� SendEvent�� 
	�ϳ��� ���� ����ϰ� �־��ٸ� �̷��� ��Ȳ���� ������ �����͸� 
	��� ó���� ���� ���� ���̴� */

	// TODO TEMP �ϴ� �ӽ÷� SendEvent�� �ǽð����� �����ؼ� ó���� ���̴�
	SendEvent* pSendEvent = xnew<SendEvent>();
	// ADD REF COUNT
	pSendEvent->m_pOwner = shared_from_this();
	pSendEvent->m_vecBuffer.resize(_iLen);

	/* vector�� data �Լ��� vector�� ������ ��ҵ��� �����ϴ� �� ���������� 
	����ϴ� �޸� �迭�� ���� ���� ������(direct pointer)�� ��ȯ�Ѵ� */
	memcpy(pSendEvent->m_vecBuffer.data(), _pBuffer, _iLen);

	RegisterSend(pSendEvent);
}

bool Session::Connect()
{
	return RegisterConnect();
}

void Session::DisConnect(const WCHAR* _Cause)
{
	/* false�� �־��ָ鼭 ������ ���� ��ȯ�ϴµ� ���� ���� false���ٸ�
	�ٷ� return�� ���ְ� ���� ���� true ���ٸ� ������ ������ش�.
	���� ������ ���ؼ� �� ���� ȣ���� �ǵ��� ���ش� */
	if (m_Connected.exchange(false) == false)
		return;

	// TEMP
	wcout << "DisConnect : " << _Cause << '\n';

	// ������ �ڵ忡�� �������̵�
	OnDisConnected(); 
	GetService()->ReleaseSession(GetSessionRef());

	RegisterDisConnect();
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(m_Socket);
}

void Session::Dispatch(IocpEvent* _pIocpEvent, int32 _iNumOfBytes)
{
	// TODO
	switch (_pIocpEvent->m_EventType)
	{
	case EventType::Connect:
		ProcessConnect();
		break;
	case EventType::DisConnect:
		ProcessDisConnect();
		break;
	case EventType::Recv:
		ProcessRecv(_iNumOfBytes);
		break;
	case EventType::Send:
		ProcessSend(static_cast<SendEvent*>(_pIocpEvent), _iNumOfBytes);
		break;
	default:
		break;
	}

}

bool Session::RegisterConnect()
{
	if (IsConnected())
		return false;

	if (GetService()->GetServiceType() != ServiceType::Client)
		return false;
	
	m_ConnectEvent.Init();
	// ADD REF COUNT
	m_ConnectEvent.m_pOwner = shared_from_this();

	if (SocketUtils::SetReuseAddress(m_Socket, true) == false)
		return false;

	/* BindAnyAddress �Լ��� ���� ���Ͽ� �ƹ� IP�ּ�/��Ʈ�� �����ؾ��Ѵ�.
	�������� ���� ��� ConnectEx���� ������ �߻��Ѵ�. ����� Prot ���� 
	0���� �Ѱ��ָ� �ڵ����� �����ִ� ��Ʈ�� �����Ѵ� */
	if (SocketUtils::BindAnyAddress(m_Socket, 0) == false)
		return false;


	DWORD NumOfBytes = { 0 };
	DWORD Flags = { 0 };

	/* ����� ���� ����Ÿ���� Client��� GetNetAddress�Լ��� ������ 
	�� �ּҴ� ������ �ǹ��Ѵ�. ��, �پ�� �ϴ� ���� �ּҸ� ���Ѵ�	*/
	SOCKADDR_IN SockAddr = GetService()->GetNetAddress().GetSockAddr();

	// ConnectEx ȣ��
	if (false == SocketUtils::ConnectEx(m_Socket, reinterpret_cast<SOCKADDR*>(&SockAddr),
		sizeof(SockAddr), nullptr, 0, &NumOfBytes, &m_ConnectEvent))
	{
		int32 iErrorCode = WSAGetLastError();
		if (iErrorCode != WSA_IO_PENDING)
		{
			HandleError(iErrorCode);
			// RELEASE REF COUNT
			m_ConnectEvent.m_pOwner = nullptr;
			return false;
		}
	}

	return true;

	// WSAConnect();
}

bool Session::RegisterDisConnect()
{
	m_DisConnectEvent.Init();
	m_DisConnectEvent.m_pOwner = shared_from_this();

	DWORD Flags = { 0 };

	/* �� ��° ���ڰ����� �Ѱ��ִ� Flag���� �߿��ϴ�. �ؿ� �������� �Ѱ��ְ� 
	�ִ� TF_REUSE_SOCKET�� �̸� �״�� ������ �ٽ� �����ϰڴٴ� �ǹ��ε�
	���� �ش� �ɼǰ��� �Ѱ��� DisConnectEx �Լ��� ���������� ȣ���� �Ϸᰡ
	�� ��� �ش� ������ AcceptEx, ConnectEx �Լ��� �� �ٽ� �Ѱ��� �� �ִٴ�
	Ư¡�� �ִ�. �� �ڼ��� �˰� ������ ������ ã�ƺ��� �ȴ� */
	if (false == SocketUtils::DisConnectEx(m_Socket, &m_DisConnectEvent, 
		TF_REUSE_SOCKET, 0))
	{
		int32 iErrorCode = WSAGetLastError();

		if (iErrorCode != WSA_IO_PENDING)
		{
			// RELEASE REF COUNT
			m_DisConnectEvent.m_pOwner = nullptr;
			HandleError(iErrorCode);
			return false;
		}
	}

	return true;
}

void Session::RegisterRecv()
{
	// ������ ������� Ȯ��
	if (!IsConnected())
		return;

	m_RecvEvenet.Init();
	// ADD REF COUNT
	m_RecvEvenet.m_pOwner = shared_from_this();


	WSABUF WsaBuf;
	WsaBuf.buf = reinterpret_cast<char*>(m_RecvBuffer.WritePos());
	WsaBuf.len = m_RecvBuffer.FreeSize();

	DWORD  NumOfBytes = {0};
	DWORD  Flags = {0};


	if (SOCKET_ERROR == WSARecv(m_Socket, &WsaBuf, 1, OUT &NumOfBytes, OUT &Flags, &m_RecvEvenet, nullptr))
	{
		int32 iErrorCode = WSAGetLastError();
		if (iErrorCode != WSA_IO_PENDING)
		{
			HandleError(iErrorCode);
			// RELEASE REF COUNT
			m_RecvEvenet.m_pOwner = nullptr; 

			/* ���� �ڵ尡 ����� �ƴ϶�� IocpCore�� �۾��� �Ϸ�Ǿ��ٴ�
			������ ���� ���� ���� ������ �ش� ������ �������ش� */
		}
	}

}

void Session::RegisterSend(SendEvent* _pSendEvent)
{
	// ������ ������� Ȯ��
	if (!IsConnected())
		return;

	WSABUF wsaBuf;
	wsaBuf.buf = (char*)_pSendEvent->m_vecBuffer.data();
	wsaBuf.len = (ULONG)_pSendEvent->m_vecBuffer.size();
	
	DWORD NumOfBytes = { 0 };

	/* IOCP�� ���� ������� ����� �س��ұ� ������ �۾���
	�Ϸ�Ǹ� PrecessSend �Լ��� ȣ���� �ȴ� */
	if (SOCKET_ERROR == WSASend(m_Socket, &wsaBuf, 1, OUT & NumOfBytes, 0, _pSendEvent, nullptr))
	{
		int32 iErrorCode = WSAGetLastError();
		if (iErrorCode != WSA_IO_PENDING)
		{
			HandleError(iErrorCode);
			// RELEASE REF COUNT
			_pSendEvent->m_pOwner = nullptr;
			xdelete(_pSendEvent);
		}
	}
}

void Session::ProcessConnect()
{
	/* ProcessConnect �Լ��� ��� �󵵰� ������ ���࿡ ������ ����ϰ� �ִ� 
	���񽺰� Ŭ���̾�Ʈ ���� Ÿ���̶�� ���濡�� ���� ��û�� �ɾ 
	�پ�� �ϴ� ��쿡 �ش� �Լ��� ����ϰ� �� ���̴�. �׷��ϱ� �ᱹ ������ 
	Ŭ���̾�Ʈ ���忡�� �ٸ� ������ �����Ҷ��� ProcessConnect �Լ��� ����ϰ� 
	�ݴ�� ���� ������ �����̰� �ٸ� Ŭ���̾�Ʈ�� ������ ��û�Ҷ��� �� �Լ���
	����� ���̴� */

	// RELEASE REF COUNT
	m_ConnectEvent.m_pOwner = nullptr;

	// ���� �Ϸ�
	m_Connected.store(true);

	// ���� ���
	GetService()->AddSession(GetSessionRef());
	
	// ������ �ڵ忡�� �������̵�
	OnConnected();

	/* �����ϰ� �ִ� ������ �����͸� �������� �����͸� 
	���� �� �ֵ��� ���� ����� ���ش� */
	RegisterRecv();
}

void Session::ProcessDisConnect()
{
	m_DisConnectEvent.m_pOwner = nullptr;
	 
}

void Session::ProcessRecv(int32 _iNumOfBytes)
{
	
	// �� �̻� ����(RegisterRecv) �س��� Recv�� ���� ������ RELEASE REF COUNT
	m_RecvEvenet.m_pOwner = nullptr;

	if (_iNumOfBytes == 0)
	{
		// �޾ƿ� _iNumOfBytes �����Ͱ� 0�� ��� ������ ������ ���� ��Ȳ�̴�
		DisConnect(L"Recv 0");
		return;
	}

	/* ���������� ���ۿ� �����Ͱ� ���簡 �Ǿ��ٸ� OnWrite 
	�Լ��� ȣ���Ͽ� _iNumOfBytes ��ŭ �̵���ų ���̴�	*/
	if (m_RecvBuffer.OnWrite(_iNumOfBytes) == false)
	{
		DisConnect(L"OnWrite OverFlow");
		return;
	}

	/* m_RecvBuffer�� DataSize�� ������ ���´� �����Ϳ� ���� ���� 
	�����͸� ������ ũ�⸦ �ǹ��Ѵ�. ��, ������ �������� ũ��� */
	int32 iDataSize = m_RecvBuffer.DataSize();

	/* ReadPos���� �����ؼ� DataSize��ŭ�� ������ ��ȿ�� �������� ���� */

	/* ���������� ó���� �������� ũ�⸦ �޾��ش�. �̷��� �޾��� ���� 
	����ؼ� �����͸� ���� ��ŭ ReadPos ��ġ�� �̵���Ų�� */
	int32 iProcessLen = OnRecv(m_RecvBuffer.ReadPos(), iDataSize);
	
	if (iProcessLen < 0 || iDataSize < iProcessLen || m_RecvBuffer.OnRead(iProcessLen) == false)
	{
		DisConnect(L"OnRead OverFlow");
		return;
	}

	m_RecvBuffer.Clean();

	// ���� ���
	RegisterRecv();
}

void Session::ProcessSend(SendEvent* _pSendEvent, int32 _iNumOfBytes)
{
	// RELEASE REF COUNT
	_pSendEvent->m_pOwner = nullptr;
	xdelete(_pSendEvent);

	if (_iNumOfBytes == 0)
	{
		DisConnect(L"Send 0");
		return;
	}

	// ������ �ڵ忡�� �������̵�
	OnSend(_iNumOfBytes);
}

void Session::HandleError(int32 _iErrorCode)
{
	switch (_iErrorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		DisConnect(L"HandleError");
		break;
	default:
		/* TODO LOG
		
		�α׸� �ֿܼ� ����ϴ� �͵� ��� ���� ���ؽ�Ʈ ����Ī�� �߻��ϱ� 
		������ �α׸� ����ϴ� �����带 ���� ���� ó���ϱ⵵ �Ѵ� */
		cout << "Handle Error : " << _iErrorCode << '\n';
		break;
	}

}
