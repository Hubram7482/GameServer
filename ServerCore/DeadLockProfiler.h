#pragma once
#include <stack>
#include <map>
#include <vector>


/*---------------------
	DeadLockProfiler
-----------------------*/

/* ����� ��忡�� ���� �ɰų� �������ٶ� �׷��� 
�˰����� ���� ����� ��Ȳ���� �Ǻ����ش� */

class DeadLockProfiler
{
public:
	// 
	void PushLock(const char* _pName);
	void PopLock(const char* _pName);
	void CheckCycle();

private:
	void DFS(int32 _iIndex);

private:
	/* ���� �޾��� ���� �����ϱ� ������ ���ڿ��� �޾������� ���������� 
	������ �ϴ� ��Ȳ������ ������ ������ ������ ���� �ΰ��� ����*/
	unordered_map<const char*, int32> m_umapNameToID;
	unordered_map<int32, const char*> m_umapIDToName;
	// Lock�� ����Ǵ� ������ �����ϱ� ���� ����
	stack<int32>					  m_stkLock;
	// Lock�� ����� Lock�� ���� ������ �����ϱ� ���� ����
	map<int32, set<int32>>			  m_mapLockHistory;

	Mutex							  m_Lock;

private:
	// ����Ŭ�� �Ǻ��ϴ� �˰����� ���� �ӽ� ���鿡 ���� ����

	// ��尡 �߰ߵ� ������ ����ϴ� �迭 
	vector<int32> m_vecDiscoveredOrder;
	// ��尡 �߰ߵ� ����
	int32		  m_iDiscoveredCount = 0; 
	// DFS(i)�� ���� �Ǿ������� ���� ����
	vector<bool>  m_vecFinished;
	// �ڱ��ڽ��� �߰��� ��带 ����
	vector<int32> m_vecParent;

};

