#include "_precl.h"
#include <math.h>
#define CLEOAPI 
#include "CLEO_SDK\III.CLEO.h"

int wsavedEax;
#define STOREREGS()  __asm mov wsavedEax, eax __asm pushad __asm mov eax, wsavedEax
#define RESTOREREGS() __asm mov wsavedEax, eax __asm popad __asm mov eax, wsavedEax
#define STACK_REGS_SIZE 32


const Char kBannedVehs[] = "";
const Char kBigVehs[] = "";

#define OBJECTIVE_LV_CAR_DIE 38

const UInt32 anim1ID = 37;
const UInt32 anim2ID = 173;

class CSettings
{
public:
	std::vector<Int32> m_vecVehicleBigList;
	std::vector<Int32> m_vecVehicleBlackList;
	Bool bShotgun;
	Bool bFixLongDebrisBugByRockstar;
	Bool bGlassFx;
	Bool bViceGlassFx;
	Bool bCleoIntegration;
	UInt32 nAnim1;
	UInt32 nAnim2;
	UInt32 nOpcode;

	CSettings()
	{
		CIniReader Ini("");
		
		char *pGameVersion = Ini.ReadString("MAIN", "Version", "auto");
		for ( Int n = 1; n < ARRLEN(aGameVer); n++)
		{
			if (!strcmp(pGameVersion, aGameVer[n]) && strcmp(pGameVersion, "auto"))
			{
				bAutoVersionCheck = false;
				nVersion = n;
				break;
			}
		}
		if (!strcmp(pGameVersion, "auto"))
		{
			bAutoVersionCheck = true;
			nVersion = -1;
		}

		std::vector<cppext::String>BigVehsList = cppext::String(Ini.ReadString("MAIN", "BigVehicles", kBigVehs)).Split(",");
		
		if ( BigVehsList.size() > 0 )
		{
			for ( auto i = BigVehsList.begin(); i != BigVehsList.end(); ++i )
				m_vecVehicleBigList.push_back((*i).Trim().ToInt());
		}
		
		std::vector<cppext::String>vehsList = cppext::String(Ini.ReadString("MAIN", "BannedVehicles", kBannedVehs)).Split(",");
		
		if ( vehsList.size() > 0 )
		{
			for ( auto i = vehsList.begin(); i != vehsList.end(); ++i )
				m_vecVehicleBlackList.push_back((*i).Trim().ToInt());
		}
		
		bGlassFx = Ini.ReadBoolean("MAIN", "bGlassFx", true);
		nAnim1 = Ini.ReadInteger("MAIN", "nAnim1", anim1ID);
		nAnim2 = Ini.ReadInteger("MAIN", "nAnim2", anim2ID);
		bShotgun = Ini.ReadBoolean("MAIN", "bBreakGlassWithShotgun", true);
		bViceGlassFx = Ini.ReadBoolean("MAIN", "bViceCityGlassFx", true);
		bFixLongDebrisBugByRockstar = Ini.ReadBoolean("MAIN", "bFixLongDebrisBugByRockstar", true);
		nOpcode = Ini.ReadInteger("OPCODES", "SET_CHAR_CAN_BE_SHOT_IN_VEHICLE", 0x054A);
		bCleoIntegration = Ini.ReadBoolean("MAIN", "bAddOpcodesForCleo", true);
	}
	
	Bool IsVehicleBig(Int32 index)
	{
		if ( m_vecVehicleBigList.size() > 0 )
		{
			for ( Int i = 0; i < m_vecVehicleBigList.size(); i++ )
			{
				if ( m_vecVehicleBigList[i] == index )
					return true;
			}
		}

		return false;
	}
	
	Bool IsVehicleBanned(Int32 index)
	{
		if ( m_vecVehicleBlackList.size() > 0 )
		{
			for ( Int i = 0; i < m_vecVehicleBlackList.size(); i++ )
			{
				if ( m_vecVehicleBlackList[i] == index )
					return true;
			}
		}

		return false;
	}

}settings;

CBaseModelInfo *getModelInfoPtrs(unsigned int id)
{
	//CBaseModelInfo **CModelInfo::ms_modelInfoPtrs = (CBaseModelInfo **)AddressByVersion(0x83D408, 0x83D408, 0x84D548);
	
	CBaseModelInfo ***_modelInfoPtrs = (CBaseModelInfo ***)(AddressByVersion(0x50B870, 0x50B960, 0x50B8F0) + 3);
		
	return (*_modelInfoPtrs)[id];
}

void PlayOneShotScriptObject(unsigned char id, CVector const &pos)
{
	((void (__cdecl *)(unsigned char, CVector const &))AddressByVersion(0x57C5F0, 0x57C940, 0x57C840))(id, pos);
}

int _cwrand()
{
	return ((int (__cdecl *)())AddressByVersion(0x5A41D0, 0X5A4490, 0X5A5170))();
}

RwTexture *&gpShadowHeadLightsTex = *(RwTexture **)AddressByVersion(0x95CB98, 0x95CD50, 0x96CE90);
RwTexture *&gpCrackedGlassTex = *(RwTexture **)AddressByVersion(0x95CB94, 0x95CD4C, 0x96CE8C);
RwTexture *&gpShadowExplosionTex = *(RwTexture **)AddressByVersion(0x8F2A00, 0x8F2AB4, 0x902BF4);

#include "Glass.h"

void SetExitBoat(CPed *This, CVehicle *pBoat)
{
	This->m_nPedState = PED_IDLE;
	
	CVector savedPos = This->m_sCoords.pos;

	CAnimManager::BlendAnimation(This->m_pRwClump, (AssocGroupId)This->m_nAnimGroupId, (AnimationId)3, 100.0f);

	if ( pBoat->m_nModelIndex == MODEL_SPEEDER && pBoat->IsUpsideDown() )
	{
		This->m_pVehicleAnimBlendAssoc = CAnimManager::BlendAnimation(This->m_pRwClump, (AssocGroupId)0, (AnimationId)134, 8.0f);
		This->m_pVehicleAnimBlendAssoc->SetFinishCallback(CPed::PedSetOutCarCB, This);
		This->m_nEnterType = 11;
		This->m_nPedState = PED_EXIT_CAR;
	}
	else
	{
		This->m_nEnterType = 11;
		CPed::PedSetOutCarCB(0, This);
		This->m_Flags0 = This->m_Flags0 & 0xFE | 1;
		This->m_pCurrentSurface = pBoat;
		This->m_pCurrentSurface->RegisterReference(&This->m_pCurrentSurface);
	}
	
	This->m_sCoords.pos = savedPos;
	
	This->SetMoveState((eMoveState)1);

	This->m_vecVelocity = pBoat->m_vecVelocity;
			  
	//This->m_Flags7 = This->m_Flags7 & 0xEF | 0x10;
	//CWaterLevel::FreeBoatWakeArray();
}

//SET_CHAR_CAN_BE_SHOT_IN_VEHICLE = 0x54A,
Bool __cdecl CAN_BE_SHOT_IN_VEHICLE_Func(CPed *This);

typedef Bool ( __cdecl *t_CAN_BE_SHOT_IN_VEHICLE_CB)(class CPed *pPed);
t_CAN_BE_SHOT_IN_VEHICLE_CB _CAN_BE_SHOT_IN_VEHICLECB = CAN_BE_SHOT_IN_VEHICLE_Func;

Bool __cdecl CAN_BE_SHOT_IN_VEHICLE(CPed *This)
{
	return _CAN_BE_SHOT_IN_VEHICLECB(This);
}

Bool IsVehicleNotBanned(CVehicle *pVehicle)
{
	return !settings.IsVehicleBanned(pVehicle->m_nModelIndex);
}

Bool IsVehicleBig(CVehicle *pVehicle)
{
	if ( settings.IsVehicleBig(pVehicle->m_nModelIndex) )
		return true;

	return false;
}


enum BoneTag {
	BONE_Swaist,
	BONE_Supperlegr,
	BONE_Slowerlegr,
	BONE_Sfootr,
	BONE_Supperlegl,
	BONE_Slowerlegl,
	BONE_Sfootl,
	BONE_Smid,
	BONE_Storso,
	BONE_Shead,
	BONE_Supperarmr,
	BONE_Slowerarmr,
	BONE_SRhand,
	BONE_Supperarml,
	BONE_Slowerarml,
	BONE_SLhand,
};

int
ConvertPedNode2BoneTag(int node)
{
	static int tags[] = { BONE_Swaist, BONE_Storso, BONE_Shead,
	                      BONE_Supperarml, BONE_Supperarmr,
	                      BONE_SLhand, BONE_SRhand,
	                      BONE_Supperlegl, BONE_Supperlegr,
	                      BONE_Sfootl, BONE_Sfootr,
	                      BONE_Slowerlegr, BONE_Slowerlegl };
	if(node > 12)
		return -1;
	return tags[node];
}

static RpAtomic *GetAnimHierarchyCallback(RpAtomic *atomic, void *data)
{
	*(RpHAnimHierarchy**)data = RpSkinAtomicGetHAnimHierarchy(atomic);
	return NULL;
}

RpHAnimHierarchy *GetAnimHierarchyFromSkinClump(RpClump *clump)
{
	RpHAnimHierarchy *hier = NULL;
	RpClumpForAllAtomics(clump, GetAnimHierarchyCallback, &hier);
	return hier;
}

RwMatrix *RpHAnimHierarchyGetMatrixArray(RpHAnimHierarchy *hierarchy)
{
	return hierarchy->pMatrixArray;
}

RwInt32 RpHAnimIDGetIndex(RpHAnimHierarchy *hierarchy, RwInt32 ID)
{
	for(RwInt32 i = 0; i < hierarchy->numNodes; i++)
		if(hierarchy->pNodeInfo[i].nodeID == ID)
			return i;
	return -1;
}

static RpAtomic *isSkinnedCb(RpAtomic *atomic, void *data)
{
	RpAtomic **ret = (RpAtomic **)data;
	if(*ret)
		return NULL;
	if(RpSkinGeometryGetSkin(atomic->geometry))
		*ret = atomic;
	return atomic;
}

RpAtomic *IsClumpSkinned(RpClump *c)
{
	RpAtomic *ret = NULL;
	RpClumpForAllAtomics(c, isSkinnedCb, &ret);
	return ret;
}

CVector GetPedHeadPosition(CPed *ped)
{
	if ( IsClumpSkinned(ped->m_pRwClump) )
	{
		RwV3d out = { 0.0f, 0.0f, 0.0f };
		RpHAnimHierarchy *hier = GetAnimHierarchyFromSkinClump(ped->m_pRwClump);
		int bonetag = ConvertPedNode2BoneTag(2);
		RwUInt32 index = RpHAnimIDGetIndex(hier, bonetag);
		RwMatrix *pMatrix = RpHAnimHierarchyGetMatrixArray(hier);
		RwV3dTransformPoints(&out, &out, 1, &pMatrix[index]);
		
		return CVector(out.x, out.y, out.z);
	}
	else
	{
		RwMatrix matrix;
		CPedIK::GetWorldMatrix(ped->m_apBodyParts[2]->m_pRwFrame, &matrix);
		return CVector(matrix.pos.x, matrix.pos.y, matrix.pos.z);
	}
}


Bool IsGlass(UInt32 material)
{
	return material == 7;
}


Bool _CanPedExitCar(CVehicle *This, Bool bSkipSpeedCheck)
{
	if ( This->m_sCoords.at.z > 0.1f || This->m_sCoords.at.z < -0.1f )
	{
		if ( This->m_vecVelocity.MagnitudeSqr() > 0.005f && !bSkipSpeedCheck )
			return false;

		if ( This->m_vecAngularVelocity.x < 0.0f && This->m_vecAngularVelocity.x < -0.01f )
			return false;
		else if ( This->m_vecAngularVelocity.x > 0.01f )
			return false;

		if ( This->m_vecAngularVelocity.y < 0.0f && This->m_vecAngularVelocity.y < -0.01f )
			return false;
		else if ( This->m_vecAngularVelocity.y > 0.01f )
			return false;

		if ( This->m_vecAngularVelocity.z < 0.0f && This->m_vecAngularVelocity.z < -0.01f )
			return false;
		else if ( This->m_vecAngularVelocity.z > 0.01f )
			return false;
	}
	else
	{
		if ( This->m_vecVelocity.MagnitudeSqr() >= 0.005f )
			return false;

		if ( This->m_vecAngularVelocity.x < 0.0f && This->m_vecAngularVelocity.x <= -0.01f )
			return false;
		else if ( This->m_vecAngularVelocity.x >= 0.01f )
			return false;

		if ( This->m_vecAngularVelocity.y < 0.0f && This->m_vecAngularVelocity.y <= -0.01f )
			return false;
		else if ( This->m_vecAngularVelocity.y >= 0.01f )
			return false;

		if ( This->m_vecAngularVelocity.z < 0.0f && This->m_vecAngularVelocity.z <= -0.01f )
			return false;
		else if ( This->m_vecAngularVelocity.z >= 0.01f )
			return false;
	}

	return true;
}

void __cdecl _CheckForShootingVehicleOccupant(CEntity **ppEntity/*out*/, CColPoint *colPoint/*out*/, eWeaponType weapon, CVector const &start, CVector const &end)
{
	CAutomobile *pAuto = (CAutomobile *)*ppEntity;

	if ( (*ppEntity)->m_nType == 2 )
	{
		CColSphere sphere;
		CColPoint savedColPoint = *colPoint;
		Float maxTouchDistance = 1.0;
		Bool bVictimExist = false;
		CColLine line(start, end);
		
		if ( !IsVehicleNotBanned(pAuto) )
			return;
	
		CPed *driver = pAuto->pDriver;

		if ( driver )
		{
			if ( CAN_BE_SHOT_IN_VEHICLE(driver) )
			{
				CVector pos = GetPedHeadPosition(driver);
				pos.z += 0.1f;

				sphere.Set(0.2f, pos, 0, 6);
				
				if ( CCollision::ProcessLineSphere(line, sphere, *colPoint, maxTouchDistance) )
				{
					*ppEntity = driver;
					bVictimExist = true;
				}
			}
		}

		for ( Int32 i = 0; i < 8; i++ )
		{
			CPed *ped = pAuto->pPassengers[i];
			if ( ped )
			{
				if ( CAN_BE_SHOT_IN_VEHICLE(ped) )
				{
					CVector pos = GetPedHeadPosition(ped);
					pos.z += 0.1f;

					sphere.Set(0.2f, pos, 0, 6);
					
					if ( CCollision::ProcessLineSphere(line, sphere, *colPoint, maxTouchDistance) )
					{
						*ppEntity = ped;
						bVictimExist = true;
					}
				}
			}
		}

		if ( pAuto->m_eVehicleType == VEHICLETYPE_CAR )
		{
			Float fYDot = DotProduct(end - start, pAuto->m_sCoords.up);
			Float fZDot = DotProduct(end - start, pAuto->m_sCoords.at);
			
			if ( (fYDot < 0.0f) && ( (fZDot <= 0.0f) || IsVehicleBig(pAuto) )  )
			{
				CColModel *colModel = getModelInfoPtrs(pAuto->m_nModelIndex)->m_pColModel;

				if ( colModel->m_nNumTriangles > 0 )
				{
					Bool bContactWithGlass = false;
					
					CMatrix transform;
					Invert(pAuto->m_sCoords, transform);
					
					line.m_vStart = transform * line.m_vStart;
					line.m_vEnd = transform * line.m_vEnd;
					
					CCollision::CalculateTrianglePlanes(colModel);
			
					for ( Int32 i = 0; i < colModel->m_nNumTriangles; i++ )
					{
						if ( IsGlass(colModel->m_pTriangles[i].material) && CCollision::TestLineTriangle(line, colModel->m_pVertices, colModel->m_pTriangles[i], colModel->m_pTrianglePlanes[i]) )
						{
							bContactWithGlass = true;
							break;
						}
					}
	
					if ( bContactWithGlass && pAuto->m_sCarDamage.ProgressPanelDamage(4) )
					{
						if ( pAuto->m_sCarDamage.GetPanelStatus(4) == 2 )
							pAuto->m_sCarDamage.ProgressPanelDamage(4);
						pAuto->SetPanelDamage(19, (ePanels)4, true);
						
						DMAudio.PlayOneShot(pAuto->m_nAudioIndex, 12, 0.0f);
					}
				}
			}
		}

		if ( !bVictimExist )
		{
			*ppEntity = pAuto;
			*colPoint = savedColPoint;
		}
	}
}

static int jmp_0x55867E = AddressByVersion(0x55867E, 0x5587AE, 0x55875E);
static int jmp_0x558695 = AddressByVersion(0x558695, 0x5587C5, 0x558775);
void NAK BulletInfoPatch()
{
	__asm
	{
		STOREREGS()
		
		lea     eax, [esp+STACK_REGS_SIZE+0B8h-60h]
		push    eax                        // end
		
		lea     eax, [esp+STACK_REGS_SIZE+0B8h+4h-6Ch]
		push    eax                        // start
		
		mov     eax, [ebp]
		push    eax                        // weapon
		
		lea     eax, [esp+STACK_REGS_SIZE+0B8h+0Ch-98h]
		push    eax                        // colPoint
		
		lea     eax, [esp+STACK_REGS_SIZE+0B8h+10h-70h]
		push    eax                        // ppEntity

		call    _CheckForShootingVehicleOccupant
		add     esp, 14h
		
		RESTOREREGS()
	}
	
	
	__asm
	{
		call    FindPlayerPed
		cmp     [ebp+4], eax
		jnz     short loc_558695 
	}
	VARJMP(jmp_0x55867E);
	
loc_558695:
	VARJMP(jmp_0x558695);
}

static int jmp_0x55D925 = AddressByVersion(0x55D925, 0x55DA55, 0x55DA05);
void NAK FireInstantHitPatch()
{
	__asm
	{
		/*
		test    al, al
		jz      short FIHP_FAIL
		*/
		
		cmp     dword ptr [esp+4C0h-3FCh], 0
		jz      FIHP_FAIL

		call    FindPlayerPed
		cmp     dword ptr [esp+4C0h+4], eax
		jnz     FIHP_FAIL
		
		push    ecx
		mov     ecx, TheCamera
		add     ecx, 1A4h
		call    CCam::Using3rdPersonMouseCam
		pop     ecx
		test    al, al
		jz      FIHP_FAIL

		STOREREGS()
		
		lea     eax, [esp+STACK_REGS_SIZE+4C0h-3CCh]
		push    eax						// end
		
		lea     eax, [esp+STACK_REGS_SIZE+4C0h+4-3D8h]
		push    eax						// start
		
		mov     eax, [ebx]
		push    eax						// weapon
		
		lea     eax, [esp+STACK_REGS_SIZE+4C0h+0Ch-424h]
		push    eax						// colPoint
		
		lea     eax, [esp+STACK_REGS_SIZE+4C0h+10h-3FCh]
		push    eax						// ppEntity
		
		call    _CheckForShootingVehicleOccupant
		add     esp, 14h
		
		RESTOREREGS()
	}

FIHP_FAIL:
	__asm
	{
		cmp     dword ptr [ebx], 6
		mov     esi, 1
		jnz     short loc_55D925
		mov     esi, 4
	}

loc_55D925:
	VARJMP(jmp_0x55D925);

}

static int jmp_0x562298 = AddressByVersion(0x562298, 0x5623C8, 0x562378);
static int var_0x6FACF8 = ADDR_THECAMERA+0x76;
void NAK FireM16_1stPersonPatch()
{
	__asm
	{
		add     esp, 34h
		test    al, al
		jz      short FM161PP_FAIL
		
		/*
		cmp     [esp+7Ch-24h], 0
		jz      short FM161PP_FAIL
		*/
	}
	
	__asm
	{
		STOREREGS()
		
		lea     eax, [esp+STACK_REGS_SIZE+7Ch-70h]
		push    eax						// end
		
		lea     eax, [esp+STACK_REGS_SIZE+7Ch+4-64h]
		push    eax						// start
		
		mov     eax, [esi]
		push    eax						// weapon
		
		lea     eax, [esp+STACK_REGS_SIZE+7Ch+0Ch-4Ch]
		push    eax						// colPoint
		
		lea     eax, [esp+STACK_REGS_SIZE+7Ch+10h-24h]
		push    eax						// ppEntity
		
		call    _CheckForShootingVehicleOccupant
		add     esp, 14h
		
		RESTOREREGS()
	}
	
FM161PP_FAIL:
	__asm
	{		
		mov       eax, var_0x6FACF8
		movzx     eax, byte ptr ds:[eax]
	}
	VARJMP(jmp_0x562298);
}


static int jmp_0x560EFE = AddressByVersion(0x560EFE, 0x56102E, 0x560FDE); 
void NAK FireShotgunPatch()
{
	__asm mov     [esp+200h-118h], eax

	__asm
	{
		call    FindPlayerPed
		cmp     dword ptr [esp+200h+4], eax
		jnz     FSP_FAIL
		
		push    ecx
		mov     ecx, TheCamera
		add     ecx, 1A4h
		call    CCam::Using3rdPersonMouseCam
		pop     ecx
		test    al, al
		jz      FSP_FAIL
	}
	
	__asm
	{
		STOREREGS()
		
		lea     eax, [esp+STACK_REGS_SIZE+200h-1B8h]
		push    eax						// end

		//mov     eax, [esp+STACK_REGS_SIZE+200h+4-234h]
		lea     eax, [esp+STACK_REGS_SIZE+200h+4-10Ch] 
		push    eax						// start
		
		//mov     eax, [esp+STACK_REGS_SIZE+200h+8-224h]
		//push    eax						// weapon
		
		mov     eax, [esp+STACK_REGS_SIZE+200h+8-1F0h]
		push    dword ptr [eax]                // weapon

		
		lea     eax, [esp+STACK_REGS_SIZE+200h+0Ch-1A0h]
		push    eax						// colPoint
		
		lea     eax, [esp+STACK_REGS_SIZE+200h+10h-178h]
		push    eax						// ppEntity
		
		call    _CheckForShootingVehicleOccupant
		add     esp, 14h
		
		RESTOREREGS()
	}

FSP_FAIL:
	VARJMP(jmp_0x560EFE);
}


void __cdecl KillPedInVehicle(CPed *This, CEntity *pDamager, eWeaponType Weapon, Bool bHeadShotted, Int32 anim, Float speed1, Float speed2)
{	
	CVehicle *pVehicle = This->m_pVehicle;
	if ( pVehicle )
	{
		if ( pVehicle->m_eVehicleType == VEHICLETYPE_CAR )
		{
			if ( pVehicle->pDriver == This )
			{
				if ( pVehicle->m_nState == STATUS_SIMPLE )
				{
					pVehicle->m_nState = STATUS_PHYSICS;
					CCarCtrl::SwitchVehicleToRealPhysics(pVehicle);
				}
				pVehicle->m_AutoPilot.m_nMaxSpeed = 0;
				pVehicle->m_AutoPilot.m_nDriverBehaviour = 0;
				pVehicle->m_AutoPilot.m_nSimpleAction = 5;
				pVehicle->m_AutoPilot.m_snSimpleActionTime = CTimer::m_snTimeInMilliseconds + 2000;
			}
		}

		if ( _CanPedExitCar(pVehicle, true) )
		{
			This->SetObjective((eObjective)OBJECTIVE_LV_CAR_DIE/*38*/, pVehicle);
		}
		else
		{
			This->m_fHealth = 0.0f;
			if ( pVehicle && pVehicle->pDriver == This )
			{
				This->SetRadioStation();
				pVehicle->m_nState = STATUS_ABANDONED;
			}

			This->SetDie(anim, speed1, speed2); // This->SetDie(v7, v58, v59);
		}

        
		for ( Int32 i = 0; i < 8; i++ )
		{
			CPed *pPassanger = pVehicle->pPassengers[i];
			if ( pPassanger && pPassanger != This && pDamager )
				pPassanger->ReactToAttack((CEntity*)pDamager);
		}

		CPed *pDriver = pVehicle->pDriver;
		if ( pDriver && pDriver != This && pDamager )
			pDriver->ReactToAttack((CEntity*)pDamager);


		if ( pDamager == FindPlayerPed() || pDamager && pDamager == FindPlayerVehicle() )
		{
			CDarkel::RegisterKillByPlayer(This, Weapon, bHeadShotted);
			This->m_pThreatPed = FindPlayerPed();
		}
		else
			CDarkel::RegisterKillNotByPlayer(This, Weapon);
	}
}

static int jmp_0x4EADDA = AddressByVersion(0x4EADDA, 0x4EAE8A, 0x4EAE1A);
void NAK InflictDamagePatch()
{
	__asm
	{
		STOREREGS()
		
		push    [esp+STACK_REGS_SIZE+30h+0h-1Ch]  // speed2
		push    [esp+STACK_REGS_SIZE+30h+4h-20h]  // speed1,
		push    ebx                               // anim
		push    [esp+STACK_REGS_SIZE+30h+0Ch-28h] // bHeadShotted,
		push    [esp+STACK_REGS_SIZE+30h+10h+8h]  // Weapon
		push    [esp+STACK_REGS_SIZE+30h+14h+4h]  // pDamager
		push    ebp                               // This
		call    KillPedInVehicle
		add     esp, 1Ch
		
		RESTOREREGS()
	}
	
	__asm mov     dword ptr [ebp+2C0h], 3F800000h
	VARJMP(jmp_0x4EADDA);
}

void __cdecl _ProcessObjective(CPed *pPed)
{
	if ( CTimer::m_snTimeInMilliseconds > pPed->m_nLeaveCarTimer )
	{
		if ( pPed->m_bInVehicle )
		{
			CVehicle *veh = pPed->m_pVehicle;
			if ( veh )
			{
				if ( pPed->m_nPedState != PED_EXIT_CAR 
					&& pPed->m_nPedState != PED_DRAG_FM_CAR
					&& pPed->m_nPedState != PED_EXIT_TRAIN )
				{
					if ( veh->m_eVehicleType == VEHICLETYPE_BOAT )
						SetExitBoat(pPed, pPed->m_pVehicle);
					else if ( veh->m_Flags1 & 2 )
						pPed->SetExitCar(veh, 0);
					else
					{
						UInt32 nEnterType = 15;
						if ( veh->pDriver == pPed )
							nEnterType = 15;
						else if ( veh->pPassengers[0] == pPed )
							nEnterType = 11;
						else if ( veh->pPassengers[1] == pPed )
							nEnterType = 16;
						else if ( veh->pPassengers[2] == pPed )
							nEnterType = 12;
						else
							nEnterType = 15;

						pPed->SetBeingDraggedFromCar(veh, nEnterType, false);
					}
				}
			}
		}
	}

	if ( (pPed->m_Flags3 >> 6) & 1 || pPed->m_nObjectiveTimer != 0 && CTimer::m_snTimeInMilliseconds > pPed->m_nObjectiveTimer )
	{
		pPed->RestorePreviousObjective();
		
		if ( pPed->m_nObjectiveTimer > CTimer::m_snTimeInMilliseconds || !pPed->m_nObjectiveTimer )
			pPed->m_nObjectiveTimer = CTimer::m_snTimeInMilliseconds - 1;
		
		if ( pPed->m_nReferenceType != 1 || pPed->m_bInVehicle )
		{
			if ( pPed->IsPedInControl() )
				pPed->RestorePreviousState();
		}
		else
			pPed->SetWanderPath(_cwrand() & 7);

		pPed->ClearAimFlag();
		pPed->ClearLookFlag();
	}
}

static int jmp_0x4DD510 = -1;
static int jmp_0x4DD40D = AddressByVersion(0x4DD40D, 0x4DD4BD, 0x4DD44D);
void NAK ProcessObjectivePatch()
{
	__asm
	{
		cmp     dword ptr [ebx+164h], OBJECTIVE_LV_CAR_DIE//26h //38
		jz      LEAVECAR_DIE
	}

	VARJMP(jmp_0x4DD510);
	
LEAVECAR_DIE:

	__asm
	{
		fstp    st

		STOREREGS()
		
		push    ebx
		call    _ProcessObjective
		add     esp, 4
		
		RESTOREREGS()
		
		add     esp, 500h
		pop     ebp
		pop     edi
		pop     esi
		pop     ebx
		retn
	}
}

static int jmp_0x4D82AF = AddressByVersion(0x4D82AF, 0x4D834F, 0x4D82DF);
static int jmp_0x4D82B7 = AddressByVersion(0x4D82B7, 0x4D8357, 0x4D82E7);
void NAK IsTemporaryObjectivePatch()
{
	__asm
	{
		cmp     eax, OBJECTIVE_LV_CAR_DIE //26h  //38
		jz      short loc_4D82AF
		cmp     eax, 0Eh
		jnz     short loc_4D82B7
		
	}

loc_4D82AF:
	VARJMP(jmp_0x4D82AF);

loc_4D82B7:
	VARJMP(jmp_0x4D82B7);

}

static int jmp_0x4D847D = AddressByVersion(0x4D847D, 0x4D851D, 0x4D84AD);
static int jmp_0x4D8493 = AddressByVersion(0x4D8493, 0x4D8533, 0x4D84C3);
static int jmp_0x4D8484 = AddressByVersion(0x4D8484, 0x4D8524, 0x4D84B4);
void NAK SetObjectivePatch1()
{
	__asm
	{
		cmp     eax, 7
		ja      short SOP1_TRUE
	}
	VARJMP(jmp_0x4D847D)
	
SOP1_TRUE:
	__asm
	{
		cmp     eax, OBJECTIVE_LV_CAR_DIE-6//20h  // 38 - 6
		jz      SOP1_CASE_38	
	}

	VARJMP(jmp_0x4D8493);

SOP1_CASE_38:
	VARJMP(jmp_0x4D8484);
}


static int jmp_0x4D843C = AddressByVersion(0x4D843C, 0x4D84DC, 0x4D846C);
static int jmp_0x4D8443 = AddressByVersion(0x4D8443, 0x4D84E3, 0x4D8473);
void NAK SetObjectivePatch2()
{
	__asm
	{
		cmp     eax, 1Ch
		ja      SOP2_TRUE
	}
	VARJMP(jmp_0x4D843C)
	
SOP2_TRUE:
	__asm
	{
		cmp     eax, OBJECTIVE_LV_CAR_DIE-6//20h  // 38 - 6
		jz      SOP2_CASE_38	
	}
	VARJMP(jmp_0x4D8493);
	
SOP2_CASE_38:
	VARJMP(jmp_0x4D8443);
}

static int jmp_0x4D8763 = AddressByVersion(0x4D8763, 0x4D8803, 0x4D8793);
static int jmp_0x4D8657 = AddressByVersion(0x4D8657, 0x4D86F7, 0x4D8687);
void NAK SetObjectivePatch3()
{
	__asm
	{
		cmp     eax, OBJECTIVE_LV_CAR_DIE-6//20h  // 38 - 6
		jz      SOP3_CASE_38	
	}

	VARJMP(jmp_0x4D8763);

SOP3_CASE_38:
	VARJMP(jmp_0x4D8657);
}


void __cdecl _PedSetDraggedOutCarCB(CAnimBlendAssociation *asoc, CPed *pPed)
{
	asoc->SetDeleteCallback(CPed::PedSetDraggedOutCarPositionCB, pPed);
	pPed->m_fHealth = 0.0f;
	pPed->SetDie(settings.nAnim1, 1000.0f, 0.5f);
}

static int jmp_0x4CF149 = AddressByVersion(0x4CF149, 0x4CF1E9, 0x4CF179);
static int jmp_0x4CF156 = AddressByVersion(0x4CF156, 0x4CF1F6, 0x4CF186);
void NAK PedSetDraggedOutCarCBPatch()
{
	__asm
	{
		cmp     dword ptr [esi+164h], OBJECTIVE_LV_CAR_DIE //26h  // 38
		jnz     short PSDOPCBP_FAIL
		
		STOREREGS()
		
		push    esi
		push    ebp
		call    _PedSetDraggedOutCarCB
		add     esp, 8
		
		RESTOREREGS()
		
		pop     ebp
		pop     esi
		pop     ebx
		retn
	}
	
PSDOPCBP_FAIL:
	__asm
	{
		test    bl, bl
		jz      short loc_4CF156
		mov     ecx, ebp
	}
	VARJMP(jmp_0x4CF149);
	
loc_4CF156:
	VARJMP(jmp_0x4CF156);
}


void __cdecl _PedSetOutCarCB(CPed *pPed)
{
	pPed->m_fHealth = 0.0f;
    pPed->SetDie(settings.nAnim1, 4.0f, 0.5f);
}

static int jmp_0x4CE963 = AddressByVersion(0x4CE963, 0x4CEA03, 0x4CE993);
void NAK PedSetOutCarCBPatch()
{
	__asm
	{
		cmp     dword ptr [ebp+164h], OBJECTIVE_LV_CAR_DIE
		jnz     short PSOCP_EXIT

		STOREREGS()
		
		push    ebp
		call   _PedSetOutCarCB
		add     esp, 4
		
		RESTOREREGS()
	}
	
PSOCP_EXIT:
	__asm mov     byte ptr [ebp+314h], 0
	VARJMP(jmp_0x4CE963);
}

void __cdecl _BeingDraggedFromCar(CPed *pPed)
{
	static float fTimeMul = 5.0f;
	
	if ( pPed->m_nObjective == OBJECTIVE_LV_CAR_DIE )
	{
		CVehicle *veh = pPed->m_pVehicle;
		if ( veh )
			veh->ProcessOpenDoor(pPed->m_nEnterType, settings.nAnim2, fTimeMul * pPed->m_pVehicleAnimBlendAssoc->m_fCurrentTime);
	}
}

void NAK BeingDraggedFromCarPatch()
{
	__asm
	{
		STOREREGS()
		
		push     ebx
		call    _BeingDraggedFromCar
		add     esp, 4
		
		RESTOREREGS()
	}
	
	__asm
	{
		pop     ebx
		retn
	}
}

static int jmp_0x4DF7C9 = AddressByVersion(0x4DF7C9, 0x4DF879, 0x4DF809);
static int jmp_0x4DF8F7 = AddressByVersion(0x4DF8F7, 0x4DF9A7, 0x4DF937);
void NAK PedAnimStepOutCarCBPatch()
{
	__asm
	{
		cmp     dword ptr [ebp+164h], OBJECTIVE_LV_CAR_DIE //26h  //38
		jnz     short PASOCCBP_FALI
		mov     byte ptr [esp+30h-28h], 0
	}
	
PASOCCBP_FALI:
	__asm
	{
		cmp     byte ptr [esp+30h-28h], 0
		jz      loc_4DF8F7
	}
	VARJMP(jmp_0x4DF7C9);
	
loc_4DF8F7:
	VARJMP(jmp_0x4DF8F7);
}

void NAK PedAnimStepOutCarCBPatch_Patched()
{
	__asm
	{
		cmp     dword ptr [ebp+164h], OBJECTIVE_LV_CAR_DIE //26h  //38
		jnz     short PASOCCBP_FALI
		mov     byte ptr [esp+20h-18h], 0
	}
	
PASOCCBP_FALI:
	__asm
	{
		cmp     byte ptr [esp+20h-18h], 0
		jz      loc_4DF8F7
	}
	VARJMP(jmp_0x4DF7C9);
	
loc_4DF8F7:
	VARJMP(jmp_0x4DF8F7);
}

static int jmp_0x4DF313 = AddressByVersion(0x4DF313, 0x4DF3C3, 0x4DF353);
static int jmp_0x4DF325 = AddressByVersion(0x4DF325, 0x4DF3D5, 0x4DF365);
void NAK PedAnimDoorCloseCBPatch()
{
	__asm
	{
		cmp     dword ptr [eax+164h], OBJECTIVE_LV_CAR_DIE //26h // 38
		jz      short PADCCBP
		cmp     dword ptr [eax+164h], 0Dh
		jnz     short loc_4DF325
	}
	
PADCCBP:
	VARJMP(jmp_0x4DF313); // call vtable
	
loc_4DF325:
	VARJMP(jmp_0x4DF325);
}

static int jmp_0x4EA4C0 = AddressByVersion(0x4EA4C0, 0x4EA570, 0x4EA500);
void NAK InflictDamagePatch2()
{
	__asm
	{
		mov     al, [ebp+51h]
		and     al, 1
		jnz     short loc_4EA4C0

		/**/
		cmp     byte ptr [ebp+314h], 0
		jz      short loc_525C65
		cmp     dword ptr [ebp+224h], 2Ch //STATES_DRIVING
		jz      short loc_4EA4C0
		/**/
		
loc_525C65:
		cmp     dword ptr [esp+30h+8h], 14h //WEAPONTYPE_DROWNING
		jz      short loc_4EA4C0
		
		xor     al, al
		add     esp, 20h
		pop     ebp
		pop     edi
		pop     esi
		pop     ebx
		retn    14h 
	}

loc_4EA4C0:
	VARJMP(jmp_0x4EA4C0);
}

Bool __cdecl _ReactToAttack(CPed *This, CEntity *pAttacker)
{
	/*
	if ( This->m_nPedType == PEDTYPE_GANG_PLAYER && pAttacker->m_nState == STATUS_PHYSICS && ((CPed *)pAttacker)->IsPlayer() )
	{
		if ( This->m_nPedState != PED_FALL )
			This->SetFall(15000, ((AnimationId)CGeneral::GetRandomNumberInRange(0.0f, 5.0f) + 13), 0);
	}
	
	else*/
	if ( This->m_nPedState == PED_DRIVING && This->m_bInVehicle && This->m_pVehicle )
	{
		CVehicle *pVehicle = This->m_pVehicle;
		CPed *pDriver = pVehicle->pDriver;

		if ( pDriver == This || pDriver && pDriver->m_nPedState == PED_DRIVING && pDriver->m_nObjective != OBJECTIVE_LV_CAR_DIE )
		{
			if ( pVehicle->m_nReferenceType == 1 && (pVehicle->m_nState == STATUS_SIMPLE || pVehicle->m_nState == STATUS_PHYSICS) && pVehicle->m_AutoPilot.m_nDriverBehaviour == 1 )
			{
				CCarCtrl::SwitchVehicleToRealPhysics(pVehicle);
				pVehicle->m_AutoPilot.m_nTrafficBehaviour = 2;
				pVehicle->m_AutoPilot.m_nMaxSpeed = Int32(60.0f * ((tHandlingData *)pVehicle->m_VehicleType)->m_TransmissionData.m_fMaxSpeed);
				pVehicle->m_nState = pVehicle->m_nState = STATUS_PHYSICS;
			}
			return true;
		}
	}

	return false;
}

static int jmp_0x4DDEDE = AddressByVersion(0x4DDEDE, 0x4DDF8E, 0x4DDF1E);
static int jmp_0x4DDF05 = AddressByVersion(0x4DDF05, 0x4DDFB5, 0x4DDF45);
void NAK ReactToAttackPatch1() //TODO check
{
	__asm
	{
		jz      short loc_4DDF05
		mov     al, byte ptr [ebp+50h]
		and     al, 7
		cmp     al, 3
		jnz     short loc_4DDF05
	}
	VARJMP(jmp_0x4DDEDE);

loc_4DDF05:
	__asm
	{
		STOREREGS()
		
		push    ebp
		push    ebx
		call    _ReactToAttack
		add     esp, 8

		RESTOREREGS()
		
		test    al, al
		
		jz      short RTAP1_EXIT // if false
		
		add     esp, 10h
		pop     ebp
		pop     esi
		pop     ebx
		retn    4  
	}

RTAP1_EXIT:
	VARJMP(jmp_0x4DDF05);
}

static int jmp_0x4DDF10 = AddressByVersion(0x4DDF10, 0x4DDFC0, 0x4DDF50);
static int jmp_0x4DDF2F = AddressByVersion(0x4DDF2F, 0x4DDFDF, 0x4DDF6F);
void NAK ReactToAttackPatch2()
{
	__asm
	{		
		mov     ecx, ebx
		call    CPed::IsPedInControl
		test    al, al
		
		jnz     short RTAP2_CHECKSTATUS
		cmp     dword ptr [ebx+224h], PED_DRIVING
		jnz     short RTAP2_EXIT
	}
	
RTAP2_CHECKSTATUS:
	VARJMP(jmp_0x4DDF10);

RTAP2_EXIT:
	VARJMP(jmp_0x4DDF2F);
}

void __cdecl _CarWindscreenShatters(CVehicle *pVehicle, Bool Unk)
{
	CColModel *pVehicleCol = getModelInfoPtrs(pVehicle->m_nModelIndex)->m_pColModel;
	if ( pVehicleCol->m_nNumTriangles < 1 )
		return;

	CColTriangle *GlassTriagle1 = NULL;
	Int16 glassTriagle1_ID = -1;
	CColTriangle *GlassTriagle2 = NULL;

	for ( Int32 i = 0; i < pVehicleCol->m_nNumTriangles; i++ )
	{
		if ( IsGlass(pVehicleCol->m_pTriangles[i].material) )
		{
			if ( GlassTriagle1 )
			{
				GlassTriagle2 = &pVehicleCol->m_pTriangles[i];
				break;
			}
			glassTriagle1_ID = i;
			GlassTriagle1 = &pVehicleCol->m_pTriangles[i];
		}
	}

    if ( GlassTriagle2 == NULL )
		return;

	CCollision::CalculateTrianglePlanes(pVehicleCol);

	if ( pVehicleCol->m_pTrianglePlanes == NULL )
		return;

	CVector norm = pVehicleCol->m_pTrianglePlanes[glassTriagle1_ID].normal;
	norm = Multiply3x3(pVehicle->m_sCoords, norm);
	
	CVector vAtNorm = CrossProduct(pVehicle->m_sCoords.right, norm);
	vAtNorm.Normalise();
	
	CVector vRightNorm = CrossProduct(norm, pVehicle->m_sCoords.at);
	vRightNorm.Normalise();

	CVector aVertices[6];

	aVertices[0] = ((CVector *)pVehicleCol->m_pVertices)[GlassTriagle1->a];
	aVertices[1] = ((CVector *)pVehicleCol->m_pVertices)[GlassTriagle1->b];
	aVertices[2] = ((CVector *)pVehicleCol->m_pVertices)[GlassTriagle1->c];
	aVertices[3] = ((CVector *)pVehicleCol->m_pVertices)[GlassTriagle2->a];
	aVertices[4] = ((CVector *)pVehicleCol->m_pVertices)[GlassTriagle2->b];
	aVertices[5] = ((CVector *)pVehicleCol->m_pVertices)[GlassTriagle2->c];
	
	for ( Int32 i = 0; i < 6; i++ )
		aVertices[i] = pVehicle->m_sCoords * aVertices[i];

	Float afAtDot[6];
	Float afRightDot[6];
	
	for ( Int32 i = 0; i < 6; i++ )
	{
		afAtDot[i] = DotProduct(aVertices[i], vAtNorm);
		afRightDot[i] = DotProduct(aVertices[i], vRightNorm);
	}
	
	Int16 nMinXYDotIndex = 0;
	Float fMinXYDot = afAtDot[0] + afRightDot[0];
	Float fMaxAtDot = afAtDot[0];
	Float fMaxRightDot = afRightDot[0];
	
	for ( Int32 i = 1; i < 6; i++ )
	{
		if ( afAtDot[i] + afRightDot[i] < fMinXYDot )
		{
			fMinXYDot = afAtDot[i] + afRightDot[i];
			nMinXYDotIndex = i;
		}
		
		if ( afAtDot[i] > fMaxAtDot )
			fMaxAtDot = afAtDot[i];
		
		if ( afRightDot[i] > fMaxRightDot )
			fMaxRightDot = afRightDot[i];
	}
        
	Float fBaseAt = fMaxAtDot - afAtDot[nMinXYDotIndex];
	Float fBaseRight = fMaxRightDot - afRightDot[nMinXYDotIndex];

	PlayOneShotScriptObject(114/*57*/, pVehicle->m_sCoords.pos);
		
	if ( settings.bViceGlassFx )
		CMyGlass::GeneratePanesForWindow(2, aVertices[nMinXYDotIndex], fBaseAt * vAtNorm, fBaseRight * vRightNorm, pVehicle->m_vecVelocity, aVertices[nMinXYDotIndex] + ( (0.5f * fBaseAt) * vAtNorm ) + ( (0.5f * fBaseRight) * vRightNorm ), 0.1f, false, false, 2, true);
	else
		CMyGlass::GeneratePanesForWindow(0, aVertices[nMinXYDotIndex], fBaseAt * vAtNorm, fBaseRight * vRightNorm, pVehicle->m_vecVelocity, aVertices[nMinXYDotIndex] + ( (0.5f * fBaseAt) * vAtNorm ) + ( (0.5f * fBaseRight) * vRightNorm ), 0.1f, false, false, 1, false);
	
}


static int jmp_0x5301EF = AddressByVersion(0x5301EF, 0x53042F, 0x5303BF);
void NAK SetPanelDamagePatch()
{
	__asm
	{
		cmp     byte ptr [esp+8h+0Ch], 0
		jz      short SPDP_FAIL

		cmp     dword ptr [esp+8h+8h], 4
		jnz     short SPDP_FAIL
	}
	
	__asm
	{
		STOREREGS()
		
		push    0
		push    ebx
		call    _CarWindscreenShatters
		add     esp, 8
		
		RESTOREREGS()
	}
	
SPDP_FAIL:
	__asm mov     eax, [ebx+ebp*4+37Ch]
	VARJMP(jmp_0x5301EF);
}

class PedWindshield
{
public:
	Bool bCanBeShotInVeh;
 
	PedWindshield() {}

	PedWindshield(CPed *) : bCanBeShotInVeh(true) {}
	~PedWindshield() { }

	void Store(bool) {}
	void Restore() {}
};

static PedExtendedData<PedWindshield> pedShootInVeh;

Bool __cdecl CAN_BE_SHOT_IN_VEHICLE_Func(CPed *This)
{
	// Log("CHECK %d", CPools::ms_pPedPool->GetIndex(This) >> 8);
	//return m_aPedShootInVeh[CPools::ms_pPedPool->GetIndex(This) >> 8];
	//return true;
	return pedShootInVeh.Get(This).bCanBeShotInVeh == true;
}

#define CLEO_VERSION_MAIN    2
#define CLEO_VERSION_MAJOR   0
#define CLEO_VERSION_MINOR   0
#define CLEO_VERSION_BINARY  0
#define CLEO_VERSION ((CLEO_VERSION_MAIN << 16)|(CLEO_VERSION_MAJOR << 12)|(CLEO_VERSION_MINOR << 8)|(CLEO_VERSION_BINARY))

tScriptVar *Params;

namespace _Opcodes
{
	void EXP __cdecl SET_CHAR_CAN_BE_SHOT_IN_VEHICLE(int ped, int value)
	{
		// Log("SET %d", ped >> 8);
	/*
		if ( value )
			m_aPedShootInVeh[ped >> 8] = true;
		else
			m_aPedShootInVeh[ped >> 8] = false;
		*/
		CPed *pPed = CPools::ms_pPedPool->GetAt(ped);
		if ( pPed )
		{
			if ( value )
				pedShootInVeh.Get(pPed).bCanBeShotInVeh = true;
			else
				pedShootInVeh.Get(pPed).bCanBeShotInVeh = false;
		}
	}
	
	eOpcodeResult WINAPI _054A_SET_CHAR_CAN_BE_SHOT_IN_VEHICLE(CScript *script)
	{
		script->Collect(2);
		SET_CHAR_CAN_BE_SHOT_IN_VEHICLE(Params[0].nVar, Params[1].nVar);
		return OR_CONTINUE;
	}
};


FARPROC CleoGetProc(LPCSTR lpProcName)
{
	HMODULE cleo = GetModuleHandleA("III.CLEO.asi");
	if(cleo == NULL) cleo = GetModuleHandleA("IIII.CLEO.dll");

	if(cleo)
		return GetProcAddress(cleo, lpProcName);
	
	return NULL;
}

Bool IsCleoInstalled()
{
	if ( GetModuleHandleA("III.CLEO.asi") == NULL && GetModuleHandleA("IIII.CLEO.dll") == NULL )
		return false;
	else
		return true;
}


tScriptVar* CLEO_GetParamsAddress()
{
	return ((tScriptVar* (__cdecl *)())CleoGetProc("?CLEO_GetParamsAddress@@YAPATtScriptVar@@XZ"))();
}

char* CLEO_GetScriptSpaceAddress()
{
	return ((char* (__cdecl *)())CleoGetProc("?CLEO_GetScriptSpaceAddress@@YAPADXZ"))();
}

unsigned CLEO_GetVersion()
{
	return ((unsigned (__cdecl *)())CleoGetProc("?CLEO_GetVersion@@YAIXZ"))();
}

void CScript::Collect(unsigned int numParams)
{
	((void (__thiscall *)(CScript*, unsigned int))CleoGetProc("?Collect@CScript@@QAEXI@Z"))(this, numParams);
}

void CScript::Collect(unsigned int* pIp, unsigned int numParams)
{
	((void (__thiscall *)(CScript*, unsigned int*, unsigned int))CleoGetProc("?Collect@CScript@@QAEXPAII@Z"))(this, pIp, numParams);
}

int CScript::CollectNextWithoutIncreasingPC(unsigned int ip)
{
	return ((int (__thiscall *)(CScript*, unsigned int))CleoGetProc("?CollectNextWithoutIncreasingPC@CScript@@QAEHI@Z"))(this, ip);
}

eParamType CScript::GetNextParamType()
{
	return ((eParamType (__thiscall *)(CScript*))CleoGetProc("?GetNextParamType@CScript@@QAE?AW4eParamType@@XZ"))(this);
}

void* CScript::GetPointerToScriptVariable()
{
	return ((void* (__thiscall *)(CScript*))CleoGetProc("?GetPointerToScriptVariable@CScript@@QAEPAXXZ"))(this);
}

void CScript::JumpTo(int address)
{
	((void (__thiscall *)(CScript*, int))CleoGetProc("?JumpTo@CScript@@QAEXH@Z"))(this, address);
}

void CScript::ReadShortString(char* out)
{
	((void (__thiscall *)(CScript*, char*))CleoGetProc("?ReadShortString@CScript@@QAEXPAD@Z"))(this, out);
}

bool Opcodes::RegisterOpcode(unsigned short id, Opcode func)
{
	return ((bool (__cdecl *)(unsigned short, Opcode))CleoGetProc("?RegisterOpcode@Opcodes@@SA_NGP6G?AW4eOpcodeResult@@PAVCScript@@@Z@Z"))(id, func);
}

void CScript::Store(unsigned int numParams)
{
	((void (__thiscall *)(CScript*, unsigned int))CleoGetProc("?Store@CScript@@QAEXI@Z"))(this, numParams);
}

void CScript::UpdateCompareFlag(bool result)
{
	((void (__thiscall *)(CScript*, bool))CleoGetProc("?UpdateCompareFlag@CScript@@QAEX_N@Z"))(this, result);
}

void CheckAsi()
{
	if ( !IsCleoInstalled() )
		return;

	if (CLEO_GetVersion() < CLEO_VERSION)
	{
		MessageBox(HWND_DESKTOP, "An incorrect version of CLEO was loaded.", "IIIBreakableWindshields.asi", MB_ICONERROR);
		return;
	}
	
	Params = CLEO_GetParamsAddress();
		
	Opcodes::RegisterOpcode(settings.nOpcode, _Opcodes::_054A_SET_CHAR_CAN_BE_SHOT_IN_VEHICLE);
}

static char * _EGGVar;
void EGG(char *a)
{
	_EGGVar = a;
	__asm mov eax, eax
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		EGG("Get Out Of Here, Stalker!");
		
		CPatch::RedirectJump(AddressByVersion(0x558674, 0x5587A4, 0x558754), BulletInfoPatch);
		CPatch::RedirectJump(AddressByVersion(0x55D916, 0x55DA46, 0x55D9F6), FireInstantHitPatch);
		CPatch::RedirectJump(AddressByVersion(0x56228E, 0x5623BE, 0x56236E), FireM16_1stPersonPatch);
		if ( settings.bShotgun )
			CPatch::RedirectJump(AddressByVersion(0x560EF7, 0x561027, 0x560FD7), FireShotgunPatch);
		
		CPatch::RedirectJump(AddressByVersion(0x4EADD0, 0x4EAE80, 0x4EAE10), InflictDamagePatch);

		CPatch::RedirectJump(AddressByVersion(0x4D82AA, 0x4D834A, 0x4D82DA), IsTemporaryObjectivePatch); //CPed::IsTemporaryObjective patch - OBJECTIVE_LV_CAR_DIE
		
	
		CPatch::RedirectJump(AddressByVersion(0x4D8478, 0x4D8518, 0x4D84A8), SetObjectivePatch1);     //CPatch::RedirectJA(0x4D847B, SetObjectivePatch4);
		CPatch::RedirectJump(AddressByVersion(0x4D8437, 0x4D84D7, 0x4D8467), SetObjectivePatch2);		//0x4D843C patch - OBJECTIVE_LV_CAR_DIE
		CPatch::RedirectJA(AddressByVersion(0x4D84FA, 0x4D859A, 0x4D852A), SetObjectivePatch3);		//0x4D8657 patch - OBJECTIVE_LV_CAR_DIE

		CPatch::RedirectJump(AddressByVersion(0x4CF143, 0x4CF1E3, 0x4CF173), PedSetDraggedOutCarCBPatch);	//CPed::PedSetDraggedOutCarCB
		CPatch::RedirectJump(AddressByVersion(0x4CE95C, 0x4CE9FC, 0x4CE98C), PedSetOutCarCBPatch);		//CPed::PedSetOutCarCB
		CPatch::RedirectJump(AddressByVersion(0x4E0913, 0x4E09C3, 0x4E0953), BeingDraggedFromCarPatch);	//CPed::BeingDraggedFromCar
		CPatch::RedirectJump(AddressByVersion(0x4DF7BE, 0x4DF86E, 0x4DF7FE), FunctionByVersion(PedAnimStepOutCarCBPatch, PedAnimStepOutCarCBPatch_Patched, PedAnimStepOutCarCBPatch_Patched));	//CPed::PedAnimStepOutCarCB
		CPatch::RedirectJump(AddressByVersion(0x4DF30A, 0x4DF3BA, 0x4DF34A), PedAnimDoorCloseCBPatch);	//CPed::PedAnimDoorCloseCB

		CPatch::JA(AddressByVersion(0x4D965D, 0x4D96FD, 0x4D968D), ProcessObjectivePatch, jmp_0x4DD510);
		
		CPatch::RedirectJump(AddressByVersion(0x4EA4A1, 0x4EA551, 0x4EA4E1), InflictDamagePatch2);

		CPatch::RedirectJump(AddressByVersion(0x4DDED3, 0x4DDF83, 0x4DDF13), ReactToAttackPatch1);
		CPatch::RedirectJump(AddressByVersion(0x4DDF07, 0x4DDFB7, 0x4DDF47), ReactToAttackPatch2);
		
		
		if ( settings.bGlassFx )
			CPatch::RedirectJump(AddressByVersion(0x5301E8, 0x530428, 0x5303B8), SetPanelDamagePatch);

		if ( settings.bGlassFx )
		{
			CHook::Register(F_ReInitGameObjectVariables, CMyGlass::Init);
			CHook::Register(F_Initialise, CMyGlass::Init);
			CHook::Register(F_RenderEffects, CMyGlass::Render);
			CHook::Register(F_Process, CMyGlass::Update);
		}
		
		pedShootInVeh.Init();
		if ( settings.bCleoIntegration )
			CHook::Register(F_ASICheck, CheckAsi);
		
		//CGlass MathFix
		// CPatch::SetChar(0x502FDB, 0x2C);	//0x502FDB	0x1	30 	2C
		// CPatch::SetChar(0x502FE6, 0x2C);	//0x502FE6	0x1	30 	2C
		// CPatch::SetChar(0x502FF1, 0x2C);	//0x502FF1	0x1	30 	2C
	}
	return TRUE;
}

namespace BreakableWindshields
{
	void EXP Set_CAN_BE_SHOT_IN_VEHICLE_CallBack(t_CAN_BE_SHOT_IN_VEHICLE_CB func)
	{
		_CAN_BE_SHOT_IN_VEHICLECB = func;
	}
	
	t_CAN_BE_SHOT_IN_VEHICLE_CB EXP Get_CAN_BE_SHOT_IN_VEHICLE_CallBack()
	{
		return _CAN_BE_SHOT_IN_VEHICLECB;
	}
};