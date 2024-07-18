	__declspec(align(16)) struct VSInputPerFrame
	{
		void Init(DirectX::XMMATRIX* pWorld, DirectX::XMMATRIX* pView, DirectX::XMMATRIX* pProjection, uint32 dwBaseColor)
		{
			mWorld = DirectX::XMMatrixTranspose(*pWorld);
			mView = DirectX::XMMatrixTranspose(*pView);
			mProjection = DirectX::XMMatrixTranspose(*pProjection);

			vBaseColor = DirectX::XMVectorSet(
				RGBA_GETFR(dwBaseColor) / 255.0f,
				RGBA_GETFG(dwBaseColor) / 255.0f,
				RGBA_GETFB(dwBaseColor) / 255.0f,
				RGBA_GETFA(dwBaseColor) / 255.0f);
		}

		DirectX::XMMATRIX	mWorld;
		DirectX::XMMATRIX	mView;
		DirectX::XMMATRIX	mProjection;
		DirectX::XMVECTOR	vBaseColor;
	};
	
	
	
	XMVECTOR vUp = { 0.0f, 1.0f, 0.0f, 0.0f };
	XMVECTOR vPos = { 0.0f, 0.0f, 0.0f, 0.0f };
	XMVECTOR vLookAt = { 0.0f, 0.0f, 1.0f, 0.0f };
	XMMATRIX mWorld = XMMatrixIdentity();
	XMMATRIX mRotation = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f);
	vLookAt = XMVector3TransformCoord(vLookAt, mRotation);
	vUp = XMVector3TransformCoord(vUp, mRotation);
	vLookAt = vPos + vLookAt;
	XMMATRIX mView = XMMatrixLookAtLH(vPos, vLookAt, vUp);
	XMMATRIX mProjection = XMMatrixOrthographicOffCenterLH(0.0f, (float)g_dwScreenWidth, (float)g_dwScreenHeight, 0.0f, 0.0f, 1.0f);
	
	
	
	LTRect rcCurSrcRect;
	if (pRequest->m_pSrcRect == nullptr)
		rcCurSrcRect = LTRect{ 0, 0, (int)pSurface->m_dwWidth, (int)pSurface->m_dwHeight };
	else
		rcCurSrcRect = *pRequest->m_pSrcRect;

	LTRect rcCurDestRect;
	if (pRequest->m_pDestRect == nullptr)
		rcCurDestRect = LTRect{ 0, 0, (int)g_D3DDevice.GetModeInfo()->m_dwWidth, (int)g_D3DDevice.GetModeInfo()->m_dwHeight };
	else
		rcCurDestRect = *pRequest->m_pDestRect;
	
	
	
	
	    /* VS
		
	if (g_vFogColorEnabled.w > 0.0f)
    {
        float4 vCameraPos = mul(Output.vPosition, g_mWorldMatrix);
        vCameraPos = mul(vCameraPos, g_mViewMatrix);
        
        Output.fFogFactor = saturate((g_vFogStartEnd.y - vCameraPos.z) / (g_vFogStartEnd.y - g_vFogStartEnd.x));
    }*/
	
	
	// PS
	//if (Input.vFogColorEnabled.w > 0.0f)
    //    vFinalColor = Input.fFogFactor * vFinalColor + float4((1.0f - Input.fFogFactor) * Input.vFogColorEnabled.rgb, 1.0f);
	
	
	            // TODO - density
            //float4 vPosRelWorld = (g_dwMode & MODE_REALLY_CLOSE) ? g_vCameraPos : mul(Input.vPosition, g_mModelWorld);
            //Output.fFogFactor = ComputeVFogFactor(vPosRelWorld.y, g_vFogMinMaxY.x, g_vFogMinMaxY.y, 
            //  g_vFogMinMaxYVal.x, g_vFogMinMaxYVal.y, g_fVFogDensity);
	
float ComputeVFogFactor(float fDistOrY, float fFogMinY, float fFogMaxY, float fFogMinYVal, float fFogMaxYVal, float fDensity)
{
    float fFogFactor = (fDistOrY - fFogMinY) / (fFogMaxY - fFogMinY);
    float fFogFactorModded = fFogFactor * (fFogMaxYVal - fFogMinYVal) + fFogMinYVal;
    return saturate(fFogFactorModded);
}
	
float ComputeVFogFactorEx2(float fCameraY, float fDistOrZ, float fDistOrY, float fFogMinY, float fFogMaxY, float fFogMinYVal, float fFogMaxYVal, float fDensity)
{
    float fCamFogFactorY = (fCameraY - fFogMinY) / (fFogMaxY - fFogMinY);
    float fCamFogFactorYModded = saturate(fCamFogFactorY * (fFogMaxYVal - fFogMinYVal) + fFogMinYVal);
    
    float fFogFactorY = (fDistOrY - fFogMinY) / (fFogMaxY - fFogMinY);
    float fFogFactorYModded = saturate(fFogFactorY * (fFogMaxYVal - fFogMinYVal) + fFogMinYVal);
    
    float fFogFactorZ = saturate(fDistOrZ / fDensity);
    
    return max(fCamFogFactorYModded, fFogFactorYModded) * fFogFactorZ;
}

float ComputeVFogFactorEx1(float fCameraY, float fDistOrZ, float fDistOrY, float fFogMinY, float fFogMaxY, float fFogMinYVal, float fFogMaxYVal, float fDensity)
{  
    float fFogFactor = (fDistOrY - fFogMinY) / (fFogMaxY - fFogMinY);
    float fFogFactorModded = fFogFactor * (fFogMaxYVal - fFogMinYVal) + fFogMinYVal;
    float fFogFactorY = saturate(fFogFactorModded);
    
    float fFogFactorZ = saturate(fDistOrZ / fDensity);
    
    if (fCameraY > fFogMinY && fCameraY < fFogMaxY)
        return fFogFactorZ * fFogFactorY;
    else
        return fFogFactorY;
}	
	
// TEST
static void SetupCloseEnvMapPanning(ViewParams* pParams)
{
	DirectX::XMMATRIX mMat = DirectX::XMMatrixIdentity();
	DirectX::XMFLOAT4X4& mView = g_ViewParams.m_mInvView;

	static DirectX::XMFLOAT3 s_vLastPos = { mView.m[0][3], mView.m[1][3], mView.m[2][3] };
	static bool s_bInitCamOfs = true;

	//DirectX::XMVECTOR vLastPos = DirectX::XMLoadFloat3(&s_vLastPos);

	static DirectX::XMFLOAT4X4 s_mCamOfs;
	if (s_bInitCamOfs)
	{
		DirectX::XMStoreFloat4x4(&s_mCamOfs, DirectX::XMMatrixIdentity());
		s_bInitCamOfs = false;
	}

	DirectX::XMFLOAT3 v1, v2, v3;
	Matrix_GetBasisVectorsLT(&mView, &v1, &v2, &v3);

	DirectX::XMVECTOR vOffset =
		DirectX::XMVectorSet(mView.m[0][3] - s_vLastPos.x, mView.m[1][3] - s_vLastPos.y, mView.m[2][3] - s_vLastPos.z, 1.0f);

	vOffset /= 32.0f;
	DirectX::XMFLOAT3 vCamRelOfs = {
		DirectX::XMVectorGetX(DirectX::XMVector3Dot(vOffset, DirectX::XMLoadFloat3(&v1))),
		DirectX::XMVectorGetX(DirectX::XMVector3Dot(vOffset, DirectX::XMLoadFloat3(&v2))),
		DirectX::XMVectorGetX(DirectX::XMVector3Dot(vOffset, DirectX::XMLoadFloat3(&v3)))
	};

	DirectX::XMMATRIX mCamOfs = DirectX::XMLoadFloat4x4(&s_mCamOfs);

	DirectX::XMMATRIX mRotateAboutY =
		DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), vCamRelOfs.x);
	mCamOfs = mRotateAboutY * mCamOfs;
	DirectX::XMMATRIX mRotateAboutX =
		DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f), -(vCamRelOfs.y + vCamRelOfs.z));
	mCamOfs = mRotateAboutX * mCamOfs;

	DirectX::XMFLOAT4X4 mCamOfsTemp;
	DirectX::XMStoreFloat4x4(&mCamOfsTemp, mCamOfs);
	Matrix_NormalizeLT(&mCamOfsTemp);
	mCamOfs = DirectX::XMLoadFloat4x4(&mCamOfsTemp);

	mMat = mCamOfs * DirectX::XMLoadFloat4x4(&g_ViewParams.m_mView);

	s_vLastPos.x = mView.m[0][3];
	s_vLastPos.y = mView.m[1][3];
	s_vLastPos.z = mView.m[2][3];

	DirectX::XMStoreFloat4x4(&pParams->m_mRCEnvMapTransform, mMat);
}

bool CWorldModelData::InitOld(WorldBsp* pBsp)
{
	Array_InternalWorldModelVertex aVertexData;
	Array_uint16 aIndexData;

	srand(*(uint32*)&g_fLastClientTime);

	m_dwVerts = pBsp->m_dwPoints;

	for (uint32 dwPoly = 0; dwPoly < pBsp->m_dwPolies; dwPoly++)
		m_dwIndices += (pBsp->m_pPolies[dwPoly]->m_wNumVerts - 2) * 3;

	aVertexData.reserve(m_dwVerts);
	aIndexData.reserve(m_dwIndices);

	for (uint32 dwVert = 0; dwVert < m_dwVerts; dwVert++)
	{
		InternalWorldModelVertex internalVertex;

		uint8 nColorR = rand() % 256;
		uint8 nColorG = rand() % 256;
		uint8 nColorB = rand() % 256;

		internalVertex.vPosition = *PLTVECTOR_TO_PXMFLOAT3(&pBsp->m_pPoints[dwVert]);
		//internalVertex.vNormal = *PLTVECTOR_TO_PXMFLOAT3(&pPoly->m_pPlane->m_vNormal);
		internalVertex.vNormal = { 0.0f, 1.0f, 0.0f };
		internalVertex.vTexCoords = { 0.0f, 0.0f };
		internalVertex.adwTextureIndices[0] = nColorR;
		internalVertex.adwTextureIndices[1] = nColorG;
		internalVertex.adwTextureIndices[2] = nColorB;

		aVertexData.push_back(internalVertex);
	}

	for (uint32 dwPoly = 0; dwPoly < pBsp->m_dwPolies; dwPoly++)
	{
		WorldPoly* pPoly = pBsp->m_pPolies[dwPoly];

		int nStartVertex = pPoly->m_Vertices[0].m_pPoints - pBsp->m_pPoints;

		for (uint32 dwVert = 0; dwVert < pPoly->m_wNumVerts; dwVert++)
		{
			int nRealIndex = pPoly->m_Vertices[dwVert].m_pPoints - pBsp->m_pPoints;

			if (dwVert > 2)
			{
				aIndexData.push_back((uint16)(nStartVertex));
				aIndexData.push_back(aIndexData[aIndexData.size() - 2]);
			}

			aIndexData.push_back((uint16)(nRealIndex));
		}
	}

	return CreateVertexBuffer(aVertexData.data(), m_dwVerts) &&
		CreateIndexBuffer(aIndexData.data(), m_dwIndices);
}

static void d3d_ProjectWithBias(ViewParams* pParams, InternalSpriteVertex* pPoints, uint32 dwPoints, float fBiasDist)
{
	DirectX::XMFLOAT4X4& mDTP = pParams->m_mDeviceTimesProjection;
	
	for (uint32 i = 0; i < dwPoints; i++)
	{
		DirectX::XMVECTOR vPtViewProj = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&pPoints[i].vPosition), 
			DirectX::XMLoadFloat4x4(&g_ViewParams.m_mViewTransposed));

		vPtViewProj = DirectX::XMVector3Transform(vPtViewProj, 
			DirectX::XMLoadFloat4x4(&g_ViewParams.m_mProjectionTransposed));

		DirectX::XMStoreFloat3(&pPoints[i].vPosition, vPtViewProj);

		if (pPoints[i].vPosition.z + fBiasDist < g_CV_NearZ.m_Val)
			fBiasDist = g_CV_NearZ.m_Val - pPoints[i].vPosition.z;

		float fYMod_42 = mDTP._42 * pPoints[i].vPosition.y;
		float fXMod_41 = mDTP._41 * pPoints[i].vPosition.x;

		float fScaleXY = 1.0f / (mDTP._43 * pPoints[i].vPosition.z + fYMod_42 + fXMod_41 + mDTP._44);

		float fNewX = (mDTP._13 * pPoints[i].vPosition.z +
			mDTP._11 * pPoints[i].vPosition.x +
			mDTP._12 * pPoints[i].vPosition.y + mDTP._14) * fScaleXY;

		float fNewY = (mDTP._23 * pPoints[i].vPosition.z +
			mDTP._21 * pPoints[i].vPosition.x +
			mDTP._22 * pPoints[i].vPosition.y + mDTP._24) * fScaleXY;

		float fNewZ_0 = fBiasDist + pPoints[i].vPosition.z;

		float fScaleZ = 1.0f / (mDTP._43 * fNewZ_0 + fYMod_42 + fXMod_41 + mDTP._44);

		float fNewZ_1 = mDTP._31 * pPoints[i].vPosition.x +
			mDTP._32 * pPoints[i].vPosition.y +
			mDTP._33 * fNewZ_0 + mDTP._34;

		pPoints[i].vPosition.x = fNewX;
		pPoints[i].vPosition.y = fNewY;
		pPoints[i].vPosition.z = fNewZ_1 * fScaleZ;
	}
}


bool g_abWorldModelVisible[256] = { };
static void d3d_PolyTest(WorldPoly* pPoly)
{
	MainWorld* pMainWorld = g_pSceneDesc->m_pRenderContext->m_pMainWorld;

	for (uint32 i = 0; i < pMainWorld->GetWorldModelCount(); i++)
	{
		WorldBsp* pBsp = pMainWorld->GetWorldModelData(i)->m_pOriginalBsp;

		if (pPoly->m_vCenter.x <= pBsp->m_vMaxBox.x && pPoly->m_vCenter.x >= pBsp->m_vMinBox.x &&
			pPoly->m_vCenter.y <= pBsp->m_vMaxBox.y && pPoly->m_vCenter.y >= pBsp->m_vMinBox.y &&
			pPoly->m_vCenter.z <= pBsp->m_vMaxBox.z && pPoly->m_vCenter.z >= pBsp->m_vMinBox.z)
		{
			g_abWorldModelVisible[i] = true;
		}
	}
}

static void d3d_IterateLeaf_VisNodes(Leaf* pLeaf, void* pData)
{
	if (pLeaf->m_wFrameCode != g_wCurFrameCode)
	{
		pLeaf->m_wFrameCode = g_wCurFrameCode;

		for (uint32 i = 0; i < pLeaf->m_dwPolies; i++)
		{
			WorldPoly* pPoly = pLeaf->m_pPolies[i];

			if (pPoly->m_wFrameCode != g_wCurFrameCode)
			{
				pPoly->m_wFrameCode = g_wCurFrameCode;
				
				d3d_PolyTest(pPoly);
			}
		}
	}
}

static void d3d_QueuePolies_VisNodes(ViewParams* pParams)
{
	WorldTree* pWorldTree = &g_pSceneDesc->m_pRenderContext->m_pMainWorld->m_WorldTree;

	VisQueryRequest request;

	request.m_eObjArray = NOA_Objects;
	request.m_vViewpoint = *(LTVector*)&pParams->m_vPos;
	request.m_fViewRadius = 10000.0f;
	request.m_LeafCB = d3d_IterateLeaf_VisNodes;
	//request.m_PortalTest = [](UserPortal* pPortal) { return LTBOOL(0); };
	//request.m_NodeFilterFn = [](WorldTreeNode* pNode) { return LTBOOL(1); };
	request.m_pUserData = nullptr;

	WorldTree_VTable* pVTable = (WorldTree_VTable*)pWorldTree->m_pVTable;

	uint32 dwNotUsed = 0;
	pVTable->WorldTree__DoVisQuery(pWorldTree, &dwNotUsed, &request);
}




float3 GetShadowMapColor2(float3 vWorldPos, float3 vWorldNormal, float3 vShadowMap, LMAnimData_PS animData)
{
    const float3 fAttConfig= float3(0.0f, 0.2f, 0.0f);
    
    float3 vFinalColor = float3(0.0f, 0.0f, 0.0f);
    float3 vToPixelVector = animData.m_vLightPos - vWorldPos;
    float fDist = length(vToPixelVector);
 
    if (fDist > animData.m_vLightColorAndRadius.a)
        return vFinalColor;

    vToPixelVector /= fDist;

    float fIntensity = dot(vToPixelVector, vWorldNormal);
    
    if (fIntensity > 0.0f)
    {
        vFinalColor += fIntensity * animData.m_vLightColorAndRadius.rgb * vShadowMap;
        vFinalColor /= fAttConfig.x + (fAttConfig.y * fDist) + (fAttConfig.z * (fDist * fDist));
    }

    return vFinalColor;
}

float3 GetShadowMapColorFull(float3 vWorldPos, float3 vWorldNormal, float3 vShadowMap, LMAnimData_PS animData)
{
    float3 vToLight = animData.m_vLightPos - vWorldPos;
    float fDist = length(vToLight);

    vToLight /= fDist;
    float fIntensity = saturate(dot(vToLight, vWorldNormal));
    float3 vFinalColor = animData.m_vLightColorAndRadius.rgb * fIntensity;
    
    float fDistNorm = 1.0f - saturate(fDist * (1.0f / animData.m_vLightColorAndRadius.a));
    float fAttenuation = fDistNorm * fDistNorm; // pow(fDistNorm, 4.0f)
    
    vFinalColor = vFinalColor * fAttenuation * vShadowMap;
    
    return vFinalColor;
}

void CRenderShader_Base::LMBlendData_PS::SetupLightColor(LMAnim* pAnim)
{
	/*m_vLightColorAndRadius = DirectX::XMVectorSet(
		pAnim->m_vLightColor.x / 255.0f,
		pAnim->m_vLightColor.y / 255.0f,
		pAnim->m_vLightColor.z / 255.0f,
		pAnim->m_fLightRadius
	);*/

	int nRed = (uint32)pAnim->m_vLightColor.x;
	int nGreen = (uint32)pAnim->m_vLightColor.y;
	int nBlue = (uint32)pAnim->m_vLightColor.z;

	nRed = (nRed << 16) - (0xFF0000 - (nRed << 16));
	nGreen = (nGreen << 16) - (0xFF0000 - (nGreen << 16));
	nBlue = (nBlue << 16) - (0xFF0000 - (nBlue << 16));

	if (nRed < 0 || nGreen < 0 || nBlue < 0)
	{
		m_vLightColorAndRadius = DirectX::XMVectorSet(
			0.0f,
			0.0f,
			0.0f,
			pAnim->m_fLightRadius
		);

		return;
	}

	m_vLightColorAndRadius = DirectX::XMVectorSet(
		(float)(nRed >> 16) / 255.0f,
		(float)(nGreen >> 16) / 255.0f,
		(float)(nBlue >> 16) / 255.0f,
		pAnim->m_fLightRadius
	);
}

9B 9E A4
155 158 164

  *(a12 + 76) = (a9 << 16) - (0xFF0000 - (a9 << 16));
  *(a12 + 80) = (a10 << 16) - (0xFF0000 - (a10 << 16));
  *(a12 + 84) = (a11 << 16) - (0xFF0000 - (a11 << 16));

00370000 003D0000 00490000 - 76 (19) 80 (20) 84 (21)
00004E74 009B9EA4 - 92 (23) 96 (24)

00370000 * FF00 >> 32 = 36 -> 35
003D0000 * FF00 >> 32 = 3C -> 3B
00490000 * FF00 >> 32 = 48 -> 47

----

86 88 8E
134 136 142

---

4D 4F 52
77 79 82

FF9B0000 FF9F0000 FFA50000

FF9B0000 * FF00 >> 32 = FE8B
FF9F0000 * FF00 >> 32 = FE9F
FFA50000 * FF00 >> 32 = FEA5