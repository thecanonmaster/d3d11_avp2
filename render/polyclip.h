int bInside[50], * pInside;
uint32 nInside = 0;
T* pPrev, * pCur, * pEnd, * pOldOut;
uint32 iPrev, iCur;
float t;

pCur = pVerts;
pEnd = pCur + dwVerts;
pInside = bInside;
while (pCur != pEnd)
{
	*pInside = CLIPTEST(pCur->vPosition);
	nInside += *pInside;
	++pInside;
	++pCur;
}

if (nInside == 0)
{
	return false;
}
else if (nInside != dwVerts)
{
	pOldOut = pOut;

	iPrev = dwVerts - 1;
	pPrev = pVerts + iPrev;
	for (iCur = 0; iCur < dwVerts; iCur++)
	{
		pCur = pVerts + iCur;

		if (bInside[iPrev])
			*pOut++ = *pPrev;

		if (bInside[iPrev] != bInside[iCur])
		{
			DOCLIP(pPrev->vPosition, pCur->vPosition)

				T::ClipExtra(pPrev, pCur, pOut, t);

			++pOut;
		}

		iPrev = iCur;
		pPrev = pCur;
	}

	dwVerts = pOut - pOldOut;
	pVerts = pOldOut;
	pOut += dwVerts;
}
