#include "pch.h"

#include "full_intersectline.h"
#include "d3d_mathhelpers.h"
#include "intersectline.h"
#include "tagnodes.h"

using namespace DirectX;

static XMIntersectQuery* g_pCurQuery;
static LTObject* g_pIntersection;
static Node* g_pWorldIntersection;
static HPOLY g_hWorldPoly;
static float g_fIntersectionBestDistSqr;

static bool g_bProcessNonSolid;
static bool g_bProcessObjects;

static DirectX::XMFLOAT3 g_vTimesInvVV;
static DirectX::XMFLOAT3 g_vOrigin;
static DirectX::XMFLOAT3 g_vDir;

static float g_fLineLen;
static float g_fVPTimesInvVV;

static XMPLANE g_IntersectionPlane;
static DirectX::XMFLOAT3 g_vIntersectionPos;

static void (*g_FindIntersectionsFn)(WorldBsp* pWorldBsp, Node** pNodeIntersectionPtr,
    DirectX::XMFLOAT3* pIntersectionPosPtr, float* pDistSqrPtr, HPOLY* hWorldPoly,
    DirectX::XMFLOAT3* pPoint1, DirectX::XMFLOAT3* pPoint2, bool bWorldModel);

#define DO_PLANE_TEST_X(planeCoord, coord0, coord1, coord2, normalDirection) \
    fT = (planeCoord - vPoint1.coord0) / (vPoint2.coord0 - vPoint1.coord0);\
    afTestCoords[0] = vPoint1.coord1 + ((vPoint2.coord1 - vPoint1.coord1) * fT);\
    if (afTestCoords[0] > vMin.coord1 && afTestCoords[0] < vMax.coord1)\
    {\
        afTestCoords[1] = vPoint1.coord2 + ((vPoint2.coord2 - vPoint1.coord2) * fT);\
        if (afTestCoords[1] > vMin.coord2 && afTestCoords[1] < vMax.coord2)\
        {\
            pIntersectPt->coord0 = planeCoord;\
            pIntersectPt->coord1 = afTestCoords[0];\
            pIntersectPt->coord2 = afTestCoords[1];\
            pIntersectPlane->m_vNormal.x = normalDirection;\
            pIntersectPlane->m_vNormal.y = 0.0f;\
            pIntersectPlane->m_vNormal.z = 0.0f;\
            pIntersectPlane->m_fDist = vMin.x * normalDirection;\
            return true;\
        }\
    }

#define DO_PLANE_TEST_Y(planeCoord, coord0, coord1, coord2, normalDirection) \
    fT = (planeCoord - vPoint1.coord0) / (vPoint2.coord0 - vPoint1.coord0);\
    afTestCoords[0] = vPoint1.coord1 + ((vPoint2.coord1 - vPoint1.coord1) * fT);\
    if (afTestCoords[0] > vMin.coord1 && afTestCoords[0] < vMax.coord1)\
    {\
        afTestCoords[1] = vPoint1.coord2 + ((vPoint2.coord2 - vPoint1.coord2) * fT);\
        if (afTestCoords[1] > vMin.coord2 && afTestCoords[1] < vMax.coord2)\
        {\
            pIntersectPt->coord0 = planeCoord;\
            pIntersectPt->coord1 = afTestCoords[0];\
            pIntersectPt->coord2 = afTestCoords[1];\
            pIntersectPlane->m_vNormal.x = 0.0f;\
            pIntersectPlane->m_vNormal.y = normalDirection;\
            pIntersectPlane->m_vNormal.z = 0.0f;\
            pIntersectPlane->m_fDist = vMin.y * normalDirection;\
            return true;\
        }\
    }

#define DO_PLANE_TEST_Z(planeCoord, coord0, coord1, coord2, normalDirection) \
    fT = (planeCoord - vPoint1.coord0) / (vPoint2.coord0 - vPoint1.coord0);\
    afTestCoords[0] = vPoint1.coord1 + ((vPoint2.coord1 - vPoint1.coord1) * fT);\
    if (afTestCoords[0] > vMin.coord1 && afTestCoords[0] < vMax.coord1)\
    {\
        afTestCoords[1] = vPoint1.coord2 + ((vPoint2.coord2 - vPoint1.coord2) * fT);\
        if (afTestCoords[1] > vMin.coord2 && afTestCoords[1] < vMax.coord2)\
        {\
            pIntersectPt->coord0 = planeCoord;\
            pIntersectPt->coord1 = afTestCoords[0];\
            pIntersectPt->coord2 = afTestCoords[1];\
            pIntersectPlane->m_vNormal.x = 0.0f;\
            pIntersectPlane->m_vNormal.y = 0.0f;\
            pIntersectPlane->m_vNormal.z = normalDirection;\
            pIntersectPlane->m_fDist = vMin.z * normalDirection;\
            return true;\
        }\
    }

#define USE_THIS_OBJECT(pServerObj, distSqr, plane, intersectionPt, hPoly) \
    g_fIntersectionBestDistSqr = distSqr;\
    g_pIntersection = pServerObj;\
    g_IntersectionPlane = plane;\
    g_vIntersectionPos = intersectionPt;\
    g_hWorldPoly = hPoly;

inline bool i_BoundingBoxTest(DirectX::XMFLOAT3& vPoint1, DirectX::XMFLOAT3& vPoint2, const LTObject* pServerObj,
    DirectX::XMFLOAT3* pIntersectPt, XMPLANE* pIntersectPlane)
{
    float fT;
    float afTestCoords[2];

    DirectX::XMFLOAT3& vMin = *PLTVECTOR_TO_PXMFLOAT3(&pServerObj->m_vBBoxMin);
    DirectX::XMFLOAT3& vMax = *PLTVECTOR_TO_PXMFLOAT3(&pServerObj->m_vBBoxMax);

    if (vPoint1.x < vMin.x)
    {
        if (vPoint2.x < vMin.x)
            return false;

        DO_PLANE_TEST_X(vMin.x, x, y, z, -1.0f);
    }
    else if (vPoint1.x > vMax.x)
    {
        if (vPoint2.x > vMax.x)
            return false;

        DO_PLANE_TEST_X(vMax.x, x, y, z, 1.0f);
    }

    if (vPoint1.y < vMin.y)
    {
        if (vPoint2.y < vMin.y)
            return false;

        DO_PLANE_TEST_Y(vMin.y, y, x, z, -1.0f);
    }
    else if (vPoint1.y > vMax.y)
    {
        if (vPoint2.y > vMax.y)
            return false;

        DO_PLANE_TEST_Y(vMax.y, y, x, z, 1.0f);
    }

    if (vPoint1.z < vMin.z)
    {
        if (vPoint2.z < vMin.z)
            return false;

        DO_PLANE_TEST_Z(vMin.z, z, x, y, -1.0f);
    }
    else if (vPoint1.z > vMax.z)
    {
        if (vPoint2.z > vMax.z)
            return false;

        DO_PLANE_TEST_Z(vMax.z, z, x, y, 1.0f);
    }

    return false;
}

static inline bool i_QuickSphereTest(LTObject* pServerObj)
{  
    float t = Vector_Dot(&g_vTimesInvVV, PLTVECTOR_TO_PXMFLOAT3(&pServerObj->m_vPos)) - 
        g_fVPTimesInvVV;

    float fRadius = pServerObj->m_fRadius;

    if (t < -fRadius || t > (g_fLineLen + fRadius))
        return false;

    DirectX::XMVECTOR vTo = DirectX::XMLoadFloat3(&g_pCurQuery->m_vFrom) +
        DirectX::XMLoadFloat3(&g_vDir) * t -
        DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pServerObj->m_vPos));

    return (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vTo)) < fRadius * fRadius);
}

static bool i_TestWorldModel(WorldModelInstance* pObj)
{
    DirectX::XMFLOAT3 vPoints[2];
    
    Matrix_VMul_H(&vPoints[0], PLTMATRIX_TO_PXMFLOAT4X4(&pObj->m_mBackTransform), &g_pCurQuery->m_vFrom);
    Matrix_VMul_H(&vPoints[1], PLTMATRIX_TO_PXMFLOAT4X4(&pObj->m_mBackTransform), &g_pCurQuery->m_vTo);

    Node* pNodeIntersection;
    DirectX::XMFLOAT3 vIntersectionPt;
    LTFLOAT fDistToIntersectionSqr;
    HPOLY hWorldPoly;

    g_FindIntersectionsFn(pObj->m_pOriginalBsp, &pNodeIntersection, &vIntersectionPt, &fDistToIntersectionSqr, 
        &hWorldPoly, &vPoints[0], &vPoints[1], true);

    if (!pNodeIntersection)
        return false;

    Matrix_VMul_InPlace_H(PLTMATRIX_TO_PXMFLOAT4X4(&pObj->m_mTransform), &vIntersectionPt);

    fDistToIntersectionSqr = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(
        DirectX::XMLoadFloat3(&g_pCurQuery->m_vFrom) - DirectX::XMLoadFloat3(&vIntersectionPt)));

    if (fDistToIntersectionSqr < g_fIntersectionBestDistSqr)
    {
        XMPLANE tempPlane;

        DirectX::XMVECTOR vPlanePt = 
            DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pNodeIntersection->m_pPoly->m_pPlane->m_vNormal)) * 
            pNodeIntersection->m_pPoly->m_pPlane->m_fDist;

        Matrix_VMul_3x3(&tempPlane.m_vNormal, PLTMATRIX_TO_PXMFLOAT4X4(&pObj->m_mTransform),
            PLTVECTOR_TO_PXMFLOAT3(&pNodeIntersection->m_pPoly->m_pPlane->m_vNormal));

        tempPlane.m_fDist = DirectX::XMVectorGetX(DirectX::XMVector3Dot(
            DirectX::XMLoadFloat3(&tempPlane.m_vNormal), vPlanePt));  

        g_pWorldIntersection = pNodeIntersection;

        USE_THIS_OBJECT(&pObj->m_Base, fDistToIntersectionSqr, tempPlane, vIntersectionPt, hWorldPoly);
        return true;
    }

    return false;
}

static void i_FindIntersectionsHPoly(WorldBsp* pWorldBsp, Node** pNodeIntersectionPtr,
    DirectX::XMFLOAT3* pIntersectionPosPtr, float* pDistSqrPtr, HPOLY* hWorldPoly,
    DirectX::XMFLOAT3* pPoint1, DirectX::XMFLOAT3* pPoint2, bool bWorldModel)
{
    IntersectRequest req;

    req.m_pPoints[0] = pPoint1;
    req.m_pPoints[1] = pPoint2;
    req.m_pIPos = pIntersectionPosPtr;
    req.m_pQuery = g_pCurQuery;
    req.m_pWorldBsp = pWorldBsp;

    if (IntersectLineNode(pWorldBsp->m_pRootNode, &req))
    {
        *hWorldPoly = pWorldBsp->MakeHPoly(req.m_pNodeHit);
        *pNodeIntersectionPtr = req.m_pNodeHit;

        *pDistSqrPtr = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(
            DirectX::XMLoadFloat3(&g_pCurQuery->m_vFrom) - DirectX::XMLoadFloat3(req.m_pIPos)));
    }
    else
    {
        *hWorldPoly = INVALID_HPOLY;
        *pNodeIntersectionPtr = nullptr;
    }
}

static void i_FindIntersections(WorldBsp* pWorldBsp, Node** pNodeIntersectionPtr,
DirectX::XMFLOAT3* pIntersectionPosPtr, float* pDistSqrPtr, HPOLY* hWorldPoly,
    DirectX::XMFLOAT3* pPoint1, DirectX::XMFLOAT3* pPoint2, bool bWorldModel)
{
    XMPLANE iPlane;

    *hWorldPoly = INVALID_HPOLY;
    *pNodeIntersectionPtr = IntersectLine(pWorldBsp->m_pRootNode, pPoint1, pPoint2, pIntersectionPosPtr, &iPlane);

    if (*pNodeIntersectionPtr != nullptr)
    {
        *pDistSqrPtr = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(
            DirectX::XMLoadFloat3(pPoint1) - DirectX::XMLoadFloat3(pIntersectionPosPtr)));
    }
}

bool i_HandlePossibleIntersection(DirectX::XMFLOAT3& vPoint1, DirectX::XMFLOAT3& vPoint2, LTObject* pServerObj)
{
    if (i_QuickSphereTest(pServerObj))
    {
        if (g_pCurQuery->m_FilterFn != nullptr && 
            !g_pCurQuery->m_FilterFn(pServerObj, g_pCurQuery->m_pUserData))
        {
            
        }
        else
        {
            if (pServerObj->IsWorldModel())
            {
                return i_TestWorldModel(pServerObj->ToWorldModel());
            }
            else
            {
                DirectX::XMFLOAT3 vTestPt;
                XMPLANE testPlane;

                if (i_BoundingBoxTest(vPoint1, vPoint2, pServerObj, &vTestPt, &testPlane))
                {
                    float fDistToIntersectionSqr = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(
                        DirectX::XMLoadFloat3(&vTestPt) - DirectX::XMLoadFloat3(&g_pCurQuery->m_vFrom)));

                    if (g_pIntersection != nullptr)
                    {
                        if (fDistToIntersectionSqr < g_fIntersectionBestDistSqr)
                        {
                            USE_THIS_OBJECT(pServerObj, fDistToIntersectionSqr, testPlane, vTestPt, INVALID_HPOLY);
                            return true;
                        }
                    }
                    else
                    {
                        USE_THIS_OBJECT(pServerObj, fDistToIntersectionSqr, testPlane, vTestPt, INVALID_HPOLY);
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

static bool IsMainWorldModel(LTObject* pObject)
{
    return (pObject->m_nObjectType == OT_WORLDMODEL) && pObject->ToWorldModel()->IsMainWorldModel();
}

static bool i_ISCallback(WorldTreeObj* pObj, void* pCBUser)
{
    LTObject* pObject = (LTObject*)pObj;

    if (pObject->m_dwFlags & (FLAG_RAYHIT | FLAG_SOLID) || g_bProcessNonSolid)
    {
        if (!g_bProcessObjects && !IsMainWorldModel(pObject))
            return false;

        return i_HandlePossibleIntersection(g_pCurQuery->m_vFrom, g_pCurQuery->m_vTo, pObject);
    }

    return false;
}

bool i_IntersectSegment(XMIntersectQuery* pQuery, XMIntersectInfo* pInfo, WorldTree* pWorldTree)
{
    g_pCurQuery = pQuery;
    g_pIntersection = nullptr;
    g_pWorldIntersection = nullptr;
    g_hWorldPoly = INVALID_HPOLY;
    g_bProcessNonSolid = !(pQuery->m_dwFlags & IGNORE_NONSOLID);
    g_bProcessObjects = !!(pQuery->m_dwFlags & INTERSECT_OBJECTS);

    DirectX::XMVECTOR vFrom = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pQuery->m_vFrom));
    DirectX::XMVECTOR vTo = DirectX::XMLoadFloat3(PLTVECTOR_TO_PXMFLOAT3(&pQuery->m_vTo));

    g_fIntersectionBestDistSqr = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vFrom - vTo)) + 1.0f;

    g_vOrigin = *PLTVECTOR_TO_PXMFLOAT3(&pQuery->m_vFrom);

    DirectX::XMVECTOR vDir = vTo - vFrom;

    g_fLineLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(vDir));

    vDir = vDir / g_fLineLen;
    DirectX::XMStoreFloat3(&g_vDir, vDir);

    float fTestMag = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vDir));

    if (fTestMag < 0.5f || fTestMag > 2.0f)
        return false;
    
    float fVP = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vDir, vFrom));
    float fInvVV = 1.0f / fTestMag;
    DirectX::XMStoreFloat3(&g_vTimesInvVV, vDir * fInvVV);
    g_fVPTimesInvVV = fVP * fInvVV;

    if (pQuery->m_dwFlags & INTERSECT_HPOLY)
        g_FindIntersectionsFn = i_FindIntersectionsHPoly;
    else
        g_FindIntersectionsFn = i_FindIntersections;

    WorldTree_VTable* pVTable = (WorldTree_VTable*)pWorldTree->m_pVTable;

    uint32 dwNotUsed = 0;
    pVTable->WorldTree_IntersectSegment(pWorldTree, &dwNotUsed, 
        PXMFLOAT3_TO_PLTVECTOR(&pQuery->m_vFrom), PXMFLOAT3_TO_PLTVECTOR(&pQuery->m_vTo),
        i_ISCallback, nullptr, NOA_Objects);

    if (g_pIntersection)
    {
        pInfo->m_vPoint = g_vIntersectionPos;
        pInfo->m_Plane = g_IntersectionPlane;
        pInfo->m_pObject = g_pIntersection;
        pInfo->m_hPoly = g_hWorldPoly;

        if (g_pWorldIntersection)
            pInfo->m_dwSurfaceFlags = g_pWorldIntersection->m_pPoly->m_pSurface->m_wTextureFlags;
        else
            pInfo->m_dwSurfaceFlags = 0;

        return true;
    }
    else
    {
        return false;
    }
}
