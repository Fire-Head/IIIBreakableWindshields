#pragma once

namespace Base
{
	typedef void (*tCB)();
	typedef void (*tSaveLoadCB)(unsigned int);
	typedef void (*tStructorCB)(void *);

	namespace MainState
	{
		enum eFunc
		{
			F_Initialise,
			F_ReInitGameObjectVariables,
			F_Process,
			F_RenderEffects,
			F_PoolInitAfter,
			F_PoolShutDownBefore,
			F_ReplayStoreStuffInMem,
			F_ReplayRestoreStuffFromMem,
			F_ASICheck,
		};
		
		class CGameState
		{
		public:
			static void Register(eFunc type, tCB func);
		};
	};
	
	namespace EntityHooks
	{
		enum eEntityHooks
		{
			F_PedConstructorBefore = 0,
			
			F_PedDestructorAfter,
			
			F_VehicleConstructorBefore,
			
			F_VehicleDestructorAfter,			
		};
		
		class CEntityHook
		{
		public:
			static void Register(eEntityHooks type, tStructorCB func);
		};
	}
	
	namespace General
	{
		using namespace MainState;
		using namespace EntityHooks;

		class CHook 
		{
		public:
			static void Register(eFunc type, tCB func)
			{
				CGameState::Register(type, func);
			}
			static void RegisterCTDT(eEntityHooks type, tStructorCB func)
			{
				CEntityHook::Register(type, func);
			}
		};
	};
};

using namespace Base::General;