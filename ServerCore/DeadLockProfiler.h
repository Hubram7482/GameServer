#pragma once
#include <stack>
#include <map>
#include <vector>


/*---------------------
	DeadLockProfiler
-----------------------*/

/* 디버그 모드에서 락을 걸거나 해제해줄때 그래프 
알고리즘을 통해 데드락 상황인지 판별해준다 */

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
	/* 락을 받아줄 때는 구분하기 쉽도록 문자열로 받아주지만 내부적으로 
	연산을 하는 상황에서는 정수가 빠르기 때문에 변수 두개를 선언*/
	unordered_map<const char*, int32> m_umapNameToID;
	unordered_map<int32, const char*> m_umapIDToName;
	// Lock이 실행되는 순서를 추적하기 위한 변수
	stack<int32>					  m_stkLock;
	// Lock이 잡아준 Lock에 대한 정보를 관리하기 위한 변수
	map<int32, set<int32>>			  m_mapLockHistory;

	Mutex							  m_Lock;

private:
	// 사이클을 판별하는 알고리즘을 위한 임시 값들에 대한 변수

	// 노드가 발견된 순서를 기록하는 배열 
	vector<int32> m_vecDiscoveredOrder;
	// 노드가 발견된 순서
	int32		  m_iDiscoveredCount = 0; 
	// DFS(i)가 종료 되었는지에 대한 여부
	vector<bool>  m_vecFinished;
	// 자기자신을 발견한 노드를 저장
	vector<int32> m_vecParent;

};

