#ifndef __LT_MISC_TYPES__
#define __LT_MISC_TYPES__

struct IAggregate
{
	uint32*	m_pVTable;
	
	IAggregate	*m_pNextAggregate;
};

struct BaseClass
{
	uint32*	m_pVTable;	
	
	IAggregate*	m_pFirstAggregate;
	LTObject*	m_hObject;
	
	uint8	m_nType;
	
	LTBOOL	m_bTrueBaseClass;
};

#endif
