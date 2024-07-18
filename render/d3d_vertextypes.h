#ifndef __D3D_VERTEX_TYPES_H__
#define __D3D_VERTEX_TYPES_H__

#ifndef __D3D11_H__
#include <d3d11_1.h>
#define __D3D11_H__
#endif

#ifndef __DIRECT_X_MATH_H__
#include <DirectXMath.h>
#define __DIRECT_X_MATH_H__
#endif

namespace VertexTypes
{
	struct Texture
	{
		DirectX::XMFLOAT2	vTexCoords;

		bool CompareTo(Texture* pOther)
		{
			return vTexCoords.x == pOther->vTexCoords.x && vTexCoords.y == pOther->vTexCoords.y;
		}

		static constexpr uint32					c_dwInputElements = 1;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct TextureInstancedPositionColorScale
	{
		DirectX::XMFLOAT2	vTexCoords;
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT4	vDiffuseColor;
		float				fScale;

		static constexpr uint32					c_dwInputElements = 4;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct Position
	{
		DirectX::XMFLOAT3	vPosition;

		bool CompareTo(Position* pOther)
		{
			return vPosition.x == pOther->vPosition.x && vPosition.y == pOther->vPosition.y && vPosition.z == pOther->vPosition.z;
		}

		static constexpr uint32					c_dwInputElements = 1;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct PositionTexture
	{
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT2	vTexCoords;

		bool CompareTo(PositionTexture* pOther)
		{
			return vPosition.x == pOther->vPosition.x && vPosition.y == pOther->vPosition.y && vPosition.z == pOther->vPosition.z &&
				vTexCoords.x == pOther->vTexCoords.x && vTexCoords.y == pOther->vTexCoords.y;
		}

		static void ClipExtra(PositionTexture* pPrev, PositionTexture* pCur, PositionTexture* pOut, float t)
		{
			pOut->vTexCoords.x = pPrev->vTexCoords.x + t * (pCur->vTexCoords.x - pPrev->vTexCoords.x);
			pOut->vTexCoords.y = pPrev->vTexCoords.y + t * (pCur->vTexCoords.y - pPrev->vTexCoords.y);
		}

		static constexpr uint32					c_dwInputElements = 2;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct PositionTextureIndex
	{
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT2	vTexCoords;
		uint32				dwDataIndex;

		static void ClipExtra(PositionTextureIndex* pPrev, PositionTextureIndex* pCur, PositionTextureIndex* pOut, float t)
		{
			pOut->vTexCoords.x = pPrev->vTexCoords.x + t * (pCur->vTexCoords.x - pPrev->vTexCoords.x);
			pOut->vTexCoords.y = pPrev->vTexCoords.y + t * (pCur->vTexCoords.y - pPrev->vTexCoords.y);
		}

		static constexpr uint32					c_dwInputElements = 3;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct PositionColor
	{
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT4	vDiffuseColor;

		bool CompareTo(PositionColor* pOther)
		{
			return vPosition.x == pOther->vPosition.x && vPosition.y == pOther->vPosition.y && vPosition.z == pOther->vPosition.z &&
				vDiffuseColor.x == pOther->vDiffuseColor.x && vDiffuseColor.y == pOther->vDiffuseColor.y && vDiffuseColor.z == pOther->vDiffuseColor.z &&
				vDiffuseColor.w == pOther->vDiffuseColor.w;
		}

		static constexpr uint32					c_dwInputElements = 2;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct PositionColorScale
	{
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT4	vDiffuseColor;
		float				fScale;

		bool CompareTo(PositionColorScale* pOther)
		{
			return vPosition.x == pOther->vPosition.x && vPosition.y == pOther->vPosition.y && vPosition.z == pOther->vPosition.z &&
				vDiffuseColor.x == pOther->vDiffuseColor.x && vDiffuseColor.y == pOther->vDiffuseColor.y && vDiffuseColor.z == pOther->vDiffuseColor.z &&
				fScale == pOther->fScale;
		}

		static constexpr uint32					c_dwInputElements = 3;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct PositionColorTexture
	{
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT4	vDiffuseColor;
		DirectX::XMFLOAT2	vTexCoords;

		bool CompareTo(PositionColorTexture* pOther)
		{
			return vPosition.x == pOther->vPosition.x && vPosition.y == pOther->vPosition.y && vPosition.z == pOther->vPosition.z &&
				vDiffuseColor.x == pOther->vDiffuseColor.x && vDiffuseColor.y == pOther->vDiffuseColor.y && vDiffuseColor.z == pOther->vDiffuseColor.z &&
				vDiffuseColor.w == pOther->vDiffuseColor.w &&
				vTexCoords.x == pOther->vTexCoords.x && vTexCoords.y == pOther->vTexCoords.y;
		}

		static void ClipExtra(PositionColorTexture* pPrev, PositionColorTexture* pCur, PositionColorTexture* pOut, float fT)
		{
			pOut->vTexCoords.x = pPrev->vTexCoords.x + fT * (pCur->vTexCoords.x - pPrev->vTexCoords.x);
			pOut->vTexCoords.y = pPrev->vTexCoords.y + fT * (pCur->vTexCoords.y - pPrev->vTexCoords.y);

			pOut->vDiffuseColor = pCur->vDiffuseColor;
		}
		
		static constexpr uint32					c_dwInputElements = 3;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct PositionColorTextureIndex
	{
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT4	vDiffuseColor;
		DirectX::XMFLOAT2	vTexCoords;
		uint32				dwDataIndex;

		static constexpr uint32					c_dwInputElements = 4;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct PositionNormalColorTexture
	{
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT3	vNormal;
		DirectX::XMFLOAT4	vDiffuseColor;
		DirectX::XMFLOAT2	vTexCoords;

		bool CompareTo(PositionNormalColorTexture* pOther)
		{
			return vPosition.x == pOther->vPosition.x && vPosition.y == pOther->vPosition.y && vPosition.z == pOther->vPosition.z &&
				vDiffuseColor.x == pOther->vDiffuseColor.x && vDiffuseColor.y == pOther->vDiffuseColor.y && vDiffuseColor.z == pOther->vDiffuseColor.z &&
				vDiffuseColor.w == pOther->vDiffuseColor.w &&
				vNormal.x == pOther->vNormal.x && vNormal.y == pOther->vNormal.y && vNormal.z == pOther->vNormal.z &&
				vTexCoords.x == pOther->vTexCoords.x && vTexCoords.y == pOther->vTexCoords.y;
		}

		static constexpr uint32					c_dwInputElements = 4;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct PositionNormalColorTexture3Index4
	{
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT3	vNormal;
		DirectX::XMFLOAT4	vDiffuseColor;
		DirectX::XMFLOAT2	vTexCoords;
		DirectX::XMFLOAT2	vExtraCoords;
		DirectX::XMFLOAT2	vLightMapCoords;
		uint32				adwTextureIndices[4];

		static constexpr uint32					c_dwInputElements = 7;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct PositionNormalTextureSkinned
	{
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT3	vNormal;
		DirectX::XMFLOAT2	vTexCoords;
		DirectX::XMFLOAT4	vBlendWeight0;
		DirectX::XMFLOAT4	vBlendWeight1;
		DirectX::XMFLOAT4	vBlendWeight2;
		DirectX::XMFLOAT4	vBlendWeight3;
		uint32				adwBlendIndices[4];
		uint32				dwWeightCount;

		static constexpr uint32					c_dwInputElements = 9;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};

	struct PositionNormalTextureSkinnedIndex
	{
		DirectX::XMFLOAT3	vPosition;
		DirectX::XMFLOAT3	vNormal;
		DirectX::XMFLOAT2	vTexCoords;
		DirectX::XMFLOAT4	vBlendWeight0;
		DirectX::XMFLOAT4	vBlendWeight1;
		DirectX::XMFLOAT4	vBlendWeight2;
		DirectX::XMFLOAT4	vBlendWeight3;
		uint32				adwBlendIndices[4];
		uint32				dwWeightCount;
		uint32				dwTextureIndex;

		static constexpr uint32					c_dwInputElements = 10;
		static const D3D11_INPUT_ELEMENT_DESC	c_aInputElements[c_dwInputElements];
	};
}

#endif