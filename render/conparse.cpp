#include "pch.h"

#include "conparse.h"

ConParse::ConParse() : m_pArgs(), m_szArgBuffer()
{
	m_nArgs = 0;
	m_szCommandPos = nullptr;
}

ConParse::ConParse(char* szBuffer)
{
	ConParse();
	m_szCommandPos = szBuffer;
}

void ConParse::FillBuffer(char* szPos)
{
	uint32 dwTokenLen = szPos - m_szCommandPos;
	char* szDest = m_szArgBuffer + (m_nArgs * PARSE_MAXTOKENSIZE);
	strncpy_s(szDest, PARSE_MAXTOKENSIZE - 1, m_szCommandPos, dwTokenLen);

	szDest[dwTokenLen] = 0;
	m_pArgs[m_nArgs] = szDest;

	m_szCommandPos = szPos;
}

bool ConParse::TokenCompare(char* szLookFor, char* szCurToken, CompareChar pCompareChar)
{
	char* szLeft = szLookFor;
	char* szRight = szCurToken;

	while (*szLeft != 0 && *szRight != 0)
	{
		if (pCompareChar(*szLeft, *szRight))
			return false;

		szLeft++;
		szRight++;
	}

	return *szLeft == *szRight;
}

bool ConParse::ParseFind(char* szLookFor, bool bCaseSensitive, int nMinParams)
{
	auto lambdaCS = [](char a, char b) { return toupper(a) != toupper(b); };
	auto lambdaCI = [](char a, char b) { return a != b; };
	
	while (!Parse())
	{
		if (!m_nArgs)
			continue;

		if (!bCaseSensitive ? TokenCompare(szLookFor, m_pArgs[0], lambdaCS) : TokenCompare(szLookFor, m_pArgs[0], lambdaCI))
			return !(m_nArgs < nMinParams); // m_nArgs < nMinParams ? FALSE : TRUE;
	}

	return false;
}

bool ConParse::Parse()
{
	if (m_szCommandPos == nullptr || *m_szCommandPos == 0)
		return true;

	m_nArgs = 0;

	char* szPos = m_szCommandPos;

	while (true)
	{
		while (*szPos == ' ' && (*szPos != 0 && *szPos != ';'))
			szPos++;

		m_szCommandPos = szPos;

		if (*szPos == 0)
			return true;

		if (*szPos == ';')
		{
			m_szCommandPos++;
			return false;
		}
		
		while (*szPos != ' ' && (*szPos != 0 && *szPos != ';'))
			szPos++;

		FillBuffer(szPos);
		m_nArgs++;

		if (*szPos == 0)
			return true;
			
		if (*szPos == ';')
		{
			m_szCommandPos++;
			return false;
		}
	}

	return false;
}