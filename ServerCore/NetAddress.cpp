#include "pch.h"
#include "NetAddress.h"

NetAddress::NetAddress(SOCKADDR_IN sockAddr) : m_SockAddr(sockAddr)
{

}

NetAddress::NetAddress(wstring ip, uint16 port)
{
	memset(&m_SockAddr, 0, sizeof(m_SockAddr));

	m_SockAddr.sin_family = AF_INET;
	m_SockAddr.sin_addr = Ip2Address(ip.c_str());
	m_SockAddr.sin_port = htons(port);
}

wstring NetAddress::GetIPAddress()
{
	WCHAR Buffer[100];

	InetNtopW(AF_INET, &m_SockAddr, Buffer, len32(Buffer));
	return wstring(Buffer);
}

IN_ADDR NetAddress::Ip2Address(const WCHAR* ip)
{
	IN_ADDR address;
	InetPtonW(AF_INET, ip, &address);

	return address;
}
