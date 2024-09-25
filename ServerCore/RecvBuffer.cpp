#include "pch.h"
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(int32 _iBufferSize)
	: m_iBufferSize(_iBufferSize)
{
	m_iCapacity = _iBufferSize * BUFFER_COUNT;
	m_vecBuffer.resize(m_iCapacity);

}

RecvBuffer::~RecvBuffer()
{

}

void RecvBuffer::Clean()
{
	/* ����� ��κ��� ��� �ش� ���ǹ��� ������ �ǰ����� ��Ȥ �����Ͱ�
	�߰��� �ҽǵǼ� ������ �Ǿ��� ��� ��� �����͸� ó������ ���� �迭��
	������ ������ ���� �� ���̴�. �̷��� �Ǹ� �Ź� �����͸� �����ؼ� ����
	��ġ�� �Ű��ִ� �۾��� �߰��Ǹ鼭 ���� ����� �߻��ϰ� ���ٵ� �� ������
	�����ϱ� ���� ���ʿ� �迭�� ũ�⸦ ũ�� ������ ���̴�. ũ�⸦ ũ�� ������
	�迭�� ���� �����ϱ� ���� ��� �����͸� ó���Ͽ� DataSize�� 0�� �����ν�
	�����ϴ� ����� �پ�� ���̴� */

	int32 iDataSize = DataSize();

	if (iDataSize == 0)
	{
		/* DataSize�� 0�� ���� Read, Write �ε����� ������ ��Ȳ�̸� �̷� 
		��� �� �̻� �о���� �����Ͱ� ���� ������ �� �� �ʱ�ȭ ���ش� */
		m_iReadPos = m_iWritePos = 0;
	}
	else
	{
		/* �����͸� ������ �� �ִ� ���� ������ ���� 1���� 
		ũ�⺸�� �۴ٸ� �����͸� ���� ��ġ�� �Ű��ش� */
		if (FreeSize() < m_iBufferSize)
		{
			// ReadPos ��ġ�� �������� �����͸� 0�� ��ġ�� �ű��
			memcpy(&m_vecBuffer[0], &m_vecBuffer[m_iReadPos], iDataSize);
			m_iReadPos = 0;
			m_iWritePos = iDataSize;
		}
	}
}

bool RecvBuffer::OnRead(int32 _iNumOfBytes)
{
	/* _iNumOfBytes ���� DataSize���� ū ��� ��ȿ�� �����͸� 
	�ʰ��ߴٴ� �ǹ̰� �Ǳ� ������ false�� ��ȯ�Ѵ� */
	if (_iNumOfBytes > DataSize())
		return false;

	m_iReadPos += _iNumOfBytes;

	return true;
}

bool RecvBuffer::OnWrite(int32 _iNumOfBytes)
{
	if (_iNumOfBytes > FreeSize())
		return false;

	m_iWritePos += _iNumOfBytes;

	return true;
}
