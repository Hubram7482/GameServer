#pragma once

/* NetAddress Ŭ������ ��� �뵵�� ������ ����
SOCKADDR_IN�� ���ؼ� �ּҸ� �޾��ְ� �ִµ� ���߿� ������ Ŭ���̾�Ʈ��
�ּ� IP�� ���� ������ �����ϰ� ���� ��Ȳ�� ���� �� �ִµ� �̷� ��찡
�߻��� ������ �Լ��� ȣ���ؼ� ������ �����ϱ� ���ٴ� ��� �ּҸ� �ش�
NetAddress Ŭ������ �����ؼ� ����Ͽ� ���ϰ� �����ϰ� ���ϴ� ������
������ �� �ֵ��� �������ִ� �����̴� */
class NetAddress
{
public:
	NetAddress() = default;
	NetAddress(SOCKADDR_IN sockAddr);
	NetAddress(wstring ip, uint16 port);

	// SockAddr, IP �ּ�, Port ��ȯ �Լ�
	SOCKADDR_IN& GetSockAddr() { return m_SockAddr; }
	wstring		 GetIPAddress();
	uint16		 GetPort() { return ntohs(m_SockAddr.sin_port); }

public:
	/* IP �ּҸ� �̿��ؼ� IN_ADDR�� ��ȯ�ϴ� �Լ� */
	static IN_ADDR Ip2Address(const WCHAR* ip);

private:
	SOCKADDR_IN m_SockAddr = {};

};

