#include "pch.h"

WorldBsp* MainWorld::GetSpecificBsp(uint32 dwTestFlags)
{
	uint32 dwModelCount = GetWorldModelCount();

	for (uint32 i = 0; i < dwModelCount; i++)
	{
		WorldBsp* pBsp = GetWorldModelData(i)->m_pOriginalBsp;

		if (pBsp->m_dwWorldInfoFlags & dwTestFlags)
			return pBsp;
	}

	return nullptr;
}
