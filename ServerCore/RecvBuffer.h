#pragma once

class RecvBuffer
{
	/* Read, Write 인덱스가 겹치는 상황이 더욱 빈번하게 발생할 수 있도록
	생성하려는 버퍼 크기보다 10배 크게 생성할 것이다 */
	enum { BUFFER_COUNT = 10 };

public:
	RecvBuffer(int32 _iBufferSize);
	~RecvBuffer();


	void		Clean();
	// Read, Write Pos를 실질적으로 조절하는 함수
	bool		OnRead(int32 _iNumOfBytes);
	bool		OnWrite(int32 _iNumOfBytes);

	BYTE*		ReadPos() { return &m_vecBuffer[m_iReadPos]; }
	BYTE*		WritePos() { return &m_vecBuffer[m_iWritePos]; }
	int32		DataSize() { return (m_iWritePos - m_iReadPos); }
	int32		FreeSize() { return (m_iCapacity - m_iWritePos); }

private:
	int32			m_iCapacity = { 0 };
	int32			m_iBufferSize = { 0 };
	// 배열의 인덱스를 구분하기 위한 변수
	int32			m_iReadPos = { 0 };
	int32			m_iWritePos = { 0 };

	vector<BYTE>	m_vecBuffer;
};

