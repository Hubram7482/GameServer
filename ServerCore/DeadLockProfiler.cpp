#include "pch.h"
#include "DeadLockProfiler.h"

/*---------------------
	DeadLockProfiler
-----------------------*/


void DeadLockProfiler::PushLock(const char* _pName)
{
	LockGuard Guard(m_Lock);

	// ���̵� ã�ų� �߱��Ѵ�.
	int32 iLockID = 0;

	auto FindID = m_umapNameToID.find(_pName);
	if (FindID == m_umapNameToID.end())
	{
		// ó�� �߰��� ����� ��� ID�� ��ȣ ������� �߱�
		iLockID = static_cast<int32>(m_umapNameToID.size());
		m_umapNameToID[_pName] = iLockID;
		m_umapIDToName[iLockID] = _pName;
	}
	else
	{
		// ó�� �߰��� ��尡 �ƴ� ��� ID�� ����
		iLockID = FindID->second;
	}

	// ��� �ִ� Lock�� �־��ٸ�
	if (m_stkLock.size())
	{
		/* ������ �߰ߵ��� ���� ���̽���� ����� ���θ� �ٽ� Ȯ���ϴµ�,
		���� ������ Lock�� ��� ��������� �ߺ� Lock�� ��� ����
		����� ���θ� Ȯ������ �ʴ´� */
		const int32 iPrevID = m_stkLock.top();
		if (iPrevID != iLockID)
		{
			set<int32>& setHistory = m_mapLockHistory[iPrevID];
			if (setHistory.find(iLockID) == setHistory.end())
			{
				/* ���� Lock�� ���� Lock�� ó�� �߰��� ��Ȳ�̶�� 
				�����͸� �߰��ϰ� ����Ŭ �˻縦 �����Ѵ� */
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

	// Push, Pop ������ ������ �ִ��� üũ
	int32 iLockID = m_umapNameToID[_pName];
	if (iLockID != m_stkLock.top())
		CRASH("INVALID_UNLOCK");

	m_stkLock.pop();
}

void DeadLockProfiler::CheckCycle()
{
	// ������� �߰ߵ� LockCount����
	const int32 iLockCount = static_cast<int32>(m_umapNameToID.size());

	// -1�̶�� ������ �湮���� ���� ���¸� �ǹ�
	m_vecDiscoveredOrder = vector<int32>(iLockCount, - 1);
	m_vecFinished = vector<bool>(iLockCount, false);
	m_vecParent   = vector<int32>(iLockCount, -1);
	m_iDiscoveredCount = 0;

	// LockID�� ������� DFS�� ����
	for (int32 iLockID = 0; iLockID < iLockCount; ++iLockID)
	{
		DFS(iLockID);
	}

	// ������ �����ٸ� ����
	m_vecDiscoveredOrder.clear();
	m_vecFinished.clear();
	m_vecParent.clear();

}

void DeadLockProfiler::DFS(int32 _iHere)
{
	// �̹� �湮�� ����
	if (m_vecDiscoveredOrder[_iHere] != -1)
		return;

	// �湮�� ���� ���ٸ� ������� ��ȣ�� ����
	m_vecDiscoveredOrder[_iHere] = m_iDiscoveredCount++;

	// ��� ������ �������� ��ȸ�Ѵ�
	auto FindID = m_mapLockHistory.find(_iHere);
	if (FindID == m_mapLockHistory.end())
	{
		// �ٸ� Lock�� ���� ���� ���� ���ٸ�
		m_vecFinished[_iHere] = true;
		return;
	}
	
	// ����Ǿ� �ִ� ������
	set<int32>& setNext = FindID->second;
	for (int32 iThere : setNext)
	{
		// ���� �湮�� ���� ���ٸ� �湮�Ѵ�
		if (m_vecDiscoveredOrder[iThere] == -1)
		{
			m_vecParent[iThere] = _iHere;
			DFS(iThere);
			continue;
		}

		/* Here�� There���� ���� �߰ߵǾ��ٸ�, 
		There�� Here�� �ļ��̴�(������ ����) */
		if (m_vecDiscoveredOrder[_iHere] < m_vecDiscoveredOrder[iThere])
			continue;

		/* �������� �ƴϰ�, DFS(There)�� ���� ������� 
		�ʾҴٸ�, There�� Here�� �����̴�(������ ����) */
		if (!m_vecFinished[iThere])
		{
			// ����Ŭ �߻� �� �α׸� ����Ѵ�
			printf("%s -> %s\n", m_umapIDToName[_iHere], m_umapIDToName[iThere]);

			int32 iCur = _iHere;
			while (true)
			{
				// ���� ������ �θ� �����Ͽ� ���
				printf("%s -> %s\n", 
					m_umapIDToName[m_vecParent[iCur]],
					m_umapIDToName[iCur]);
				
				// There�� �����ߴٸ� ����
				iCur = m_vecParent[iCur];
				if (iCur == iThere)
					break;
			}

			CRASH("DEADLOCK_DETECTED");
		}
	}

	m_vecFinished[_iHere] = true;
}
