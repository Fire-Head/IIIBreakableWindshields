#pragma once
#include "entplugbase.h"
	
class VehicleExtendersHandler : public ExtendersHandler<CVehicle>
{
public:
	static void Add(ExtenderInterface<CVehicle> *extender)
	{
		static_data& data = get_data();
		data.extenders.push_back(extender);
		if (!data.injected)
		{
			CHook::Register(F_PoolInitAfter, Allocate);
			CHook::Register(F_PoolShutDownBefore, DeAllocate);
			CHook::RegisterCTDT(F_VehicleConstructorBefore, Constructor);
			CHook::RegisterCTDT(F_VehicleDestructorAfter, Destructor);
			CHook::Register(F_ReplayStoreStuffInMem, StoreReplay);
			CHook::Register(F_ReplayRestoreStuffFromMem, RestoreReplay);
			data.injected = true;
		}
	}
};

template <typename T> class VehicleExtendedData : public ExtenderInterface<CVehicle>
{
	T **blocks;
	unsigned char **replayBuff;
	unsigned int numBlocks;
	
	void AllocateBlocks() 
	{
		numBlocks = CPools::ms_pVehiclePool->m_Size;
		
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
				blocks[i]->Store();

				replayBuff[i] = new unsigned char[sizeof(T)];
				
				memcpy(replayBuff[i], blocks[i], sizeof(T));
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

	void OnConstructor(CVehicle *vehicle)
	{
		int idx = CPools::ms_pVehiclePool->GetJustIndex(vehicle);
		blocks[idx] = new T(vehicle);
	}

	void OnDestructor(CVehicle *vehicle)
	{
		int idx = CPools::ms_pVehiclePool->GetJustIndex(vehicle);
		delete blocks[idx];
		blocks[idx] = 0;
	}
	
public:
	
	void Init()
	{
		blocks = 0;
		replayBuff = 0;
		VehicleExtendersHandler::Add(this);
	}

	T &Get(CVehicle *vehicle)
	{
		int idx = CPools::ms_pVehiclePool->GetJustIndex(vehicle);

		return *blocks[idx];
	}
};	
