#pragma once
#include "entplugbase.h"
	
class PedExtendersHandler : public ExtendersHandler<CPed>
{
public:
	static void Add(ExtenderInterface<CPed> *extender)
	{
		static_data& data = get_data();
		data.extenders.push_back(extender);
		if (!data.injected)
		{
			CHook::Register(F_PoolInitAfter, Allocate);
			CHook::Register(F_PoolShutDownBefore, DeAllocate);
			CHook::RegisterCTDT(F_PedConstructorBefore, Constructor);
			CHook::RegisterCTDT(F_PedDestructorAfter, Destructor);
			CHook::Register(F_ReplayStoreStuffInMem, StoreReplay);
			CHook::Register(F_ReplayRestoreStuffFromMem, RestoreReplay);
			data.injected = true;
		}
	}
};

template <typename T> class PedExtendedData : public ExtenderInterface<CPed>
{
	T **blocks;
	unsigned char **replayBuff;
	unsigned int numBlocks;
	
	void AllocateBlocks() 
	{
		numBlocks = CPools::ms_pPedPool->m_Size;

		blocks = new T*[numBlocks];
		for (unsigned int i = 0; i < numBlocks; i++)
			blocks[i] = 0;
		
		//
		
		replayBuff = new unsigned char *[numBlocks];
		for (unsigned int i = 0; i < numBlocks; i++)
			replayBuff[i] = 0;
	}
		
	void DeAllocateBlocks()
	{
		for (unsigned int i = 0; i < numBlocks; i++)
		{
			if ( replayBuff[i] )
				delete [] replayBuff[i];
		}
		delete[] replayBuff;
		
		//
		
		for (unsigned int i = 0; i < numBlocks; i++)
		{
			if ( blocks[i] )
				delete blocks[i];
		}
		delete[] blocks;
	}
	
	void Store()
	{
		for (unsigned int i = 0; i < numBlocks; i++)
		{
			if ( blocks[i] )
			{
				blocks[i]->Store(true);

				replayBuff[i] = new unsigned char[sizeof(T)];
				
				memcpy(replayBuff[i], blocks[i], sizeof(T));
				
				blocks[i]->Store(false);
			}
			else
				replayBuff[i] = 0;
		}
	}
	
	void Restore()
	{
		for (unsigned int i = 0; i < numBlocks; i++)
		{
			if ( replayBuff[i] )
			{				
				blocks[i] = new T();

				memcpy(blocks[i], replayBuff[i], sizeof(T));
				
				delete [] replayBuff[i];
				
				blocks[i]->Restore();
			}
			else
				blocks[i] = 0;
		}
	}

	void OnConstructor(CPed *ped)
	{
		int idx = CPools::ms_pPedPool->GetJustIndex(ped);
		blocks[idx] = new T(ped);
	}

	void OnDestructor(CPed *ped)
	{
		int idx = CPools::ms_pPedPool->GetJustIndex(ped);
		delete blocks[idx];
		blocks[idx] = 0;
	}
	
public:
	
	void Init()
	{
		blocks = 0;
		replayBuff = 0;
		PedExtendersHandler::Add(this);
	}

	T &Get(CPed *ped)
	{
		int idx = CPools::ms_pPedPool->GetJustIndex(ped);

		return *blocks[idx];
	}
};	
