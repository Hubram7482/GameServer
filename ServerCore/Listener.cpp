#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"
#include "Service.h"

/* 헷갈릴때는 상속 관계를 보셈 */

Listener::~Listener()
{
	SocketUtils::Close(m_Socket);

	// Listener는 소멸하는 경우가 거의 없으나 형식적으로 해줬음
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

	/* Listen Socket 또한 마찬가지로 관찰해야 하는 
	대상에 해당하기 때문에 IocpCore에 등록한다 */
	if (m_pService->GetIocpCore()->Register(shared_from_this()) == false)
		return false;

	/* 소켓 재사용 여부를 설정한다. 재사용 여부를 설정하지 않았을 경우
	간혹 주소가 겹쳐서 서버가 제대로 동작하지 않는 경우가 있다 */
	if (SocketUtils::SetReuseAddress(m_Socket, true) == false)
		return false;

	if (SocketUtils::SetLinger(m_Socket, 0, 0) == false)
		return false;

	if (SocketUtils::Bind(m_Socket, m_pService->GetNetAddress()) == false)
		return false;

	if (SocketUtils::Listen(m_Socket) == false)
		return false;

	/* 아래 로직을 보면 AcceptCnt만큼 반복문을 돌면서 관찰할 Event(완료된 입출력 작업)를
	걸어주고 있는데 이렇게 하는 이유는 나중에 동접자가 많아지거나 하는 상황이 되었을때
	일부 인원은 접속을 하지 못 하는 상황이 발생할 수도 있기 때문이다. 그래서 어느정도
	여유분을 정해서 이벤트를 걸어주는 것이다 */
	const int32 AcceptCnt = m_pService->GetMaxSessionCnt();

	for (int32 i = 0; i < AcceptCnt; ++i)
	{
		AcceptEvent* pAcceptEvent = xnew<AcceptEvent>();
		/* shared_from_this 함수를 사용하면 레퍼런스 카운트를 유지한채 
		자기자신에 대한 shared_ptr을 추출할 수 있다 */
		pAcceptEvent->m_pOwner = shared_from_this();
		// 나중에 삭제할 수 있도록 보관
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
	// TODO 임시 코드(테스트용도)
	ASSERT_CRASH(_pIocpEvent->m_EventType == EventType::Accept);
	
	AcceptEvent* pAcceptEvent = static_cast<AcceptEvent*>(_pIocpEvent);
	ProcessAccept(pAcceptEvent);
}

void Listener::RegisterAccept(AcceptEvent* _pAcceptEvent)
{
	/* ClientSocket + AcceptEvent를 받아와서 accept를 걸어준다.
	Listener가 AcceptEx를 호출한다는 것이 핵심이다. */

	/* AcceptEx()는 accept와 차이는 AcceptEx()는 미리 두개의 소켓, 
	리슨 소켓과 접속을 받을 소켓을 미리 준비해야 한다는 점이다
	그래서 해당 함수에서 Session을 생성하는거임 */

	// Register IOCP(Completion Port)
	SessionRef pSession = m_pService->CreateSession();
	
	_pAcceptEvent->Init();
	_pAcceptEvent->m_pSession = pSession;

	/* 세 번째 인자값으로 버퍼를 넘겨주는데, 왜 그런지 문서를 찾아보면
	처음에 Connection을 할때 필요한 정보를 받아주기 위한 버퍼라고 써있음.
	5, 6번째 인자값은 그냥 사용방식만 외우면 된다.
	사실상 첫 번째 두 번째 그리고 마지막 인자만 중요하다 */
	
	DWORD BytesReceived = { 0 };
	
	// AcceptEx가 무슨 역할인지 헷갈리면 생성해주는 부분 보셈
	if (false == SocketUtils::AcceptEx(m_Socket, pSession->GetSocket(), pSession->m_chRecvBuffer, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, OUT & BytesReceived, static_cast<LPOVERLAPPED>(_pAcceptEvent)))
	{
		const int32 iErrorCode = WSAGetLastError();

		if (iErrorCode != WSA_IO_PENDING)
		{
			/* 해당 함수가 호출되었다는 것은 AcceptEvent를 생성하고 해당 AcceptEvent를
			IocpCore(Completion Port)에 걸어주겠다는 의미인데 이 과정에서 Pending(대기)
			상태가 아니라는 것은 당연하게도 문제가 있다는 것이고 이렇게 될 경우 
			Completion Packet(완료된 작업 통지)가 절대 발생하지 않기 때문에 또 다시
			RegisterAccept함수를 호출해서 AcceptEx가 걸릴때까지 반복하는 것이다
			(위 설명에 문제가 있을 수도 있으니까 나중에 보고 틀리면 수정하셈) */
		}

	}

}

void Listener::ProcessAccept(AcceptEvent* _pAcceptEvent)
{
	
	/* 아래 로직처럼 작업을 완료한 소켓 등 데이터를 받아오기 
	위해 AcceptEvent에 Session을 보관해놓은거다 */
	SessionRef pSession = _pAcceptEvent->m_pSession;
	
	if (!SocketUtils::SetUpdateAcceptSocket(pSession->GetSocket(), m_Socket))
	{
		/* 또한 의도한 작업이 실패를하든 성공을하든 무조건 RegisterAccept 함수를 
		호출해줘야 한다. 이유는 당연하게도 다시 AcceptEx를 걸어주기 위해서다 */
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

	// getpeername를 통해서 NetAddress정보를 추출후 설정
	pSession->SetNetAddress(NetAddress(SockAddress));

	cout << "Client Connected" << '\n';

	/* 그리고 _pAcceptEvent를 사용하고 나서 삭제를 하는	것이 아니라 계속 
	재사용한다는 것을 알 수 있다(Accept를 걸어준다)) */
	RegisterAccept(_pAcceptEvent);
}
