// shaders\model_GS.hlsl

#include "global_header.hlsl"

cbuffer C_PEROBJECT_BUFFER : register(b1)
{
    uint g_dwHiddenPieceCount : packoffset(c0);
    uint2 g_avHiddenPrimitiveID[MAX_PIECES_PER_MODEL] : packoffset(c1);
}

[maxvertexcount(3)]
void GSMain(triangle PS_INPUT_MODEL aInput[3], uint dwPrimitiveID : SV_PrimitiveID, inout TriangleStream<PS_INPUT_MODEL> outStream)
{ 
    [unroll(MAX_PIECES_PER_MODEL)]
    for (uint i = 0; i < g_dwHiddenPieceCount; i++)
    {
        if (dwPrimitiveID >= g_avHiddenPrimitiveID[i].x && dwPrimitiveID <= g_avHiddenPrimitiveID[i].y)
            return;
    }
    
    PS_INPUT_MODEL Output;
	
    [unroll(3)]
    for (i = 0; i < 3; ++i)
    {
        Output.vPosition = aInput[i].vPosition;
#ifdef MODEL_PIXEL_LIGHTING
        Output.vWorldPosition = aInput[i].vWorldPosition;
        Output.vWorldNormal = aInput[i].vWorldNormal;
#endif
        Output.vTexCoords = aInput[i].vTexCoords;
        Output.vEnvCoords = aInput[i].vEnvCoords;
        Output.vDiffuseColor = aInput[i].vDiffuseColor;
#ifndef MODEL_PIXEL_LIGHTING
        Output.vLightColor = aInput[i].vLightColor;
#endif
        Output.fFogFactor = aInput[i].fFogFactor;
        Output.dwTextureIndex = aInput[i].dwTextureIndex;
        
        outStream.Append(Output);
    }
}