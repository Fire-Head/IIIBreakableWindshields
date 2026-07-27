#include "hook.h"
#include "../inc/CPatch.h"
#include "../CVersion.h"
#include <list>

namespace Base
{	
	template<typename T>
	inline static void CALL(int address, void *func, T &CB)
	{
		if ( *(int*)(address + 1) + (address + 5) != (int)func)
		{
			CB = reinterpret_cast <T>(*(int*)(address + 1) + (address + 5));
			CPatch::RedirectCall(address, func);
		}
	}
	template<typename T>
	inline static void RAWPTR(void *address, void *func, T &CB)
	{
		if ( *(int*)address != (int)func)
		{
			CB = reinterpret_cast <T>(*(int*)((int)address));
			CPatch::SetPointer((int)address, func);
		}
	}
	
	template<typename T>
	inline static void RAWPTR(int address, void *func, T &CB)
	{
		if ( *(int*)address != (int)func)
		{
			CB = reinterpret_cast <T>(*(int*)(address));
			CPatch::SetPointer(address, func);
		}
	}

	namespace MainState
	{
		typedef void (*tdebugCB)(const char *format);
		typedef void (*tCallBack)();
		typedef char *(*tstrcpy)(char *dst, const char *src);
		typedef int (*tRsEventHandler)(int, int);


		static std::list<void (*)()>m_aInitialiseList;
		static std::list<void (*)()>m_aReInitGameObjectVariablesList;
		static std::list<void (*)()>m_aProcessList;
		static std::list<void (*)()>m_aRenderEffects;
		static std::list<void (*)()>m_aPoolsInitAfter;
		static std::list<void (*)()>m_aPoolsShutdownBefore;
		static std::list<void (*)()>m_aReplayStoreStuffInMem;
		static std::list<void (*)()>m_aReplayRestoreStuffFromMem;
		static std::list<void (*)()>m_aAsiCheck;
		
		static tstrcpy InitialiseCB;
		static tdebugCB ReInitGameObjectVariablesCB;
		static tCallBack ProcessCB;
		static tCallBack RenderEffectsCB;
		static tCallBack PoolsInitCB;
		static tCallBack PoolsShutdownCB;
		
		static tCallBack ReplayStoreStuffInMemCB;
		static tCallBack ReplayRestoreStuffFromMemCB;
		
		static tRsEventHandler AsiCheckCB;
		
		
		char *__cdecl onInitialise(char *dst, const char *src)
		{
			for(auto i = m_aInitialiseList.begin(); i != m_aInitialiseList.end(); ++i)
			(*i)();		
			return InitialiseCB(dst, src);
		}
		
		void __cdecl onReInitGameObjectVariables(const char *format)
		{
			for(auto i = m_aReInitGameObjectVariablesList.begin(); i != m_aReInitGameObjectVariablesList.end(); ++i)
			(*i)();
			ReInitGameObjectVariablesCB(format);
		}
		
		void __cdecl onProcess()
		{
			for(auto i = m_aProcessList.begin(); i != m_aProcessList.end(); ++i)
			(*i)();
			ProcessCB();
		}
		
		void __cdecl onRenderEffects()
		{
			for(auto i = m_aRenderEffects.begin(); i != m_aRenderEffects.end(); ++i)
			(*i)();
			RenderEffectsCB();
		}
		
		void __cdecl onPoolsInit()
		{
			/*
			for(auto i = m_aPoolsInitBefore.begin(); i != m_aPoolsInitBefore.end(); ++i)
				(*i)();
			*/
		
			PoolsInitCB();
			
			for(auto i = m_aPoolsInitAfter.begin(); i != m_aPoolsInitAfter.end(); ++i)
				(*i)();
		}
		
		void __cdecl onPoolsShutdown()
		{
			for(auto i = m_aPoolsShutdownBefore.begin(); i != m_aPoolsShutdownBefore.end(); ++i)
				(*i)();
		
			PoolsShutdownCB();
			
			/*
			for(auto i = m_aPoolsShutdownAfter.begin(); i != m_aPoolsShutdownAfter.end(); ++i)
				(*i)();
			*/
		}
		
		void __cdecl onReplayStoreStuffInMem()
		{
			ReplayStoreStuffInMemCB();

			for(auto i = m_aReplayStoreStuffInMem.begin(); i != m_aReplayStoreStuffInMem.end(); ++i)
				(*i)();
		}
		
		void __cdecl onReplayRestoreStuffFromMem()
		{
			for(auto i = m_aReplayRestoreStuffFromMem.begin(); i != m_aReplayRestoreStuffFromMem.end(); ++i)
				(*i)();
		
			ReplayRestoreStuffFromMemCB();
		}

		int __cdecl onAsiCheck(int a, int b)
		{
			for(auto i = m_aAsiCheck.begin(); i != m_aAsiCheck.end(); ++i)
				(*i)();
		
			return AsiCheckCB(a, b);
		}

		void CGameState::Register(eFunc type, tCB func)
		{			
			switch ( type )
			{
			case F_Initialise:
				m_aInitialiseList.push_back(func);
				CALL(AddressByVersion(0x48BEE1, 0x48BFD1, 0x48BF61), onInitialise, InitialiseCB);
				break;
			case F_ReInitGameObjectVariables:
				m_aReInitGameObjectVariablesList.push_back(func);
				CALL(AddressByVersion(0x48C4D1, 0x48C5D1, 0x48C561), onReInitGameObjectVariables, ReInitGameObjectVariablesCB);
				break;
			case F_Process:
				m_aProcessList.push_back(func);
				CALL(AddressByVersion(0x48C888, 0x48C988, 0x48C918), onProcess, ProcessCB);
				break;
			case F_RenderEffects:
				m_aRenderEffects.push_back(func);
				CALL(AddressByVersion(0x48E0C7, 0x48E187, 0x48E117), onRenderEffects, RenderEffectsCB);
				break;
			case F_PoolInitAfter:
				m_aPoolsInitAfter.push_back(func);
				CALL(AddressByVersion(0x48BEE8, 0x48BFD8, 0x48BF68), onPoolsInit, PoolsInitCB);
				break;
				
			case F_PoolShutDownBefore:
				m_aPoolsShutdownBefore.push_back(func);
				CALL(AddressByVersion(0x48C491, 0x48C581, 0x48C511), onPoolsShutdown, PoolsShutdownCB);
				break;
				
			case F_ReplayStoreStuffInMem:
				m_aReplayStoreStuffInMem.push_back(func);
				CALL(AddressByVersion(0x596154, 0x596404, 0x5962F4), onReplayStoreStuffInMem, ReplayStoreStuffInMemCB);
				break;
			case F_ReplayRestoreStuffFromMem:
				m_aReplayRestoreStuffFromMem.push_back(func);
				CALL(AddressByVersion(0x595B37, 0x595DE7, 0x595CD7), onReplayRestoreStuffFromMem, ReplayRestoreStuffFromMemCB);
				break;

			case F_ASICheck:
				m_aAsiCheck.push_back(func);				
				CALL(AddressByVersion(0x58275E, 0x582A9E, 0x58298E), onAsiCheck, AsiCheckCB);
				break;
			}
		}
	};

	namespace EntityHooks
	{
		template <typename T, int A, int B, int C, bool bPatchRaw = false>
		class hook_thiscall
		{
			typedef T (__thiscall *tThiscallCB)(T);
			
			static tThiscallCB _callback;
			static std::list<tStructorCB> m_aAfter;
			static std::list<tStructorCB> m_aBefore;
			
			static void Before(T Obj)
			{
				for(auto i = m_aBefore.begin(); i != m_aBefore.end(); ++i)
					(*i)(Obj);
			}
			
			static void After(T Obj)
			{
				for(auto i = m_aAfter.begin(); i != m_aAfter.end(); ++i)
					(*i)(Obj);
			}
			
			static T __thiscall Func(T This)
			{
				__asm pushad
				Before(This);
				__asm popad
				
				T result = _callback(This);
				
				__asm pushad
				After(This);
				__asm popad
				
				return result;
			}
			
		public:
			void Patch()
			{
				if ( bPatchRaw == false )
					CALL(AddressByVersion(A, B, C), Func, _callback);
				else
					RAWPTR(AddressByVersion(A, B, C), Func, _callback);
			}
			
			void RegisterBefore(tStructorCB func)
			{
				m_aBefore.push_back(func);
			}
			
			void RegisterAfter(tStructorCB func)
			{
				m_aAfter.push_back(func);
			}
		};
		
		#define DECL_STATICS(a) \
			a::tThiscallCB a::_callback; \
			std::list<tStructorCB> a::m_aAfter; \
			std::list<tStructorCB> a::m_aBefore;
			
		typedef hook_thiscall<class CPed *, 0x4C41C9, 0x4C4269, 0x4C41F9> PedConstructor;
		PedConstructor PedConstructorHook;
		DECL_STATICS(PedConstructor);
		
		typedef hook_thiscall<class CPed *, 0x4C5216, 0x4C52B6, 0x4C5246> PedDestructor;
		PedDestructor PedDestructorHook;
		DECL_STATICS(PedDestructor);
		
		typedef hook_thiscall<class CVehicle *, 0x550A68, 0x550BA8, 0x550B58> VehicleConstructor;
		VehicleConstructor VehicleConstructorHook;
		DECL_STATICS(VehicleConstructor);
		
		typedef hook_thiscall<class CVehicle *, 0x551114, 0x551254, 0x551204> VehicleDestructor;
		VehicleDestructor VehicleDestructorHook;
		DECL_STATICS(VehicleDestructor);
		
		#undef  DECL_STATICS(a)
		
		void CEntityHook::Register(eEntityHooks type, tStructorCB func)
		{
			switch ( type )
			{
			case F_PedConstructorBefore:
				PedConstructorHook.Patch();
				PedConstructorHook.RegisterBefore(func);
				break;
			case F_PedDestructorAfter:
				PedDestructorHook.Patch();
				PedDestructorHook.RegisterAfter(func);
				break;
				
			case F_VehicleConstructorBefore:
				VehicleConstructorHook.Patch();
				VehicleConstructorHook.RegisterBefore(func);
				break;
			
			case F_VehicleDestructorAfter:
				VehicleDestructorHook.Patch();
				VehicleDestructorHook.RegisterAfter(func);
				break;
			};
		}
	};
};