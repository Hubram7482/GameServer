#pragma once

class RecvBuffer
{
	/* Read, Write �ε����� ��ġ�� ��Ȳ�� ���� ����ϰ� �߻��� �� �ֵ���
	�����Ϸ��� ���� ũ�⺸�� 10�� ũ�� ������ ���̴� */
	enum { BUFFER_COUNT = 10 };

public:
	RecvBuffer(int32 _iBufferSize);
	~RecvBuffer();


	void		Clean();
	// Read, Write Pos�� ���������� �����ϴ� �Լ�
	bool		OnRead(int32 _iNumOfBytes);
	bool		OnWrite(int32 _iNumOfBytes);

	BYTE*		ReadPos() { return &m_vecBuffer[m_iReadPos]; }
	BYTE*		WritePos() { return &m_vecBuffer[m_iWritePos]; }
	int32		DataSize() { return (m_iWritePos - m_iReadPos); }
	int32		FreeSize() { return (m_iCapacity - m_iWritePos); }

private:
	int32			m_iCapacity = { 0 };
	int32			m_iBufferSize = { 0 };
	// �迭�� �ε����� �����ϱ� ���� ����
	int32			m_iReadPos = { 0 };
	int32			m_iWritePos = { 0 };

	vector<BYTE>	m_vecBuffer;
};

