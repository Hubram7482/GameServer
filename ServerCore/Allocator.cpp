#include "pch.h"
#include "Allocator.h"

void* BaseAllocator::Alloc(int32 _iSize)
{
	return malloc(_iSize);
}

void BaseAllocator::Release(void* _pPtr)
{
	free(_pPtr);
}
