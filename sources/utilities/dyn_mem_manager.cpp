#include "dyn_mem_manager.h"
#include <malloc.h>


void * utilities::allocate(const size_t sz)
{
	return malloc(sz);
}

void utilities::deallocate(void * ptr)
{
	free(ptr);
}

void * utilities::reallocate(void * ptr, const size_t sz)
{
	return realloc(ptr, sz);
}
