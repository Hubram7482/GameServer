#pragma once

/*---------------------
	  RefCountable
-----------------------*/

class RefCountable
{
public:
	RefCountable() : m_iRefCount(1) {}
	virtual ~RefCountable() {}

	int32 GetRefCount() { return m_iRefCount; }
	
	int32 AddRef() { return ++m_iRefCount; }
	int32 ReleaseRef()
	{
		int32 iRefCount = --m_iRefCount;
	
		if (iRefCount == 0)
		{
			delete this;
		}

		return iRefCount;
	}

private:
	Atomic<int32> m_iRefCount;


};

