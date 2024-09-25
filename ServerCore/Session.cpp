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
	/* 1) 버퍼 관리
	   2) SendEvent를 어떻게 관리할지, 단일로 할지 여러 개로 할지 그리고
	WSASend는 중첩해서 호출할 수 있을지 등등의 문제를 생각해야 한다 
	
	무슨 말이냐면 예를 들어서 MMORPG 게임을 만들고 있다고 가정했을때 
	플레이어가 필드에서 몬스터를 처치해서 몬스터의 정보가 클라에 보내지고 
	있는 상황에서 동시에 친구에게서 귓속말이 오면 해당 귓속말에 대한 정보도
	플레이어에게 보내줘야 되는데 이렇게 되면 Send를 여러 번 호출하게 되고
	플레이어는 데이터를 동시다발적으로 받아줘야 하는데 만약 SendEvent를 
	하나만 만들어서 사용하고 있었다면 이러한 상황에서 보내온 데이터를 
	모두 처리할 수가 없을 것이다 */

	// TODO TEMP 일단 임시로 SendEvent를 실시간으로 생성해서 처리할 것이다
	SendEvent* pSendEvent = xnew<SendEvent>();
	// ADD REF COUNT
	pSendEvent->m_pOwner = shared_from_this();
	pSendEvent->m_vecBuffer.resize(_iLen);

	/* vector의 data 함수는 vector가 소유한 요소들을 저장하는 데 내부적으로 
	사용하는 메모리 배열에 대한 직접 포인터(direct pointer)를 반환한다 */
	memcpy(pSendEvent->m_vecBuffer.data(), _pBuffer, _iLen);

	RegisterSend(pSendEvent);
}

bool Session::Connect()
{
	return RegisterConnect();
}

void Session::DisConnect(const WCHAR* _Cause)
{
	/* false를 넣어주면서 기존의 값을 반환하는데 기존 값이 false였다면
	바로 return을 해주고 기존 값이 true 였다면 사유를 출력해준다.
	밑의 로직을 통해서 한 번만 호출이 되도록 해준다 */
	if (m_Connected.exchange(false) == false)
		return;

	// TEMP
	wcout << "DisConnect : " << _Cause << '\n';

	// 컨텐츠 코드에서 오버라이딩
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

	/* BindAnyAddress 함수를 통해 소켓에 아무 IP주소/포트를 연결해야한다.
	연결하지 않을 경우 ConnectEx에서 에러가 발생한다. 참고로 Prot 값을 
	0으로 넘겨주면 자동으로 여유있는 포트에 연결한다 */
	if (SocketUtils::BindAnyAddress(m_Socket, 0) == false)
		return false;


	DWORD NumOfBytes = { 0 };
	DWORD Flags = { 0 };

	/* 참고로 만약 서비스타입이 Client라면 GetNetAddress함수로 가지고 
	온 주소는 상대방을 의미한다. 즉, 붙어야 하는 서버 주소를 말한다	*/
	SOCKADDR_IN SockAddr = GetService()->GetNetAddress().GetSockAddr();

	// ConnectEx 호출
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

	/* 세 번째 인자값으로 넘겨주는 Flag값이 중요하다. 밑에 로직에서 넘겨주고 
	있는 TF_REUSE_SOCKET은 이름 그대로 소켓을 다시 재사용하겠다는 의미인데
	만약 해당 옵션값을 넘겨준 DisConnectEx 함수가 성공적으로 호출이 완료가
	될 경우 해당 소켓을 AcceptEx, ConnectEx 함수에 또 다시 넘겨즐 수 있다는
	특징이 있다. 더 자세히 알고 싶으면 문서를 찾아보면 된다 */
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
	// 연결이 끊겼는지 확인
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

			/* 에러 코드가 펜딩이 아니라면 IocpCore에 작업이 완료되었다는
			통지를 보낼 수가 없기 때문에 해당 세션을 삭제해준다 */
		}
	}

}

void Session::RegisterSend(SendEvent* _pSendEvent)
{
	// 연결이 끊겼는지 확인
	if (!IsConnected())
		return;

	WSABUF wsaBuf;
	wsaBuf.buf = (char*)_pSendEvent->m_vecBuffer.data();
	wsaBuf.len = (ULONG)_pSendEvent->m_vecBuffer.size();
	
	DWORD NumOfBytes = { 0 };

	/* IOCP에 관찰 대상으로 등록을 해놓았기 때문에 작업이
	완료되면 PrecessSend 함수가 호출이 된다 */
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
	/* ProcessConnect 함수는 사용 빈도가 적지만 만약에 세션이 사용하고 있는 
	서비스가 클라이언트 서비스 타입이라면 상대방에게 먼저 요청을 걸어서 
	붙어야 하는 경우에 해당 함수를 사용하게 될 것이다. 그러니까 결국 세션이 
	클라이언트 입장에서 다른 서버에 연결할때도 ProcessConnect 함수를 사용하고 
	반대로 현재 세션이 서버이고 다른 클라이언트가 연결을 요청할때도 이 함수를
	사용할 것이다 */

	// RELEASE REF COUNT
	m_ConnectEvent.m_pOwner = nullptr;

	// 연결 완료
	m_Connected.store(true);

	// 세션 등록
	GetService()->AddSession(GetSessionRef());
	
	// 컨텐츠 코드에서 오버라이딩
	OnConnected();

	/* 연결하고 있는 상대방이 데이터를 보냈을때 데이터를 
	받을 수 있도록 수신 등록을 해준다 */
	RegisterRecv();
}

void Session::ProcessDisConnect()
{
	m_DisConnectEvent.m_pOwner = nullptr;
	 
}

void Session::ProcessRecv(int32 _iNumOfBytes)
{
	
	// 더 이상 예약(RegisterRecv) 해놓은 Recv가 없기 때문에 RELEASE REF COUNT
	m_RecvEvenet.m_pOwner = nullptr;

	if (_iNumOfBytes == 0)
	{
		// 받아온 _iNumOfBytes 데이터가 0인 경우 무조건 연결이 끊긴 상황이다
		DisConnect(L"Recv 0");
		return;
	}

	/* 성공적으로 버퍼에 데이터가 복사가 되었다면 OnWrite 
	함수를 호출하여 _iNumOfBytes 만큼 이동시킬 것이다	*/
	if (m_RecvBuffer.OnWrite(_iNumOfBytes) == false)
	{
		DisConnect(L"OnWrite OverFlow");
		return;
	}

	/* m_RecvBuffer의 DataSize는 이전에 보냈던 데이터와 현재 보낸 
	데이터를 포함한 크기를 의미한다. 즉, 누적된 데이터의 크기다 */
	int32 iDataSize = m_RecvBuffer.DataSize();

	/* ReadPos부터 시작해서 DataSize만큼의 범위가 유효한 데이터의 영역 */

	/* 실질적으로 처리된 데이터의 크기를 받아준다. 이렇게 받아준 값을 
	사용해서 데이터를 읽은 만큼 ReadPos 위치를 이동시킨다 */
	int32 iProcessLen = OnRecv(m_RecvBuffer.ReadPos(), iDataSize);
	
	if (iProcessLen < 0 || iDataSize < iProcessLen || m_RecvBuffer.OnRead(iProcessLen) == false)
	{
		DisConnect(L"OnRead OverFlow");
		return;
	}

	m_RecvBuffer.Clean();

	// 수신 등록
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

	// 컨텐츠 코드에서 오버라이딩
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
		
		로그를 콘솔에 출력하는 것도 어느 정도 컨텍스트 스위칭이 발생하기 
		때문에 로그만 출력하는 쓰레드를 따로 만들어서 처리하기도 한다 */
		cout << "Handle Error : " << _iErrorCode << '\n';
		break;
	}

}
