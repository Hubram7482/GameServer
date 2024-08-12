#pragma once

/*-------------------
	 IocpObject
-------------------*/

// 이전까지 사용했던 Session 구조체의 역할과 유사하다
class IocpObject
{
public:
	virtual HANDLE GetHandle() abstract;
	// 어떤 일감인지 IocpEvent로 받아서 Dispatch 함수에서 처리한다
	virtual void Dispatch(class IocpEvent* _pIocpEvent, int32 _iNumOfBytes = 0) abstract;

};

/*-------------------
	  IocpCore
-------------------*/

/* 대략적인 흐름을 쓰자면 생성자에서 Completion Port를 생성해주고
Regsiter 함수를 통해서 관찰할 소켓을 등록하고 Dispatch 함수를 
계속 호출하면서 일감이 있는지 체크하고 만약 일감이 있다면
해당 일감을 처리하는 방식으로 동작한다 

1. Completion Port 생성 
2. 관찰할 소켓(IocpObject) 등록
3. Completion Port에 등록된 관찰 대상들의 완료된 
일감(Recv, Send 등의 입출력 작업)이 있는지 확인
4. 완료된 일감이 있다면 대기 상태에서 빠져나옴과 동시에 
완료된 입출력 작업에 대한 정보를 반환한다.
5. 위 과정에서 IOCP는 대기 중인 쓰레드(Worker Thread)하나를 
활성화하여 완료된 작업을 처리한다 */
class IocpCore
{
public:
	IocpCore();
	~IocpCore();

	HANDLE GetHandle() { return m_IocpHandle; }

	// 소켓을 Iocp에 등록하는 함수
	bool Register(class IocpObject* _pIocpObject);
	// 쓰레드들이 Iocp에 일감이 있는지 확인하는 함수
	bool Dispatch(uint32 TimeOutMs = INFINITE);

private:
	HANDLE m_IocpHandle;

};

