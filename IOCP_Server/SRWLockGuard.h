#pragma once
#include <Windows.h>

class SRWLockGuard
{
public:
	SRWLockGuard(SRWLOCK* lock, bool exclusive = true)
		: mLock(lock)
		, mExclusive(exclusive)
	{
		if (mExclusive)
		{
			AcquireSRWLockExclusive(mLock);
		}
		else
		{
			AcquireSRWLockShared(mLock);
		}
	}

	~SRWLockGuard()
	{
		if (mExclusive)
		{
			ReleaseSRWLockExclusive(mLock);
		}
		else
		{
			ReleaseSRWLockShared(mLock);
		}
	}

	SRWLockGuard(const SRWLockGuard&) = delete;
	SRWLockGuard& operator=(const SRWLockGuard&) = delete;

private:
	SRWLOCK* mLock;
	bool mExclusive;
};
