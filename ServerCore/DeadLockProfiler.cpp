#include "pch.h"
#include "DeadLockProfiler.h"

/*---------------------
	DeadLockProfiler
-----------------------*/


void DeadLockProfiler::PushLock(const char* _pName)
{
	LockGuard Guard(m_Lock);

	// 아이디를 찾거나 발급한다.
	int32 iLockID = 0;

	auto FindID = m_umapNameToID.find(_pName);
	if (FindID == m_umapNameToID.end())
	{
		// 처음 발견한 노드인 경우 ID를 번호 순서대로 발급
		iLockID = static_cast<int32>(m_umapNameToID.size());
		m_umapNameToID[_pName] = iLockID;
		m_umapIDToName[iLockID] = _pName;
	}
	else
	{
		// 처음 발견한 노드가 아닌 경우 ID를 추출
		iLockID = FindID->second;
	}

	// 잡고 있는 Lock이 있었다면
	if (m_stkLock.size())
	{
		/* 기존에 발견되지 않은 케이스라면 데드락 여부를 다시 확인하는데,
		만약 동일한 Lock인 경우 재귀적으로 중복 Lock을 잡는 경우라서
		데드락 여부를 확인하지 않는다 */
		const int32 iPrevID = m_stkLock.top();
		if (iPrevID != iLockID)
		{
			set<int32>& setHistory = m_mapLockHistory[iPrevID];
			if (setHistory.find(iLockID) == setHistory.end())
			{
				/* 이전 Lock이 현재 Lock을 처음 발견한 상황이라면 
				데이터를 추가하고 사이클 검사를 진행한다 */
				setHistory.emplace(iLockID);
				CheckCycle();
			}

		}

		return;
	}

	m_stkLock.emplace(iLockID);
}

void DeadLockProfiler::PopLock(const char* _pName)
{
	LockGuard Guard(m_Lock);

	if (m_stkLock.empty())
	{
		CRASH("MULTIPLE_UNLOCK");
	}

	// Push, Pop 순서에 문제가 있는지 체크
	int32 iLockID = m_umapNameToID[_pName];
	if (iLockID != m_stkLock.top())
		CRASH("INVALID_UNLOCK");

	m_stkLock.pop();
}

void DeadLockProfiler::CheckCycle()
{
	// 현재까지 발견된 LockCount개수
	const int32 iLockCount = static_cast<int32>(m_umapNameToID.size());

	// -1이라면 정점이 방문되지 않은 상태를 의미
	m_vecDiscoveredOrder = vector<int32>(iLockCount, - 1);
	m_vecFinished = vector<bool>(iLockCount, false);
	m_vecParent   = vector<int32>(iLockCount, -1);
	m_iDiscoveredCount = 0;

	// LockID를 순서대로 DFS를 실행
	for (int32 iLockID = 0; iLockID < iLockCount; ++iLockID)
	{
		DFS(iLockID);
	}

	// 연산이 끝났다면 정리
	m_vecDiscoveredOrder.clear();
	m_vecFinished.clear();
	m_vecParent.clear();

}

void DeadLockProfiler::DFS(int32 _iHere)
{
	// 이미 방문된 정점
	if (m_vecDiscoveredOrder[_iHere] != -1)
		return;

	// 방문된 적이 없다면 순서대로 번호를 저장
	m_vecDiscoveredOrder[_iHere] = m_iDiscoveredCount++;

	// 모든 인접한 정점들을 순회한다
	auto FindID = m_mapLockHistory.find(_iHere);
	if (FindID == m_mapLockHistory.end())
	{
		// 다른 Lock을 아직 잡은 적이 없다면
		m_vecFinished[_iHere] = true;
		return;
	}
	
	// 연결되어 있는 정점들
	set<int32>& setNext = FindID->second;
	for (int32 iThere : setNext)
	{
		// 아직 방문한 적이 없다면 방문한다
		if (m_vecDiscoveredOrder[iThere] == -1)
		{
			m_vecParent[iThere] = _iHere;
			DFS(iThere);
			continue;
		}

		/* Here가 There보다 먼저 발견되었다면, 
		There는 Here의 후손이다(순방향 간선) */
		if (m_vecDiscoveredOrder[_iHere] < m_vecDiscoveredOrder[iThere])
			continue;

		/* 순방향이 아니고, DFS(There)가 아직 종료되지 
		않았다면, There는 Here의 선조이다(역방향 간선) */
		if (!m_vecFinished[iThere])
		{
			// 사이클 발생 시 로그를 출력한다
			printf("%s -> %s\n", m_umapIDToName[_iHere], m_umapIDToName[iThere]);

			int32 iCur = _iHere;
			while (true)
			{
				// 현재 정점의 부모를 추적하여 출력
				printf("%s -> %s\n", 
					m_umapIDToName[m_vecParent[iCur]],
					m_umapIDToName[iCur]);
				
				// There에 도달했다면 정지
				iCur = m_vecParent[iCur];
				if (iCur == iThere)
					break;
			}

			CRASH("DEADLOCK_DETECTED");
		}
	}

	m_vecFinished[_iHere] = true;
}
