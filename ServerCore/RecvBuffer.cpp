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
	/* 참고로 대부분의 경우 해당 조건문에 들어오게 되겠지만 간혹 데이터가
	중간에 소실되서 저장이 되었을 경우 모든 데이터를 처리하지 못해 배열에
	데이터 파편이 남게 될 것이다. 이렇게 되면 매번 데이터를 복사해서 시작
	위치로 옮겨주는 작업이 추가되면서 복사 비용이 발생하게 될텐데 이 문제를
	간단하기 위해 애초에 배열의 크기를 크게 설정한 것이다. 크기를 크게 잡으면
	배열의 끝에 도달하기 전에 모든 데이터를 처리하여 DataSize가 0이 됨으로써
	복사하는 비용이 줄어들 것이다 */

	int32 iDataSize = DataSize();

	if (iDataSize == 0)
	{
		/* DataSize가 0인 경우는 Read, Write 인덱스가 동일한 상황이며 이럴 
		경우 더 이상 읽어들일 데이터가 없기 때문에 둘 다 초기화 해준다 */
		m_iReadPos = m_iWritePos = 0;
	}
	else
	{
		/* 데이터를 저장할 수 있는 여유 공간이 버퍼 1개의 
		크기보다 작다면 데이터를 시작 위치로 옮겨준다 */
		if (FreeSize() < m_iBufferSize)
		{
			// ReadPos 위치에 저장중인 데이터를 0번 위치로 옮긴다
			memcpy(&m_vecBuffer[0], &m_vecBuffer[m_iReadPos], iDataSize);
			m_iReadPos = 0;
			m_iWritePos = iDataSize;
		}
	}
}

bool RecvBuffer::OnRead(int32 _iNumOfBytes)
{
	/* _iNumOfBytes 값이 DataSize보다 큰 경우 유효한 데이터를 
	초과했다는 의미가 되기 때문에 false를 반환한다 */
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
