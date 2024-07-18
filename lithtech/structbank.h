#ifndef __LT_STRUCT_BANK__
#define __LT_STRUCT_BANK__

struct StructLink 
{
    StructLink* m_pSLNext;
};

struct StructBankPage
{
    StructBankPage* m_pNext;

    uint32  m_dwObjects; 
    uint32  m_Data[1];
};

struct StructBank
{
    uint32  m_dwStructSize;
    uint32  m_dwAlignedStructSize;
    uint32  m_CacheSize;
    uint32  m_dwPages;
    uint32  m_dwTotalObjects;

    StructBankPage* m_pPageHead;
    StructLink* m_pFreeListHead;
};

struct ObjectBank
{
    uint32* m_pVTable;

    StructBank  m_Bank;
    uint32      m_CS;
};

#endif
