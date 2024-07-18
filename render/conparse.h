#ifndef __CONPARSE_H__
#define __CONPARSE_H__

#define PARSE_MAXTOKENS     64
#define PARSE_MAXTOKENSIZE  80

typedef bool (*CompareChar)(char a, char b);

class ConParse 
{

public:

    ConParse();
    ConParse(char* szBuffer);

    void Init(char* szBuffer) { m_szCommandPos = szBuffer; }

    char*   m_pArgs[PARSE_MAXTOKENS];
    int     m_nArgs;

public:

    bool    Parse();
    bool    ParseFind(char* szLookFor, bool bCaseSensitive, int nMinParams);

private:

    bool    TokenCompare(char* szLookFor, char* szCurToken, CompareChar pCompareChar);
    void    FillBuffer(char* szPos);

    char*   m_szCommandPos;

    char    m_szArgBuffer[PARSE_MAXTOKENS * PARSE_MAXTOKENSIZE];
};

#endif