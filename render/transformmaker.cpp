#include "pch.h"

#include "transformmaker.h"
#include "d3d_mathhelpers.h"

using namespace DirectX;

GVPStruct::GVPStruct() : m_Anims()
{
	m_pVertices = nullptr;
	m_dwVertexStride = 0;
	DirectX::XMStoreFloat4x4(&m_mBaseTransform, DirectX::XMMatrixIdentity());
	m_dwAnims = 1;
	m_dwLOD = PIECELOD_BASE;
	m_fCurrentLODDist = 0.0f;
}

void TransformMaker::SetupFromGVPStruct(GVPStruct *pStruct)
{
	for(uint32 i=0; i < pStruct->m_dwAnims; i++)
		m_Anims[i] = pStruct->m_Anims[i];

	m_dwAnims = pStruct->m_dwAnims;
	m_pStartMatrix = &pStruct->m_mBaseTransform;
}

bool TransformMaker::IsValid()
{
	if(m_dwAnims == 0 || m_dwAnims > MAX_GVP_ANIMS)
		return false;

	Model* pModel = m_Anims[0].m_pModel;

	for(uint32 i = 0; i < m_dwAnims; i++)
	{
		AnimTimeRef* pAnim = &m_Anims[i];

		if(!pAnim->IsValid() || pAnim->m_pModel != pModel)
			return false;

		if (i != 0)
		{
			if (pAnim->m_Cur.m_dwWeightSet >= pModel->GetWeightSetCount())
				return false;
		}
	}

	return true;
}

void TransformMaker::CopyTimes(TransformMaker &other)
{
	for(uint32 i = 0; i < other.m_dwAnims; i++)
		m_Anims[i] = other.m_Anims[i];

	m_dwAnims = other.m_dwAnims;
}

bool TransformMaker::SetupTransforms()
{
	if(!SetupCall()) 
		return false;

	NodeEvalState* pNodeEvalStates = new NodeEvalState[m_pModel->GetNodeCount()];
	Recurse(m_pModel->m_pRootNode->m_wNodeIndex, m_pStartMatrix, pNodeEvalStates);
	delete [] pNodeEvalStates;

	return true;
}

bool TransformMaker::GetNodeTransform(uint32 dwNode)
{
	if(!SetupCall())
		return false;

	if(dwNode >= m_pModel->GetNodeCount())
		return false;

	uint32 dwThePath[MAX_MODELPATH_LEN];
	m_dwCurPath = 0;

	while (true)
	{
		if(m_dwCurPath >= MAX_MODELPATH_LEN)
			return false;

		dwThePath[m_dwCurPath] = dwNode;
		m_dwCurPath++;

		dwNode = m_pModel->GetNode(dwNode)->m_dwParentNode;
		if(dwNode == NODEPARENT_NONE)
			break;
	}

	m_pRecursePath = dwThePath;
	m_dwCurPath--;

	NodeEvalState* pNodeEvalStates = new NodeEvalState[m_pModel->GetNodeCount()];
	Recurse(m_pModel->m_pRootNode->m_wNodeIndex, m_pStartMatrix, pNodeEvalStates);
	delete [] pNodeEvalStates;

	return true;
}

bool TransformMaker::SetupCall()
{
	if(!IsValid())
		return false;

	m_pRecursePath = nullptr;
	m_pModel = m_Anims[0].m_pModel;

	if(m_pStartMatrix == nullptr)
	{
		DirectX::XMStoreFloat4x4(&m_mIdentity, DirectX::XMMatrixIdentity());
		m_pStartMatrix = &m_mIdentity;
	}

	if(m_pOutput == nullptr)
		m_pOutput = (DirectX::XMFLOAT4X4*)m_pModel->m_Transforms.m_pArray;

	AnimInfo* pAnim = m_pModel->GetAnimInfo(m_Anims[0].m_Prev.m_wAnim);
	m_pNodeRelation = &pAnim->m_pChildInfo->m_Relation;

	return true;
}

void TransformMaker::InitTransform(uint32 dwAnim, uint32 dwNode, DirectX::XMFLOAT4& rOutQuat, DirectX::XMFLOAT3& vOutVec)
{
	AnimTimeRef* pTimeRef = &m_Anims[dwAnim];
	ModelAnim* pAnims[2] = { m_pModel->GetAnim(pTimeRef->m_Prev.m_wAnim), m_pModel->GetAnim(pTimeRef->m_Cur.m_wAnim) };
	AnimNode* pAnimNodes[2] = { pAnims[0]->m_pAnimNodes[dwNode], pAnims[1]->m_pAnimNodes[dwNode] };
	
	NodeKeyFrame* pKeys[2] = 
	{ 
		pAnimNodes[0]->GetKeyFrame(pTimeRef->m_Prev.m_wFrame), 
		pAnimNodes[1]->GetKeyFrame(pTimeRef->m_Cur.m_wFrame) 
	};

	DirectX::XMVECTOR vQuaternion0 = DirectX::XMLoadFloat4(PLTROTATION_TO_PXMFLOAT4(&pKeys[0]->m_rQuaternion));
	DirectX::XMVECTOR vQuaternion1 = DirectX::XMLoadFloat4(PLTROTATION_TO_PXMFLOAT4(&pKeys[1]->m_rQuaternion));

	DirectX::XMVECTOR vTranslation0 = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pKeys[0]->m_vTranslation));
	DirectX::XMVECTOR vTranslation1 = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pKeys[1]->m_vTranslation));

	DirectX::XMVECTOR rOutQuatTemp = DirectX::XMQuaternionSlerp(vQuaternion0, vQuaternion1, pTimeRef->m_fPercent);
	DirectX::XMVECTOR vOutVecTemp = vTranslation0 + (vTranslation1 - vTranslation0) * pTimeRef->m_fPercent;

	if (dwNode == 0)
	{
		vTranslation0 = 
			DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&m_pModel->GetAnimInfo(pTimeRef->m_Prev.m_wAnim)->m_vTranslation));
		vTranslation1 = 
			DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&m_pModel->GetAnimInfo(pTimeRef->m_Cur.m_wAnim)->m_vTranslation));
			
		vOutVecTemp += vTranslation0 + (vTranslation1 - vTranslation0) * pTimeRef->m_fPercent;
	}

	DirectX::XMStoreFloat4(&rOutQuat, rOutQuatTemp);
	DirectX::XMStoreFloat3(&vOutVec, vOutVecTemp);
}

void TransformMaker::InitTransformAdditive(uint32 dwAnim, uint32 dwNode, DirectX::XMFLOAT4& rOutQuat, DirectX::XMFLOAT3& vOutVec)
{
	AnimTimeRef* pTimeRef = &m_Anims[dwAnim];
	ModelAnim* pAnims[2] = { m_pModel->GetAnim(pTimeRef->m_Prev.m_wAnim), m_pModel->GetAnim(pTimeRef->m_Cur.m_wAnim) };
	AnimNode* pAnimNodes[2] = { pAnims[0]->m_pAnimNodes[dwNode], pAnims[1]->m_pAnimNodes[dwNode] };

	NodeKeyFrame* pKeys[2] = 
	{ 
		pAnimNodes[0]->GetKeyFrame(pTimeRef->m_Prev.m_wFrame),
		pAnimNodes[1]->GetKeyFrame(pTimeRef->m_Cur.m_wFrame)
	};

	NodeKeyFrame* pBaseKey = pAnimNodes[0]->GetKeyFrame(0);

	DirectX::XMVECTOR vQuaternion0 = DirectX::XMLoadFloat4(PLTROTATION_TO_PXMFLOAT4(&pKeys[0]->m_rQuaternion));
	DirectX::XMVECTOR vQuaternion1 = DirectX::XMLoadFloat4(PLTROTATION_TO_PXMFLOAT4(&pKeys[1]->m_rQuaternion));

	DirectX::XMVECTOR vTranslation0 = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pKeys[0]->m_vTranslation));
	DirectX::XMVECTOR vTranslation1 = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pKeys[1]->m_vTranslation));
	
	DirectX::XMVECTOR rOutQuatTemp = DirectX::XMQuaternionSlerp(vQuaternion0, vQuaternion1, pTimeRef->m_fPercent);
	DirectX::XMVECTOR vOutVecTemp = vTranslation0 + (vTranslation1 - vTranslation0) * pTimeRef->m_fPercent;

	rOutQuatTemp = DirectX::XMQuaternionConjugate(DirectX::XMLoadFloat4(PLTROTATION_TO_PXMFLOAT4(&pBaseKey->m_rQuaternion))) *
		rOutQuatTemp;
	vOutVecTemp = vOutVecTemp - DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pBaseKey->m_vTranslation));

	DirectX::XMStoreFloat4(&rOutQuat, rOutQuatTemp);
	DirectX::XMStoreFloat3(&vOutVec, vOutVecTemp);
}

void TransformMaker::BlendTransform(uint32 dwAnim, uint32 dwNode)
{
	float fPercent = m_pModel->GetWeightSet(m_Anims[dwAnim].m_Cur.m_dwWeightSet)->GetWeight(dwNode);
	
	if (fPercent == 0.0f)
		return;
	
	DirectX::XMFLOAT4 rTransform;
	DirectX::XMFLOAT3 vTransform;

	if (fPercent == 2.0f)
	{
		InitTransformAdditive(dwAnim, dwNode, rTransform, vTransform);

		DirectX::XMVECTOR rTranformTemp = DirectX::XMLoadFloat4(&rTransform);
		DirectX::XMVECTOR vTransformTemp = DirectX::XMLoadFloat3(&vTransform);
		DirectX::XMVECTOR rQuaternionTemp = DirectX::XMLoadFloat4(&m_rQuaternion);
		DirectX::XMVECTOR vTranslationTemp = DirectX::XMLoadFloat3(&m_vTranslation);

		DirectX::XMStoreFloat4(&m_rQuaternion, rQuaternionTemp * rTranformTemp);
		DirectX::XMStoreFloat3(&m_vTranslation, vTranslationTemp + vTransformTemp);
	}
	else
	{
		InitTransform(dwAnim, dwNode, rTransform, vTransform);

		DirectX::XMVECTOR vTransformTemp = DirectX::XMLoadFloat3(&vTransform);
		DirectX::XMVECTOR rTransformTemp = DirectX::XMLoadFloat4(&rTransform);
		DirectX::XMVECTOR rQuaternionTemp = DirectX::XMLoadFloat4(&m_rQuaternion);
		DirectX::XMVECTOR vTranslationTemp = DirectX::XMLoadFloat3(&m_vTranslation);

		rQuaternionTemp = DirectX::XMQuaternionSlerp(rQuaternionTemp, rTransformTemp, fPercent);
		DirectX::XMStoreFloat3(&m_vTranslation, vTranslationTemp + (vTransformTemp - vTranslationTemp) * fPercent);
	}
}

void TransformMaker::Recurse(uint32 dwNode, DirectX::XMFLOAT4X4* pParentT, NodeEvalState* pNodeEvalStates)
{
	ModelNode* pNode = m_pModel->GetNode(dwNode);

	DirectX::XMFLOAT4X4* pMyGlobal = &m_pOutput[dwNode];
	*pMyGlobal = *pParentT;

	switch (pNodeEvalStates[dwNode])
	{	
		case NES_EVAL :
		{
			InitTransform(0, dwNode, m_rQuaternion, m_vTranslation);

			for(uint32 i = 1; i < m_dwAnims; i++)
				BlendTransform(i, dwNode);

			Matrix_FromQuaternionLT(&m_mTemp, &m_rQuaternion);

			if (pNode->m_nFlags & MNODE_ROTATIONONLY)
				Matrix_SetTranslationLT(&m_mTemp, PLTVECTOR_TO_PXMFLOAT3(&pNode->m_vOffsetFromParent));
			else
				Matrix_SetTranslationLT(&m_mTemp, PLTVECTOR_TO_PXMFLOAT3(&m_vTranslation));

			DirectX::XMMATRIX mTempTemp = DirectX::XMLoadFloat4x4(&m_mTemp);
			DirectX::XMMATRIX mMyGlobalTemp = DirectX::XMLoadFloat4x4(pMyGlobal);

			DirectX::XMStoreFloat4x4(pMyGlobal, mMyGlobalTemp * mTempTemp);
			pNodeEvalStates[dwNode] = NES_DONE;

			break;
		}
		
		case NES_DONE :
		{
			*pMyGlobal = *PLTMATRIX_TO_PXMFLOAT4X4(m_pModel->GetTransform(pNode->m_wNodeIndex));
			break;
		}
	}

	if(m_pRecursePath)
	{
		if(m_dwCurPath > 0)
		{
			m_dwCurPath--;
			Recurse(m_pRecursePath[m_dwCurPath], pMyGlobal, pNodeEvalStates);
		}
	}
	else
	{
		for(uint32 i = 0; i < pNode->GetChildCount(); i++)
			Recurse(pNode->GetChild(i)->m_wNodeIndex, pMyGlobal, pNodeEvalStates);
	}
}
