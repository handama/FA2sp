#include "../FA2sp.h"

struct FreeBlock
{
	FreeBlock* next;
};

static FreeBlock* g_pool64 = nullptr;
static FreeBlock* g_pool256 = nullptr;
static FreeBlock* g_pool1024 = nullptr;

static int g_pool64_count = 0;
static int g_pool256_count = 0;
static int g_pool1024_count = 0;

static const int POOL_LIMIT_64 = 262144;
static const int POOL_LIMIT_256 = 8192;
static const int POOL_LIMIT_1024 = 1024; 

static char* AllocFromPool(int total)
{
	FreeBlock** pool = nullptr;
	int* count = nullptr;
	int allocSize = 0;

	if (total <= 64)
	{
		pool = &g_pool64;
		count = &g_pool64_count;
		allocSize = 64;
	}
	else if (total <= 256)
	{
		pool = &g_pool256;
		count = &g_pool256_count;
		allocSize = 256;
	}
	else if (total <= 1024)
	{
		pool = &g_pool1024;
		count = &g_pool1024_count;
		allocSize = 1024;
	}
	else
	{
		return (char*)FAMemory::Allocate(total);
	}

	if (*pool)
	{
		FreeBlock* block = *pool;
		*pool = block->next;
		(*count)--;
		return (char*)block;
	}

	return (char*)FAMemory::Allocate(allocSize);
}

static void FreeToPool(void* pRaw, int total)
{
	FreeBlock* block = (FreeBlock*)pRaw;
#ifndef NDEBUG
	memset(pRaw, 0xDD, total);
#endif
	if (total <= 64)
	{
		if (g_pool64_count >= POOL_LIMIT_64)
		{
			FAMemory::Deallocate(pRaw);
			return;
		}
		block->next = g_pool64;
		g_pool64 = block;
		g_pool64_count++;
	}
	else if (total <= 256)
	{
		if (g_pool256_count >= POOL_LIMIT_256)
		{
			FAMemory::Deallocate(pRaw);
			return;
		}
		block->next = g_pool256;
		g_pool256 = block;
		g_pool256_count++;
	}
	else if (total <= 1024)
	{
		if (g_pool1024_count >= POOL_LIMIT_1024)
		{
			FAMemory::Deallocate(pRaw);
			return;
		}
		block->next = g_pool1024;
		g_pool1024 = block;
		g_pool1024_count++;
	}
	else
	{
		FAMemory::Deallocate(pRaw);
	}
}

DEFINE_HOOK(555D7C, CString_AllocBuffer, 6)
{
	GET(ppmfc::CString*, pThis, ECX);
	GET_STACK(int, nLen, 0x4);

	if (nLen < 0)
		nLen = 0;

	int total = nLen + 1 + 0xC;

	char* pRaw = AllocFromPool(total);

	// CStringData
	*(LONG*)(pRaw + 0x0) = 1; 
	*(int*)(pRaw + 0x4) = nLen; 
	*(int*)(pRaw + 0x8) = total - 0xC - 1;

	pRaw[0xC + nLen] = '\0';

	char* psz = pRaw + 0xC;
	*(char**)pThis = psz;

	return 0x555DFB;
}

DEFINE_HOOK(555DFE, CString_FreeData, 6)
{
	GET(char*, pRaw, ECX);

	if (!pRaw)
		return 0x555E45;

	int allocLen = *(int*)(pRaw + 0x8);
	int total = allocLen + 1 + 0xC;

	FreeToPool(pRaw, total);

	return 0x555E45;
}
