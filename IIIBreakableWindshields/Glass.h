#pragma once

#define MAX_GLASS_PANES 45

Float CentersWithTriangle[5][2];

Float CoorsWithTriangle[5][6] = 
{
	{ 0.0f, 0.0f, 0.0f, 1.0f, 0.4f, 0.5f },
	{ 0.0f, 1.0f, 1.0f, 1.0f, 0.4f, 0.5f },
	{ 0.0f, 0.0f, 0.4f, 0.5f, 0.7f, 0.0f },
	{ 0.7f, 0.0f, 0.4f, 0.5f, 1.0f, 1.0f },
	{ 0.7f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f }
};

int TempBufferIndicesStoredHiLight;
int TempBufferVerticesStoredHiLight;
RwIm3DVertex TempBufferRenderVertices[512];
RwImVertexIndex TempBufferRenderIndexList[1024];
int TempBufferIndicesStoredShattered;
int TempBufferVerticesStoredShattered;
int TempBufferIndicesStoredReflection;
int TempBufferVerticesStoredReflection;

class CMyFallingGlassPane
{
public:
	CMatrix m_Matrix;
	CVector m_vecVelocity;
	CVector field_54;
	Int32 m_nTimer;
	Float m_fGroundZ;
	Float field_68;
	UInt8 m_nTriagleIndex;
	Bool m_bUpdate;
	Bool field_6E;
	
	Bool m_bCarGlass;
	
	char _pad0[2];

	void Update();
	void Render();
};

class CMyGlass
{
public:

	static CMyFallingGlassPane aGlassPanes[MAX_GLASS_PANES];
	
	static void Init();
	static void Update();
	static void Render();
	static CMyFallingGlassPane *FindFreePane();
	static void GeneratePanesForWindow(int glassType,
										CVector vecPos, CVector vecAt, CVector vecRight, CVector vecSpeed, CVector vecEnd,
										Float fVelocityMul, Bool b1, Bool b2,
										Int32 CarGlassCycles, Bool b3);
	static void RenderHiLightPolys();
	static void RenderShatteredPolys();
	static void RenderReflectionPolys();
};

void CMyFallingGlassPane::Update()
{
	if ( CTimer::m_snTimeInMilliseconds >= m_nTimer )
	{
		if ( m_bCarGlass )
			m_Matrix.pos += m_vecVelocity * 0.35f * CTimer::ms_fTimeStep;
		else
			m_Matrix.pos += m_vecVelocity * CTimer::ms_fTimeStep;
		
		if ( m_bCarGlass )
			m_vecVelocity.z -= 0.01f * CTimer::ms_fTimeStep;
		else
			m_vecVelocity.z -= 0.02f * CTimer::ms_fTimeStep;
		
		m_Matrix.right += CrossProduct(field_54, m_Matrix.right);	
		m_Matrix.up += CrossProduct(field_54, m_Matrix.up);
		m_Matrix.at += CrossProduct(field_54, m_Matrix.at);
	
		if ( m_Matrix.pos.z < m_fGroundZ )
		{
			m_bUpdate = false;	  
			PlayOneShotScriptObject(117, CVector(m_Matrix.pos.x, m_Matrix.pos.y, m_fGroundZ));
			
			if ( !m_bCarGlass )
			{
				static Int32 nFrameGen = 0;
				
				for ( Int32 i = 0; i < 4; i++ )
				{
					RwRGBA color = { 255, 255, 255, 255 };
					CVector direction(CGeneral::GetRandomNumberInRange(-0.35f, 0.35f), CGeneral::GetRandomNumberInRange(-0.35f, 0.35f), CGeneral::GetRandomNumberInRange(0.05f, 0.20f));
					++nFrameGen;
					CParticle::AddParticle(PARTICLE_CAR_DEBRIS, CVector(m_Matrix.pos.x, m_Matrix.pos.y, m_fGroundZ), direction, NULL, CGeneral::GetRandomNumberInRange(0.02f, 0.20f), color, (Int32)CGeneral::GetRandomNumberInRange(-40.0f, 40.0f), 0, nFrameGen & 3, 500);
				}
			}
		}
	}
}

void CMyFallingGlassPane::Render()
{
	Float fDistToCamera = (TheCamera.m_sCoords.pos - m_Matrix.pos).Magnitude();

	CVector vecUp = m_Matrix.up;
	vecUp.Normalise();
	
	Int32 nAlphaWithNormal = CGlass::CalcAlphaWithNormal(&vecUp);

	Int32 nTimeLeft = CTimer::m_snTimeInMilliseconds - m_nTimer;
	Int32 nTime;

	if ( nTimeLeft < 0 || nTimeLeft <= 500 )
		nTime = max(nTimeLeft, 0);
	else
		nTime = 500;

	UInt8 nColor1 = Int32(Float(nTime) * 0.002f * Float(nAlphaWithNormal));

	if ( ( TempBufferIndicesStoredHiLight >= 505 || TempBufferVerticesStoredHiLight >= 252 ) && TempBufferVerticesStoredHiLight )
		CMyGlass::RenderHiLightPolys();
	
	if ( m_bCarGlass && nColor1 < 64 )
		nColor1 = 64;

	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 0], nColor1, nColor1, nColor1, nColor1);
	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 1], nColor1, nColor1, nColor1, nColor1);
	RwIm3DVertexSetRGBA(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 2], nColor1, nColor1, nColor1, nColor1);
	RwIm3DVertexSetU(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 0], 0.5f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 0], 0.5f);
	RwIm3DVertexSetU(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 1], 0.5f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 1], 0.6f);
	RwIm3DVertexSetU(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 2], 0.6f);
	RwIm3DVertexSetV(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 2], 0.6f);


	CVector vecPos1 = m_Matrix * CVector(CoorsWithTriangle[m_nTriagleIndex][0] - CentersWithTriangle[m_nTriagleIndex][0], 0.0f, CoorsWithTriangle[m_nTriagleIndex][1] - CentersWithTriangle[m_nTriagleIndex][1]);
	CVector vecPos2 = m_Matrix * CVector(CoorsWithTriangle[m_nTriagleIndex][2] - CentersWithTriangle[m_nTriagleIndex][0], 0.0f, CoorsWithTriangle[m_nTriagleIndex][3] - CentersWithTriangle[m_nTriagleIndex][1]);
	CVector vecPos3 = m_Matrix * CVector(CoorsWithTriangle[m_nTriagleIndex][4] - CentersWithTriangle[m_nTriagleIndex][0], 0.0f, CoorsWithTriangle[m_nTriagleIndex][5] - CentersWithTriangle[m_nTriagleIndex][1]);
	
	RwIm3DVertexSetPos(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 0], vecPos1.x, vecPos1.y, vecPos1.z);
	RwIm3DVertexSetPos(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 1], vecPos2.x, vecPos2.y, vecPos2.z);	
	RwIm3DVertexSetPos(&TempBufferRenderVertices[TempBufferVerticesStoredHiLight + 2], vecPos3.x, vecPos3.y, vecPos3.z);

	
	TempBufferRenderIndexList[TempBufferIndicesStoredHiLight + 0] = TempBufferVerticesStoredHiLight;
	TempBufferRenderIndexList[TempBufferIndicesStoredHiLight + 1] = TempBufferVerticesStoredHiLight + 1;
	TempBufferRenderIndexList[TempBufferIndicesStoredHiLight + 2] = TempBufferVerticesStoredHiLight + 2;
	TempBufferRenderIndexList[TempBufferIndicesStoredHiLight + 3] = TempBufferVerticesStoredHiLight;
	TempBufferRenderIndexList[TempBufferIndicesStoredHiLight + 4] = TempBufferVerticesStoredHiLight + 2;
	TempBufferRenderIndexList[TempBufferIndicesStoredHiLight + 5] = TempBufferVerticesStoredHiLight + 1;

	TempBufferVerticesStoredHiLight += 3;
	TempBufferIndicesStoredHiLight += 6;
	
	if ( field_6E )
	{
		if ( ( TempBufferIndicesStoredShattered >= 761 || TempBufferVerticesStoredShattered >= 380 ) && TempBufferVerticesStoredShattered != 256 )
			CMyGlass::RenderShatteredPolys();

		UInt8 nColor2 = 140;
		
		if ( fDistToCamera > 30.0f )
			nColor2 = Int32((1.0f - (fDistToCamera - 30.0f) * 4.0f * 0.025f) * Float(140));
	
		RwIm3DVertexSetRGBA(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 0], nColor2, nColor2, nColor2, nColor2);
		RwIm3DVertexSetRGBA(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 1], nColor2, nColor2, nColor2, nColor2);
		RwIm3DVertexSetRGBA(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 2], nColor2, nColor2, nColor2, nColor2);
		RwIm3DVertexSetU(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 0], 4.0f * CoorsWithTriangle[m_nTriagleIndex][0] * field_68);
		RwIm3DVertexSetV(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 0], 4.0f * CoorsWithTriangle[m_nTriagleIndex][1] * field_68);
		RwIm3DVertexSetU(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 1], 4.0f * CoorsWithTriangle[m_nTriagleIndex][2] * field_68);
		RwIm3DVertexSetV(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 1], 4.0f * CoorsWithTriangle[m_nTriagleIndex][3] * field_68);
		RwIm3DVertexSetU(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 2], 4.0f * CoorsWithTriangle[m_nTriagleIndex][4] * field_68);
		RwIm3DVertexSetV(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 2], 4.0f * CoorsWithTriangle[m_nTriagleIndex][5] * field_68);
		RwIm3DVertexSetPos(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 0], vecPos1.x, vecPos1.y, vecPos1.z);
		RwIm3DVertexSetPos(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 1], vecPos2.x, vecPos2.y, vecPos2.z);
		RwIm3DVertexSetPos(&TempBufferRenderVertices[TempBufferVerticesStoredShattered + 2], vecPos3.x, vecPos3.y, vecPos3.z);
	
		TempBufferRenderIndexList[TempBufferIndicesStoredShattered + 0] = TempBufferVerticesStoredShattered - 256;
		
		TempBufferRenderIndexList[TempBufferIndicesStoredShattered + 1] = TempBufferVerticesStoredShattered - 255;
		TempBufferRenderIndexList[TempBufferIndicesStoredShattered + 2] = TempBufferVerticesStoredShattered - 254;
		TempBufferRenderIndexList[TempBufferIndicesStoredShattered + 3] = TempBufferVerticesStoredShattered - 256;
		TempBufferRenderIndexList[TempBufferIndicesStoredShattered + 4] = TempBufferVerticesStoredShattered - 254;
		TempBufferRenderIndexList[TempBufferIndicesStoredShattered + 5] = TempBufferVerticesStoredShattered - 255;
		
		TempBufferIndicesStoredShattered += 6;
		TempBufferVerticesStoredShattered += 3;
	}
}


CMyFallingGlassPane CMyGlass::aGlassPanes[MAX_GLASS_PANES];
	
void CMyGlass::Init()
{
	for ( Int32 i = 0; i < MAX_GLASS_PANES; i++ )
		aGlassPanes[i].m_bUpdate = false; 
	
	for ( Int32 i = 0; i < 5; i++ )
	{
		CentersWithTriangle[i][0] = (CoorsWithTriangle[i][0] + CoorsWithTriangle[i][2] + CoorsWithTriangle[i][4]) * 0.33333f;
		CentersWithTriangle[i][1] = (CoorsWithTriangle[i][1] + CoorsWithTriangle[i][3] + CoorsWithTriangle[i][5]) * 0.33333f;
	}
}

void CMyGlass::Update()
{
	if ( !CTimer::m_UserPause && !CTimer::m_CodePause )
	{
		for ( Int32 i = 0; i < MAX_GLASS_PANES; i++ )
		{
			if ( aGlassPanes[i].m_bUpdate )
				aGlassPanes[i].Update();
		}
	}
}

void CMyGlass::Render()
{
	
	TempBufferVerticesStoredHiLight = 0;
	TempBufferIndicesStoredReflection = 768;
	TempBufferIndicesStoredHiLight = 0;
	TempBufferVerticesStoredReflection = 384;
	TempBufferVerticesStoredShattered = 256;
	TempBufferIndicesStoredShattered = 512;

	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void *)FALSE);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void *)rwFILTERLINEAR);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void *)TRUE);		 
	RwRenderStateSet(rwRENDERSTATEFOGCOLOR, (void *)RWRGBALONG(CTimeCycle::m_nCurrentFogColourRed, CTimeCycle::m_nCurrentFogColourGreen, CTimeCycle::m_nCurrentFogColourBlue, 255));
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);

	for ( Int32 i = 0; i < MAX_GLASS_PANES; i++ )
	{
		if ( aGlassPanes[i].m_bUpdate )
			aGlassPanes[i].Render();
	}
	
	RenderHiLightPolys();
	RenderShatteredPolys();
	RenderReflectionPolys();
	
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void *)FALSE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void *)FALSE);
}

CMyFallingGlassPane *CMyGlass::FindFreePane()
{
	for ( Int32 i = 0; i < MAX_GLASS_PANES; i++ )
	{
		if ( !aGlassPanes[i].m_bUpdate )
			return &aGlassPanes[i];
	}
	
	return NULL;
}

void CMyGlass::GeneratePanesForWindow(int glassType,
									CVector vecPos, CVector vecAt, CVector vecRight, CVector vecSpeed, CVector vecEnd,
									Float fVelocityMul, Bool b1, Bool b2,
									Int32 CarGlassCycles, Bool b3)
{
	Float fAtMag = vecAt.Magnitude();
	Float fRightMag = vecRight.Magnitude();

	Float v13 = max(0.75f + fAtMag, 1.0f);
	Float v14 = max(0.75f + fRightMag, 1.0f);

	UInt32 nAtCycles = min(CarGlassCycles * v13, 3);
	UInt32 nRightCycles = min(CarGlassCycles * v14, 3);
	
	if ( b2 )
	{
		if ( nAtCycles > 1 )
			nAtCycles = 1;

		if ( nRightCycles > 1 )
			nRightCycles = 1;
	}
	
	Float fAtMagPerCycle = fAtMag / nAtCycles;
	Float fRightMagPerCycle = fRightMag / nRightCycles;
	
	bool bZResult;
	Float fGroundZ = CWorld::FindGroundZFor3DCoord(vecPos.x, vecPos.y, vecPos.z, &bZResult);
	if ( !bZResult )
		fGroundZ = vecPos.z - 2.0f;
	
	for ( Int32 nAt = 0; nAt < nAtCycles; nAt++ )
	{
		Float fCurAt = nAt * fAtMagPerCycle;
		
		for ( Int32 nRight = 0; nRight < nRightCycles; nRight++ )
		{
			Float fCurRight = nRight * fRightMagPerCycle;
				
			for ( Int32 i = 0; i < 5; i++ )
			{
				CMyFallingGlassPane *Pane = FindFreePane();
				if ( Pane )
				{	
					Pane->m_nTriagleIndex = i;
					
					Pane->m_Matrix.right = (vecRight * fRightMagPerCycle) / fRightMag;
					
					//Pane->m_Matrix.at = (vecAt * fAtMagPerCycle) / fRightMag; //bug by R*
					if ( settings.bFixLongDebrisBugByRockstar )
						Pane->m_Matrix.at = (vecAt * fAtMagPerCycle) / fAtMag;
					else
						Pane->m_Matrix.at = (vecAt * fAtMagPerCycle) / fRightMag;
					
					CVector vecUp = CrossProduct(Pane->m_Matrix.right, Pane->m_Matrix.at);
					vecUp.Normalise();
					Pane->m_Matrix.up = vecUp;
					
					Pane->m_Matrix.pos = vecRight / fRightMag * (fRightMagPerCycle * CentersWithTriangle[i][0] + fCurRight)
										+ vecAt / fAtMag * (fAtMagPerCycle * CentersWithTriangle[i][1] + fCurAt)
										+ vecPos;
					
					Pane->m_vecVelocity.x = (float)((_cwrand() & 127) - 64) * 0.0015f + vecSpeed.x;
					Pane->m_vecVelocity.y = (float)((_cwrand() & 127) - 64) * 0.0015f + vecSpeed.y;
					Pane->m_vecVelocity.z = vecSpeed.z + 0.0f;

					if ( fVelocityMul != 0.0f )
					{
						CVector newVel = Pane->m_Matrix.pos - vecEnd;
						newVel.Normalise();
						Pane->m_vecVelocity = fVelocityMul * newVel + Pane->m_vecVelocity;
					}

					Pane->field_54.x = (float)((_cwrand() & 127) - 64) * 0.002f;
					Pane->field_54.y = (float)((_cwrand() & 127) - 64) * 0.002f;
					Pane->field_54.z = (float)((_cwrand() & 127) - 64) * 0.002f;
					
					switch ( glassType )
					{
						case 0:
							Pane->m_nTimer = CTimer::m_snTimeInMilliseconds;
							break;
						case 2:
							Pane->m_nTimer = CTimer::m_snTimeInMilliseconds;
							break;
						case 1:
							Pane->m_nTimer = Int32(100.0f * (Pane->m_Matrix.pos - vecEnd).Magnitude() + Float(CTimer::m_snTimeInMilliseconds));
							break;
					}
					
					Pane->m_fGroundZ = fGroundZ;
					Pane->field_6E = b1;
					Pane->field_68 = fAtMagPerCycle;
					Pane->m_bUpdate = true;
					Pane->m_bCarGlass = b3;
				}
			}
		}
	}
}

void CMyGlass::RenderHiLightPolys()
{
	if ( TempBufferVerticesStoredHiLight )
	{
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void *)gpShadowExplosionTex->raster);
		
		if ( RwIm3DTransform(TempBufferRenderVertices, TempBufferVerticesStoredHiLight, NULL, rwIM3D_VERTEXUV) )
		{
			RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, TempBufferRenderIndexList, TempBufferIndicesStoredHiLight);
			RwIm3DEnd();
		}
		
		TempBufferVerticesStoredHiLight = 0;
		TempBufferIndicesStoredHiLight = 0;
	}
}

void CMyGlass::RenderShatteredPolys()
{
	if ( TempBufferVerticesStoredShattered != 256 )
	{
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void *)gpCrackedGlassTex->raster);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);

		if ( RwIm3DTransform(&TempBufferRenderVertices[256], TempBufferVerticesStoredShattered - 256, NULL, rwIM3D_VERTEXUV) )
		{
			RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, &TempBufferRenderIndexList[512], TempBufferIndicesStoredShattered - 512);
			RwIm3DEnd();
		}
		
		TempBufferIndicesStoredShattered = 512;
		TempBufferVerticesStoredShattered = 256;
	}
}

void CMyGlass::RenderReflectionPolys()
{
	if ( TempBufferVerticesStoredReflection != 384 )
	{
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void *)gpShadowHeadLightsTex->raster);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);

		if ( RwIm3DTransform(&TempBufferRenderVertices[384], TempBufferVerticesStoredReflection - 384, NULL, rwIM3D_VERTEXUV) )
		{
			RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, &TempBufferRenderIndexList[768], TempBufferIndicesStoredReflection - 768);
			RwIm3DEnd();
		}
		
		TempBufferIndicesStoredReflection = 768;
		TempBufferVerticesStoredReflection = 384;
	}
}