#ifndef __INTERSECT_LINE_H__
#define __INTERSECT_LINE_H__

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

class XMIntersectQuery;

typedef void (*ILineCallback)(Node* pNode);

class IntersectRequest
{

public:

    IntersectRequest()
    {
        m_pPoints[0] = nullptr;
        m_pPoints[1] = nullptr;
        m_pIPos = nullptr;

        m_pNodeHit = nullptr;
        m_pQuery = nullptr;
        m_pWorldBsp = nullptr;
    }

    DirectX::XMFLOAT3*   m_pPoints[2];
    DirectX::XMFLOAT3*   m_pIPos;

    XMIntersectQuery* m_pQuery;

    WorldBsp* m_pWorldBsp;
    Node*     m_pNodeHit;
};

struct XMPLANE;

Node* IntersectLine(Node* pRoot, DirectX::XMFLOAT3* pPoint1, DirectX::XMFLOAT3* pPoint2, 
    DirectX::XMFLOAT3* pIPos, XMPLANE* pIPlane);

bool IntersectLineNode(Node* pRoot, IntersectRequest* pRequest);

#endif
