#pragma once

/* NetAddress 클래스의 사용 용도는 다음과 같다
SOCKADDR_IN을 통해서 주소를 받아주고 있는데 나중에 접속한 클라이언트의
주소 IP와 같은 정보를 추출하고 싶은 상황이 생길 수 있는데 이런 경우가
발생할 때마다 함수를 호출해서 정보를 추출하기 보다는 모든 주소를 해당
NetAddress 클래스로 랩핑해서 사용하여 편하게 설정하고 원하는 정보를
추출할 수 있도록 유도해주는 역할이다 */
class NetAddress
{
public:
	NetAddress() = default;
	NetAddress(SOCKADDR_IN sockAddr);
	NetAddress(wstring ip, uint16 port);

	// SockAddr, IP 주소, Port 반환 함수
	SOCKADDR_IN& GetSockAddr() { return m_SockAddr; }
	wstring		 GetIPAddress();
	uint16		 GetPort() { return ntohs(m_SockAddr.sin_port); }

public:
	/* IP 주소를 이용해서 IN_ADDR로 반환하는 함수 */
	static IN_ADDR Ip2Address(const WCHAR* ip);

private:
	SOCKADDR_IN m_SockAddr = {};

};

